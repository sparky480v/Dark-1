// SPDX-License-Identifier: GPL-3.0-only
// DARK-1 (from AMB-1/PERF-1) in an AudioWorklet: amb.wasm is the whole FM-1 app (web/emu/amb_web.c). Each render
// quantum asks it for 128 frames, which runs the device's clock forward; between quanta the page's
// input goes in and, about 30 times a second, the screen, the lights and any saved objects go out.

const clock = globalThis.performance ? () => globalThis.performance.now() : () => Date.now();

class Amb extends AudioWorkletProcessor {
  constructor() {
    super();
    this.ex = null;
    this.input = null;
    this.sentBlits = -1;
    this.sentWrites = -1;
    this.lastFrame = 0;
    this.busy = 0;
    this.frames = 0;
    this.port.onmessage = (e) => this.onMessage(e.data);
  }

  async onMessage(m) {
    if (m.type === "load") {
      const now = () => BigInt(Math.round(currentTime * 1e9));
      const wasi = {
        clock_time_get: (id, prec, out) => {
          new DataView(this.mem.buffer).setBigUint64(out, now(), true);
          return 0;
        },
        fd_write: (fd, iov, n, out) => { new DataView(this.mem.buffer).setUint32(out, 0, true); return 0; },
        proc_exit: () => {},
      };
      const stub = new Proxy(wasi, { get: (t, k) => (k in t ? t[k] : () => 0) });
      const env = new Proxy({}, { get: () => () => 0 });
      const { instance } = await WebAssembly.instantiate(m.wasm, { wasi_snapshot_preview1: stub, env });
      this.ex = instance.exports;
      this.mem = this.ex.memory;
      if (this.ex._initialize) this.ex._initialize();
      const max = this.ex.web_store_max();
      for (const [obj, bytes] of Object.entries(m.store || {})) {
        const b = new Uint8Array(bytes).subarray(0, max);
        new Uint8Array(this.mem.buffer, this.ex.web_store(+obj), b.length).set(b);
        this.ex.web_store_set_len(+obj, b.length);
      }
      this.sentWrites = this.ex.web_store_writes();
      this.ex.web_master(m.master ?? 2800);
      if (this.ex.web_seed) this.ex.web_seed(m.seed >>> 0);   // a different start each time
      this.ex.web_boot();
      const str = (p) => { const u = new Uint8Array(this.mem.buffer); let s = ""; while (u[p]) s += String.fromCharCode(u[p++]); return s; };
      const ng = this.ex.web_ngenre(), np = this.ex.web_npiece(), genres = [], pieces = [];
      for (let g = 0; g < ng; g++) { genres.push(str(this.ex.web_genre_name(g))); const row = []; for (let i = 0; i < np; i++) row.push(str(this.ex.web_piece_name(g, i))); pieces.push(row); }
      this.port.postMessage({ type: "presets", genres, pieces, npiece: np, hash: this.ex.web_preset_hash() >>> 0, build: str(this.ex.web_build_id()),
                              g: this.ex.web_cur_genre(), i: this.ex.web_cur_piece() });
      this.port.postMessage({ type: "ready" });
    } else if (!this.ex) {
      return;
    } else if (m.type === "buttons") {
      this.ex.web_buttons(m.mask >>> 0);
    } else if (m.type === "keys") {
      this.ex.web_keys(m.mask >>> 0);
    } else if (m.type === "enc") {
      this.ex.web_enc(m.role, m.n | 0);
    } else if (m.type === "select") {
      this.want = { g: m.g | 0, i: m.i | 0, t: currentTime, tried: -1 };   // taken at once, and again until it has (the firmware refuses a genre change while it is storing)
      this.trySelect();
    } else if (m.type === "wander") {
      this.ex.web_wander_set(m.i | 0, m.v | 0);
    } else if (m.type === "master") {
      this.ex.web_master(m.value | 0);
    }
  }

  trySelect() {
    const w = this.want, ex = this.ex;
    if (!w) return;
    if (ex.web_cur_genre() === w.g && ex.web_cur_piece() === w.i) { this.want = null; return; }
    if (currentTime - w.t > 8) { this.port.postMessage({ type: "error", text: `select ${w.g}/${w.i} not taken in 8 s: engine at ${ex.web_cur_genre()}/${ex.web_cur_piece()}, store_wait ${ex.web_store_wait()}, log ${[0,1,2].map((k) => ex.web_sel_log(k))}` }); this.want = null; return; }
    if (currentTime - w.tried >= 0.05) { w.tried = currentTime; try { ex.web_select(w.g, w.i); } catch (e) { this.port.postMessage({ type: "error", text: String(e) }); this.want = null; } }
  }

  publish() {
    const ex = this.ex;
    const blits = ex.web_blits();
    const msg = { type: "frame", buttons: ex.web_lit_buttons() >>> 0, keys: ex.web_lit_keys() >>> 0, g: ex.web_cur_genre(), i: ex.web_cur_piece(), sw: ex.web_store_wait(), sl: [0, 1, 2].map((k) => ex.web_sel_log(k)), ring: Array.from(new Uint32Array(this.mem.buffer, ex.web_sel_ring(), 96)) };
    msg.wd = [0, 1, 2, 3, 4, 5, 6, 7].map((k) => ex.web_wander_get(k)); msg.wi = [0, 1, 2, 3].map((k) => ex.web_wander_info(k));   // Wander: its settings, and what it has done
    if (ex.web_bg) msg.bg = Array.from(new Float32Array(this.mem.buffer, ex.web_bg(), 12));   // for the stream view's background
    if (this.frames >= 44100) {                       // the share of real time spent running the device
      msg.load = Math.round(100 * this.busy / (this.frames / 44.1));
      this.busy = this.frames = 0;
    }
    const transfer = [];
    if (blits !== this.sentBlits) {
      this.sentBlits = blits;
      msg.fb = new Uint16Array(this.mem.buffer, ex.web_fb(), 240 * 240).slice();
      transfer.push(msg.fb.buffer);
    }
    const writes = ex.web_store_writes();
    if (writes !== this.sentWrites) {
      this.sentWrites = writes;
      msg.store = {};
      for (let o = 0; o < ex.web_nobj(); o++) {
        const n = ex.web_store_len(o);
        if (n >= 0) msg.store[o] = new Uint8Array(this.mem.buffer, ex.web_store(o), n).slice();
      }
    }
    this.port.postMessage(msg, transfer);
  }

  process(inputs, outputs) {
    if (!this.ex) return true;
    if (this.want) this.trySelect();
    const out = outputs[0];
    const n = out[0].length;
    const t0 = clock();
    this.ex.web_render(n);
    this.busy += clock() - t0;
    this.frames += n;
    out[0].set(new Float32Array(this.mem.buffer, this.ex.web_out_l(), n));
    if (out[1]) out[1].set(new Float32Array(this.mem.buffer, this.ex.web_out_r(), n));
    if (currentTime - this.lastFrame >= 1 / 30) {
      this.lastFrame = currentTime;
      this.publish();
    }
    return true;
  }
}

registerProcessor("amb", Amb);
