#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# DARK-1: one self-contained HTML file from build/emu/{index.html,worklet.js,amb.wasm}: the wasm is embedded (base64) and the worklet is loaded from a data: URL (a Blob URL fails on file://).
# usage: make_standalone.py [build/emu] [out.html]
import base64, sys, os, re
SHIM = r'''
// A worklet from a data: or blob: URL is refused in some contexts (file://, sandboxed viewers, strict CSP). Try a Blob URL, then a data URL; if both are refused
// run the same worklet class on the main thread through a ScriptProcessorNode (deprecated, but it works everywhere).
async function DARK_MAKE_NODE(ac) {
  const urls = [() => URL.createObjectURL(new Blob([DARK_WORKLET_SRC], { type: "text/javascript" })), () => "data:text/javascript;base64," + btoa(DARK_WORKLET_SRC)];
  for (const mk of urls) {
    try {
      await ac.audioWorklet.addModule(mk());
      return new AudioWorkletNode(ac, "amb", { numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2] });
    } catch (e) { console.warn("worklet module refused:", String(e).slice(0, 120)); }
  }
  console.warn("DARK-1: running the engine on the main thread (ScriptProcessor)");
  let Cls = null;
  const base = class { constructor() { this.port = { onmessage: null, postMessage: (m) => queueMicrotask(() => sp.port.onmessage && sp.port.onmessage({ data: m })) }; } };
  const src = DARK_WORKLET_SRC.replace(/\bcurrentTime\b/g, "ctx.currentTime");
  new Function("AudioWorkletProcessor", "registerProcessor", "ctx", src)(base, (n, k) => { Cls = k; }, ac);
  const inst = new Cls();
  const sp = ac.createScriptProcessor(2048, 0, 2);
  sp.port = { postMessage: (m) => { inst.port.onmessage && inst.port.onmessage({ data: m }); } };
  sp.onaudioprocess = (ev) => {
    const L = ev.outputBuffer.getChannelData(0), R = ev.outputBuffer.getChannelData(1), l = new Float32Array(128), r = new Float32Array(128);
    for (let o = 0; o < L.length; o += 128) {
      inst.process([], [[l, r]]);
      L.set(l, o); R.set(r, o);
    }
  };
  return sp;
}
'''
d = sys.argv[1] if len(sys.argv) > 1 else 'build/emu'
out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(d, 'DARK-1.html')
html = open(os.path.join(d, 'index.html')).read()
wasm = open(os.path.join(d, 'amb.wasm'), 'rb').read()
wk = open(os.path.join(d, 'worklet.js')).read()
old = re.search(r'const \[wasm\] = await Promise\.all\(\[fetch\("amb\.wasm".*?addModule\("worklet\.js\?v=" \+ Date\.now\(\)\)\]\);', html, re.S)
assert old, 'fetch block not found'
new = ('const wasm = Uint8Array.from(atob(DARK_WASM_B64), (ch) => ch.charCodeAt(0)).buffer;\n'
       '    node = await DARK_MAKE_NODE(ac);')
html = html[:old.start()] + new + html[old.end():]
node_line = re.search(r'\n\s*node = new AudioWorkletNode\(ac, "amb", \{[^\n]*\}\);', html)
assert node_line, 'AudioWorkletNode line not found'
html = html[:node_line.start()] + html[node_line.end():]
blob = '<script>\nconst DARK_WASM_B64 = "' + base64.b64encode(wasm).decode() + '";\nconst DARK_WORKLET_SRC = ' + __import__('json').dumps(wk) + ';\n' + SHIM + '</script>\n'
i = html.index('<script')
html = html[:i] + blob + html[i:]
open(out, 'w').write(html)
print('wrote', out, len(html), 'bytes (wasm %d)' % len(wasm))
