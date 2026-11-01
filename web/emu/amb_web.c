/* SPDX-License-Identifier: GPL-3.0-only */
/* AMB-1 in the browser: the host simulator (host/amb_host.c: the whole app, UI and instrument,
 * against a simulated FM-1) compiled to WebAssembly and run in an AudioWorklet (from X0X's). The worklet asks
 * for audio; producing it advances the device's clock one millisecond at a time, which runs the
 * engine every 256 samples (the I2S half buffer) and the UI every 16 ms, as on the FM-1. The page
 * sends input, and reads the screen, the lights and the saved objects back through these exports.
 *
 *   web/emu/build.sh   ->  build/emu/amb.wasm (+ the page) */
#include <stdint.h>

static void web_audio(const int32_t *blk, uint32_t n);
#include "../../host/amb_host.c"

#undef __attribute__

/* output ring, stereo float, filled by run_ms, drained by web_render */
#define RING 4096u
static float ring[RING * 2];
static uint32_t ring_w, ring_r;
static void web_audio(const int32_t *blk, uint32_t n)
{
    uint32_t k;
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_w++ % RING) * 2u;
        ring[i] = (float)blk[2 * k] / 8388608.0f;
        ring[i + 1] = (float)blk[2 * k + 1] / 8388608.0f;
    }
}

static float out_l[1024], out_r[1024];
__attribute__((used, visibility("default"))) float *web_out_l(void) { return out_l; }
__attribute__((used, visibility("default"))) float *web_out_r(void) { return out_r; }

/* n frames (<= 1024) into out_l / out_r: the device runs until they exist */
__attribute__((used, visibility("default"))) void web_render(uint32_t n)
{
    uint32_t k;
    if (n > 1024u)
        n = 1024u;
    while (ring_w - ring_r < n)
        run_ms(1);
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_r++ % RING) * 2u;
        out_l[k] = ring[i];
        out_r[k] = ring[i + 1];
    }
}

__attribute__((used, visibility("default"))) void web_boot(void)
{
    ui_splash_ms = 3200u;                        /* the boot cards, as on the FM-1 */
    boot();
}
__attribute__((used, visibility("default"))) void web_seed(uint32_t s) { host_seed = s; }   /* before web_boot */
__attribute__((used, visibility("default"))) void web_buttons(uint32_t m) { held_btn = m; }
__attribute__((used, visibility("default"))) void web_keys(uint32_t m) { held_keys = m; }
__attribute__((used, visibility("default"))) void web_enc(int role, int32_t n)
{
    if (role >= 0 && role < NE)
        enc_acc[role] += n;
}
__attribute__((used, visibility("default"))) void web_master(uint32_t v) { master = v > 4096u ? 4096u : v; }
__attribute__((used, visibility("default"))) uint32_t web_lit_buttons(void) { return lit_btn; }
__attribute__((used, visibility("default"))) uint32_t web_lit_keys(void) { return lit_keys; }
__attribute__((used, visibility("default"))) uint16_t *web_fb(void) { return fb; }
__attribute__((used, visibility("default"))) uint32_t web_blits(void) { return blits; }
__attribute__((used, visibility("default"))) uint32_t web_now(void) { return now_ms; }
__attribute__((used, visibility("default"))) uint32_t web_cpu(void) { return cpu_pct; }

/* saved objects: the page keeps them (localStorage) and gives them back before boot */
__attribute__((used, visibility("default"))) uint8_t *web_store(uint32_t obj) { return obj < OBJ_NOBJ ? store[obj] : 0; }
__attribute__((used, visibility("default"))) int web_store_len(uint32_t obj) { return obj < OBJ_NOBJ ? store_len[obj] : -1; }
__attribute__((used, visibility("default"))) void web_store_set_len(uint32_t obj, int n)
{
    if (obj < OBJ_NOBJ)
        store_len[obj] = n < 0 ? -1 : n > (int)PLAT_STORE_MAX ? (int)PLAT_STORE_MAX : n;
}
__attribute__((used, visibility("default"))) uint32_t web_store_writes(void) { return store_writes; }
__attribute__((used, visibility("default"))) uint32_t web_nobj(void) { return OBJ_NOBJ; }
__attribute__((used, visibility("default"))) uint32_t web_store_max(void) { return PLAT_STORE_MAX; }

