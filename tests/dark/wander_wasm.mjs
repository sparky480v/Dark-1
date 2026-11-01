// SPDX-License-Identifier: GPL-3.0-only
// DARK-1 Wander in the real engine as the browser runs it (build/emu/amb.wasm, in Node): the page's own entry points (web_wander_set / _get / _info),
// minutes of audio rendered, and what Wander did read back.
//   node tests/dark/wander_wasm.mjs build/emu/amb.wasm out.json [minutes]
import fs from "fs";
const wasm = fs.readFileSync(process.argv[2]);
const outFile = process.argv[3] || "wander_wasm.json";
const MIN = +(process.argv[4] || 3);
let mem;
const wasi = new Proxy({ clock_time_get: (id, p, out) => { new DataView(mem.buffer).setBigUint64(out, 0n, true); return 0; },
  fd_write: (fd, iov, n, out) => { new DataView(mem.buffer).setUint32(out, 0, true); return 0; } }, { get: (t, k) => k in t ? t[k] : () => 0 });
const { instance } = await WebAssembly.instantiate(wasm, { wasi_snapshot_preview1: wasi, env: new Proxy({}, { get: () => () => 0 }) });
const ex = instance.exports; mem = ex.memory;
if (ex._initialize) ex._initialize();
const fails = [];
const render = (secs) => {
  const n = Math.round(secs * 44100 / 128), buf = new Float32Array(n * 128);
  for (let i = 0; i < n; i++) { ex.web_render(128); buf.set(new Float32Array(mem.buffer, ex.web_out_l(), 128), i * 128); }
  return buf;
};
const level = (x) => { let pk = 0, s = 0, bad = false; for (const v of x) { if (!Number.isFinite(v)) bad = true; pk = Math.max(pk, Math.abs(v)); s += v * v; } return { pk, rms: Math.sqrt(s / x.length), bad }; };
function run(label, settings, minutes) {
  ex.web_master(2800); ex.web_seed(7); ex.web_boot();
  render(4);
  for (const [i, v] of settings) ex.web_wander_set(i, v);
  const rows = [], seen = new Set(), kinds = { sound: 0, knobs: 0, layers: 0 };
  let lastPiece = `${ex.web_cur_genre()}/${ex.web_cur_piece()}`, changes = 0;
  for (let w = 0; w < minutes * 6; w++) {                     // ten-second windows
    const x = render(10), l = level(x);
    const now = `${ex.web_cur_genre()}/${ex.web_cur_piece()}`;
    if (now !== lastPiece) { changes++; lastPiece = now; }
    seen.add(now);
    if (ex.web_wander_info(2)) kinds.sound++;
    if (ex.web_wander_info(3)) kinds.knobs++;
    if (ex.web_wander_info(1)) kinds.layers++;
    rows.push({ w, piece: now, ...l, info: [0, 1, 2, 3].map((k) => ex.web_wander_info(k)) });
    if (l.bad) fails.push(`${label}: NaN or infinity in window ${w}`);
    if (l.pk < 0.01) fails.push(`${label}: silent window ${w} (${now})`);
    if (l.pk > 0.999) fails.push(`${label}: clipping in window ${w} (${now})`);
  }
  return { label, rows, pieces: seen.size, changes, kinds };
}
// the settings: 0 on, 1 every (0: 4 bars), 2 amount (2: wild), 3 pieces (2: any genre), 4 kits, 5 sounds, 6 knobs, 7 layers
const off = run("off", [], MIN);
if (off.changes !== 0 || off.pieces !== 1) fails.push(`off: the piece changed by itself (${off.pieces} pieces seen)`);
if (off.kinds.sound + off.kinds.knobs + off.kinds.layers) fails.push("off: something wandered");
for (let i = 0; i < 8; i++) ex.web_wander_set(i, 0);
const wild = run("wild", [[1, 0], [2, 2], [3, 2], [4, 1], [5, 1], [6, 1], [7, 1], [0, 1]], MIN);
if (ex.web_wander_get(0) !== 1) fails.push("wild: the setting did not hold");
if (wild.pieces < 5) fails.push(`wild: only ${wild.pieces} different pieces in ${MIN} minutes`);
if (wild.kinds.sound < 1) fails.push("wild: no sound swapped");
if (wild.kinds.knobs < 1) fails.push("wild: no knob moved");
if (wild.kinds.layers < 1) fails.push("wild: no layer muted");
const subtle = run("subtle", [[1, 1], [2, 0], [3, 1], [4, 1], [5, 1], [6, 1], [7, 1], [0, 1]], MIN);
if (subtle.changes > wild.changes) fails.push("subtle changes pieces more than wild");
// switched off again: everything is the piece's own
ex.web_wander_set(0, 0); render(2);
if (ex.web_wander_info(1) || ex.web_wander_info(2) || ex.web_wander_info(3)) fails.push("after Wander is off: a mute, a swapped sound or a moved knob is left");
fs.writeFileSync(outFile, JSON.stringify({ off, wild, subtle, fails }, null, 1));
console.log(`wander in wasm: off ${off.pieces} piece(s); wild ${wild.pieces} pieces, ${wild.changes} piece changes, windows with sounds swapped ${wild.kinds.sound}, knobs ${wild.kinds.knobs}, layers ${wild.kinds.layers}; subtle ${subtle.pieces} pieces; fails: ${fails.length}`);
for (const f of fails.slice(0, 20)) console.log("  " + f);
process.exit(fails.length ? 1 : 0);
