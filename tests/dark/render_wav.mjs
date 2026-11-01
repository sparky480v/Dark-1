// SPDX-License-Identifier: GPL-3.0-only
// DARK-1: audio demos from the real engine (the wasm): one WAV per genre (piece 1), or the pieces you name.
//   node tests/dark/render_wav.mjs build/emu/amb.wasm outdir [seconds=30] [g:i ...]
import fs from "fs";
const [, , wasmPath, outDir, secArg, ...sel] = process.argv;
const SECS = +(secArg || 30);
const wasm = fs.readFileSync(wasmPath); let mem;
const wasi = new Proxy({ clock_time_get: (id, p, out) => { new DataView(mem.buffer).setBigUint64(out, 0n, true); return 0; },
  fd_write: (fd, iov, n, out) => { new DataView(mem.buffer).setUint32(out, 0, true); return 0; } }, { get: (t, k) => k in t ? t[k] : () => 0 });
const { instance } = await WebAssembly.instantiate(wasm, { wasi_snapshot_preview1: wasi, env: new Proxy({}, { get: () => () => 0 }) });
const ex = instance.exports; mem = ex.memory; ex._initialize && ex._initialize();
const str = (p) => { const m = new Uint8Array(mem.buffer); let s = ""; while (m[p]) s += String.fromCharCode(m[p++]); return s; };
ex.web_master(2800); ex.web_seed(1); ex.web_boot();
const run = (s) => { const n = Math.round(s * 44100 / 128), L = new Float32Array(n * 128), R = new Float32Array(n * 128);
  for (let i = 0; i < n; i++) { ex.web_render(128); L.set(new Float32Array(mem.buffer, ex.web_out_l(), 128), i * 128); R.set(new Float32Array(mem.buffer, ex.web_out_r(), 128), i * 128); } return [L, R]; };
run(4);
fs.mkdirSync(outDir, { recursive: true });
const list = sel.length ? sel.map((s) => s.split(":").map(Number)) : Array.from({ length: ex.web_ngenre() }, (_, g) => [g, 0]);
for (const [g, i] of list) {
  ex.web_select(g, i); run(1);
  const [L, R] = run(SECS), n = L.length, buf = Buffer.alloc(44 + n * 4);
  buf.write("RIFF", 0); buf.writeUInt32LE(36 + n * 4, 4); buf.write("WAVEfmt ", 8); buf.writeUInt32LE(16, 16); buf.writeUInt16LE(1, 20); buf.writeUInt16LE(2, 22);
  buf.writeUInt32LE(44100, 24); buf.writeUInt32LE(44100 * 4, 28); buf.writeUInt16LE(4, 32); buf.writeUInt16LE(16, 34); buf.write("data", 36); buf.writeUInt32LE(n * 4, 40);
  for (let k = 0; k < n; k++) { buf.writeInt16LE(Math.max(-32768, Math.min(32767, Math.round(L[k] * 32767))), 44 + k * 4); buf.writeInt16LE(Math.max(-32768, Math.min(32767, Math.round(R[k] * 32767))), 46 + k * 4); }
  const f = `${outDir}/${String(g).padStart(2, "0")}_${str(ex.web_genre_name(g))}_${str(ex.web_piece_name(g, i))}.wav`;
  fs.writeFileSync(f, buf); console.log(f);
}