static float bg_out[12];                         /* for the stream view's background: what the screen shows */
static void bg_rgb(int at, uint16_t c)
{
    bg_out[at] = (float)(c >> 11) / 31.0f;
    bg_out[at + 1] = (float)((c >> 5) & 63u) / 63.0f;
    bg_out[at + 2] = (float)(c & 31u) / 31.0f;
}
/* the stream view's background: the screen theme's primary, accent and ground */
__attribute__((used, visibility("default"))) float *web_bg(void)
{
    bg_rgb(0, th->pad);
    bg_rgb(3, th->acc);
    bg_rgb(6, th->bg);
    bg_rgb(9, th->acid);
    return bg_out;
}

/* ---- DARK-1: the preset list and what is playing, for the page (and the tests) ---- */
#define WEBX __attribute__((used, visibility("default")))
WEBX int web_ngenre(void) { return EN_NGENRE; }
WEBX int web_npiece(void) { return NPIECES; }
WEBX const char *web_genre_name(int g) { return en_genre_name(g); }
WEBX const char *web_piece_name(int g, int i) { return en_piece_name_g(g, i); }
WEBX void web_select(int g, int i) { ui_select_preset(g, i); }
WEBX int web_cur_genre(void) { return proj.genre; }
WEBX int web_cur_piece(void) { return proj.piece; }
WEBX int web_par(int p) { return (unsigned)p < P_NPARAMS ? proj.par[p] : -32768; }
WEBX int web_nparams(void) { return P_NPARAMS; }
WEBX int web_pat(int p, int l) { return (unsigned)p < EN_NPAT && (unsigned)l < EN_NLANE ? en_pats[p][l] : -1; }
WEBX int web_pool(void) { return proj.pool; }
WEBX const char *web_build_id(void) { return "DARK-1 (PERF-1 engine, GPL-3.0-only)"; }
/* the factory recipe of (g,i) straight from the engine's table: param p (-32768 if out of range) */
WEBX int web_recipe_par(int g, int i, int p)
{
    static int16_t par[P_NPARAMS];
    static uint16_t pool;
    if ((unsigned)g >= EN_NGENRE || (unsigned)i >= NPIECES || (unsigned)p >= P_NPARAMS)
        return -32768;
    en_piece_g(g, i, par, &pool);
    return par[p];
}
/* an FNV-1a hash over every factory recipe: the page shows it, the tests compare it to the one the native tool computes */
WEBX uint32_t web_preset_hash(void)
{
    static int16_t par[P_NPARAMS];
    static uint16_t pool;
    uint32_t h = 2166136261u;
    int g, i, p;
    for (g = 0; g < EN_NGENRE; g++)
        for (i = 0; i < NPIECES; i++) {
            en_piece_g(g, i, par, &pool);
            for (p = 0; p < P_NPARAMS; p++)
                h = (h ^ (uint16_t)par[p]) * 16777619u;
            h = (h ^ pool) * 16777619u;
        }
    return h;
}
/* DARK-1 Wander: the page's controls, the same eight settings as the WANDER pages (0 on, 1 every, 2 amount, 3 pieces, 4 kits, 5 sounds, 6 knobs, 7 layers) */
WEBX int web_wander_get(int i) { return (unsigned)i < 8u ? *uiv(18 + i) : -1; }
WEBX void web_wander_set(int i, int v)
{
    static const uint8_t hi[8] = {1, 4, 2, 2, 1, 1, 1, 1};
    if ((unsigned)i < 8u)
        *uiv(18 + i) = (uint8_t)(v < 0 ? 0 : v > hi[i] ? hi[i] : v);
}
WEBX int web_wander_info(int k)    /* 0 bars since its last change, 1 the layers it has muted, 2 the sounds it has swapped (a bit each), 3 the knobs it has moved (a bit each) */
{
    int i, m = 0;
    if (k == 0)
        return ui.w_n;
    if (k == 1)
        return ui.w_mute;
    for (i = 0; i < (k == 2 ? 8 : 11); i++)
        if (k == 2 ? ui.w_ovr[i] >= 0 : ui.w_off[i] != 0)
            m |= 1 << i;
    return m;
}
WEBX int web_store_wait(void) { return ui.store_wait; }
WEBX uint32_t web_sel_log(int k) { return (unsigned)k < 4u ? ui_sel_log[k] : 0u; }
WEBX uint32_t *web_sel_ring(void) { return &ui_sel_ring[0][0]; }
