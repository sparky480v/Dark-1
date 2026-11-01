// SPDX-License-Identifier: GPL-3.0-only
// DARK-1: the real engine (build/emu/amb.wasm) in Node.  Selects every genre x piece through the same entry the page uses (web_select),
// checks the parameters reached the engine, renders audio and writes per-preset statistics (peak, rms, spectral bands) to a JSON file.
//   node tests/dark/render_all.mjs build/emu/amb.wasm out.json [seconds-per-preset]
import fs from "fs";
const wasm = fs.readFileSync(process.argv[2]);
const outFile = process.argv[3] || "render_stats.json";
const SECS = +(process.argv[4] || 4);
let mem;
const wasi = new Proxy({ clock_time_get: (id, p, out) => { new DataView(mem.buffer).setBigUint64(out, 0n, true); return 0; },
  fd_write: (fd, iov, n, out) => { new DataView(mem.buffer).setUint32(out, 0, true); return 0; } }, { get: (t, k) => k in t ? t[k] : () => 0 });
const { instance } = await WebAssembly.instantiate(wasm, { wasi_snapshot_preview1: wasi, env: new Proxy({}, { get: () => () => 0 }) });
const ex = instance.exports; mem = ex.memory;
if (ex._initialize) ex._initialize();
const str = (p) => { const m = new Uint8Array(mem.buffer); let s = ""; while (m[p]) s += String.fromCharCode(m[p++]); return s; };
ex.web_master(2800); ex.web_seed(1); ex.web_boot();
const fails = [];
const render = (secs) => {
  const n = Math.round(secs * 44100 / 128), buf = new Float32Array(n * 128);
  for (let i = 0; i < n; i++) { ex.web_render(128); buf.set(new Float32Array(mem.buffer, ex.web_out_l(), 128), i * 128); }
  return buf;
};
function fft(re, im) {                       // in-place radix-2
  const n = re.length;
  for (let i = 1, j = 0; i < n; i++) { let b = n >> 1; for (; j & b; b >>= 1) j ^= b; j ^= b; if (i < j) { [re[i], re[j]] = [re[j], re[i]]; [im[i], im[j]] = [im[j], im[i]]; } }
  for (let len = 2; len <= n; len <<= 1) {
    const a = -2 * Math.PI / len, wr = Math.cos(a), wi = Math.sin(a);
    for (let i = 0; i < n; i += len) { let cr = 1, ci = 0;
      for (let k = 0; k < len / 2; k++) { const u = i + k, v = i + k + len / 2;
        const tr = re[v] * cr - im[v] * ci, ti = re[v] * ci + im[v] * cr;
        re[v] = re[u] - tr; im[v] = im[u] - ti; re[u] += tr; im[u] += ti;
        const t = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = t; } } }
}
function stats(x) {
  let pk = 0, s = 0; for (const v of x) { pk = Math.max(pk, Math.abs(v)); s += v * v; }
  const N = 8192, bands = [0, 0, 0, 0, 0]; const edges = [0, 120, 500, 2000, 6000, 22050]; let wsum = 0, cen = 0, frames = 0;
  for (let o = 0; o + N <= x.length; o += N * 2) {
    const re = new Float64Array(N), im = new Float64Array(N);
    for (let i = 0; i < N; i++) re[i] = x[o + i] * (0.5 - 0.5 * Math.cos(2 * Math.PI * i / N));
    fft(re, im);
    for (let k = 1; k < N / 2; k++) { const f = k * 44100 / N, p = re[k] * re[k] + im[k] * im[k];
      for (let b = 0; b < 5; b++) if (f >= edges[b] && f < edges[b + 1]) bands[b] += p; cen += f * p; wsum += p; }
    frames++;
  }
  const tot = bands.reduce((a, b) => a + b, 0) || 1;
  return { peak: +pk.toFixed(4), rms: +Math.sqrt(s / x.length).toFixed(5), centroid: +(cen / (wsum || 1)).toFixed(1), bands: bands.map((b) => +(b / tot).toFixed(4)),
           nan: x.some((v) => !Number.isFinite(v)) };
}
render(4);                                    // past the boot cards
const res = [];
const t0 = Date.now();
for (let g = 0; g < ex.web_ngenre(); g++) for (let i = 0; i < ex.web_npiece(); i++) {
  ex.web_select(g, i);
  if (ex.web_cur_genre() !== g || ex.web_cur_piece() !== i) fails.push(`select ${g}/${i} -> ${ex.web_cur_genre()}/${ex.web_cur_piece()}`);
  let diff = 0, cnt = 0;                      // the recipe's values must be in the project (the sticky ones: theme, tune, midi: are the player's)
  for (let p = 0; p < ex.web_nparams(); p++) { const r = ex.web_recipe_par(g, i, p), a = ex.web_par(p); cnt++; if (r !== a) diff++; }
  render(0.5);
  const x = render(SECS);
  const st = stats(x);
  res.push({ g, i, genre: str(ex.web_genre_name(g)), piece: str(ex.web_piece_name(g, i)), paramMismatch: diff, ...st });
  if (st.nan) fails.push(`NaN ${g}/${i}`);
  if (st.peak < 0.01) fails.push(`silent ${g}/${i}`);
  if (st.peak > 0.999) fails.push(`clip ${g}/${i}`);
}
fs.writeFileSync(outFile, JSON.stringify({ hash: ex.web_preset_hash() >>> 0, build: str(ex.web_build_id()), results: res, fails }, null, 1));
console.log(`rendered ${res.length} presets in ${((Date.now() - t0) / 1000).toFixed(0)} s; fails: ${fails.length}`);
for (const f of fails.slice(0, 20)) console.log("  " + f);
process.exit(fails.length ? 1 : 0);
