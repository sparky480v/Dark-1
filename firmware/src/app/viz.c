/* SPDX-License-Identifier: GPL-3.0-only */
/* PERF-1 visualizers: the main area as a light show that plays with the music (HOME double-tapped; HOME again leaves;
 * the SELECT dial chooses). Part of the unity build, included by ui.c just before draw_main. Fifteen frames a second, in
 * the theme's colours. The screensaver has the whole screen (240 high, drawn in two bands of 120: the state moves once a frame, the drawing is
 * done for each band); the page leaves 144 rows. Everything is drawn from what the engine already tells the screen (the drums' glows, the layers'
 * levels, the chord, the pattern, the section): no audio is analysed. The knobs stay the FX knobs, ALGORITHM the mood.
 *   PLASMA  a colour field that swirls faster with the energy and flashes on the kick
 *   KALEIDO the classic media-player pattern: curves from the spectrum, mirrored four ways, trailing and turning (it replaced the tunnel)
 *   BARS    an equaliser: lows from the kick and the drone, mids from the pad and the snare, highs from the hats and the acid
 *   FLUX    the classic media-player light show: the spectrum fed back on itself, a trippy bloom
 *   SIGIL   a ritual circle with a turning pentagram and a demon eye, over a floor that lights with the drum pattern
 *   LIFE    a cellular automaton that the drums feed (the kick a cluster, the snare a spray, a hat a spark)
 *   ORBIT   a harmonograph drawn from the chord's notes: it morphs with every chord
 *   AUTO    one of them, a new one on every section change and every 32 bars */
#define VZ_W 240
#define VZ_H 144
#define VZ_CW 40                                /* the cell grids: 6 pixel cells */
#define VZ_CH 40                                /* (rows: 24 on the page, 40 on the whole screen) */
#define VZ_ROWS (vz.h / 6)

static struct {
    int32_t floor_y;                           /* the floor's top (the eye sits above it) */
    int32_t h, y0;                             /* the picture's height (144 on the page, 240 whole screen) and, drawing the whole screen, the band's top row */
    uint8_t full, pass;                        /* the whole screen; which band (the state moves in the first) */
    float tt, ph, ph2, dt;                     /* time (tempo scaled), two phases, the frame's seconds */
    float kick, snare, hat, en;                /* the frame's signals */
    float bars[24], caps[24], spec[24];
    float cf[3];
    union {
        uint8_t life[2][VZ_CH][VZ_CW];
        uint8_t fb[2][60][60];                 /* FLUX's and KALEIDO's picture, two frames (60 x 36 cells on the page, 60 x 60 whole screen) */
    } buf;
    uint8_t fcur;
    uint8_t lcur, last_step, mode_seen, auto_cur, sec_seen, inited;
    uint32_t seed, last_ms, auto_bar, life_ms;
    uint16_t lut[64];
    float dh, dhv, sprev;                      /* the eye: how open (0..1) and its pupil's width; the snare's last level */
    uint8_t dsec, dpos;                        /* the section last seen; which way the eye looks (flips on the snare) */
    uint32_t blink_until, blink_next;          /* the eye's blink: when it ends; the next one */
    uint32_t drop_bar;                         /* the sixteenth the drop came on */
} vz EN_POOL;

static int vz_vis(int32_t y, int32_t h) { return !vz.full || (y + h > vz.y0 && y < vz.y0 + 120); }   /* does a rectangle touch the band being drawn */

static uint32_t vz_rnd(void)
{
    vz.seed = vz.seed * 1664525u + 1013904223u;
    return vz.seed >> 8;
}
static float vz_rf(void) { return (float)(vz_rnd() & 0xFFFFu) * (1.0f / 65536.0f); }
static uint16_t vpal(float x)                   /* the theme's colours, round and round */
{
    return vz.lut[(int)((x - fm_floorf(x)) * 63.99f)];
}
static void vz_lut(void)
{
    const uint16_t stop[6] = {K_BG, mix(K_BG, K_PAD_T, 10), mix(K_BG, K_PAD, 9), mix(K_BG, K_ACID, 11), K_ACC, mix(K_BG, K_PAD_T, 6)};   /* dark techno: mostly near black, a deep colour, and the accent only at the peaks */
    int i;
    for (i = 0; i < 64; i++) {
        const int s = i * 5 / 64, t = (i * 5 % 64) * 16 / 64;
        vz.lut[i] = mix(stop[s], stop[s + 1], t);
    }
}

static void vz_signals(void)
{
    uint32_t now = plat_ms();
    float dt = (float)(now - vz.last_ms) * 0.001f;
    vz.last_ms = now;
    vz.dt = fm_clampf(dt, 0.001f, 0.12f);
    vz.kick = fm_clampf(en_drum_glow[0], 0.0f, 1.0f);
    vz.snare = fm_clampf(en_drum_glow[1], 0.0f, 1.0f);
    vz.hat = fm_clampf(en_drum_glow[3], 0.0f, 1.0f);
    vz.en = fm_clampf(0.3f * (en_pad_level + en_bass_level + en_acid_level) + 0.5f * fm_maxf(vz.kick, vz.snare), 0.0f, 1.0f);
    if (!en_playing)
        vz.kick = vz.snare = vz.hat = vz.en = 0.0f;
    vz.tt += vz.dt * (0.5f + (float)proj.par[P_TEMPO] * 0.008f) * (1.0f + vz.en);
    vz.ph += vz.dt * (0.02f + 0.08f * vz.en);
    vz.ph2 += vz.dt * (0.2f + 0.6f * vz.kick);
    if (vz.ph > 1000.0f)
        vz.ph -= 1000.0f;
    if (vz.ph2 > 1000.0f)
        vz.ph2 -= 1000.0f;
    if (vz.tt > 1000.0f)
        vz.tt -= 1000.0f;
}

static void vz_bars(void)                       /* the real thing: the master's spectrum (en_spectrum), 24 bands */
{
    const int rows = (int)(vz.h - 12) / 10;
    const int32_t base = vz.h - 12;
    int i, j;
    if (!vz.pass) {
        en_spectrum(vz.spec, 24);
        for (i = 0; i < 24; i++) {
            const float tg = vz.spec[i];
            vz.bars[i] = tg > vz.bars[i] ? tg : fm_maxf(vz.bars[i] - vz.dt * 1.8f, 0.0f);
            vz.caps[i] = vz.bars[i] > vz.caps[i] ? vz.bars[i] : fm_maxf(vz.caps[i] - vz.dt * 0.5f, 0.0f);
        }
    }
    for (i = 0; i < 24; i++) {
        const float u = (float)i * (1.0f / 23.0f);
        for (j = 0; j < (int)(vz.bars[i] * (float)rows + 0.5f); j++)
            if (vz_vis(base - j * 10, 8))
                cv_rect(i * 10 + 1, base - j * 10, 8, 8, vpal((float)j * 0.035f * 14.0f / (float)rows + u * 0.3f + vz.ph));
        if (vz_vis(base - (int32_t)(vz.caps[i] * (float)rows + 0.5f) * 10, 3))
            cv_rect(i * 10 + 1, base - (int32_t)(vz.caps[i] * (float)rows + 0.5f) * 10, 8, 3, K_TEXT);
    }
}
static void vz_ring(int32_t cx, int32_t cy, float r, uint16_t col)
{
    int i;
    int32_t px = cx + (int32_t)r, py = cy;
    for (i = 1; i <= 48; i++) {
        const float a = (float)i * (FM_TWO_PI / 48.0f);
        const int32_t x = cx + (int32_t)(fm_cosf(a) * r), y = cy + (int32_t)(fm_sinf(a) * r);
        cv_line(px, py, x, y, col);
        px = x, py = y;
    }
}

/* VORTEX and TUNNEL (1.0.2): two more that need no state of their own. VORTEX is a spiral of arms that turns with the tempo and
 * breathes on the kick; TUNNEL is rings rushing towards you, faster with the energy, the kick pushing them out. */
static void vz_vortex(void)
{
    const int arms = 3;
    const float cx = 120.0f, cy = (float)vz.h * 0.5f, sy = (float)vz.h * (1.0f / 144.0f), rmax = 130.0f + vz.kick * 14.0f;
    int a, i;
    for (a = 0; a < arms; a++) {
        int32_t px = 0, py = 0;
        for (i = 0; i <= 40; i++) {
            const float f = (float)i * (1.0f / 40.0f), th = vz.tt * 0.35f + (float)a * (FM_TWO_PI / (float)arms) + f * (5.0f + vz.snare * 1.5f);
            const float r = f * rmax;
            const int32_t x = (int32_t)(cx + fm_cosf(th) * r), y = (int32_t)(cy + fm_sinf(th) * r * sy * 0.8f);
            if (i)
                cv_line(px, py, x, y, vpal(f * 0.6f + vz.ph * 3.0f + (float)a * 0.33f));
            px = x, py = y;
        }
    }
    vz_ring((int32_t)cx, (int32_t)cy, 5.0f + vz.kick * 10.0f, K_TEXT);
}
static void vz_tunnel(void)
{
    const float cx = 120.0f, cy = (float)vz.h * 0.5f;
    int i;
    for (i = 0; i < 12; i++) {
        float f = (float)i * (1.0f / 12.0f) + vz.ph2 * 0.35f;
        const float u = f - fm_floorf(f), r = 4.0f + u * u * (150.0f + vz.kick * 30.0f);
        vz_ring((int32_t)(cx + fm_sinf(vz.tt * 0.4f) * 12.0f * (1.0f - u)), (int32_t)(cy + fm_cosf(vz.tt * 0.3f) * 6.0f * (1.0f - u)), r, vpal(u + vz.ph * 2.0f));
    }
}

/* DARK-1 1.0.3: six more. STARS, RAIN, STORM, RIPPLES and RADAR keep no state (every frame is drawn from the clock, the beat and a hash), WATERFALL
 * keeps the last rows of the spectrum. */
static float vz_h(uint32_t i)
{
    i = (i ^ 61u) ^ (i >> 16);
    i *= 9u;
    i ^= i >> 4;
    i *= 0x27d4eb2du;
    i ^= i >> 15;
    return (float)(i & 0xFFFFu) * (1.0f / 65536.0f);
}
static void vz_stars(void)                       /* a field of stars flying out of the middle; the kick pushes them */
{
    const int32_t cx = VZ_W / 2, cy = vz.h / 2;
    const float R = vz.full ? 190.0f : 135.0f;
    int i;
    for (i = 0; i < 90; i++) {
        const float zr = vz.tt * 0.11f + (float)i * (1.0f / 90.0f);
        const uint32_t ep = (uint32_t)zr, id = (uint32_t)i + 97u * ep;
        const float z = zr - fm_floorf(zr), a = vz_h(id * 3u + 1u) * FM_TWO_PI, k = 0.35f + vz_h(id * 3u + 2u) * 0.9f;
        const float r = z * z * R * k * (1.0f + vz.kick * 0.12f), r0 = fm_maxf(z - 0.04f, 0.0f), q = r0 * r0 * R * k;
        const int32_t x = cx + (int32_t)(fm_cosf(a) * r), y = cy + (int32_t)(fm_sinf(a) * r * 0.85f);
        const uint16_t col = z < 0.1f ? K_DIM : vpal(vz_h(id * 3u) + vz.ph);
        if (!vz_vis(y - 6, 12))
            continue;
        if (z > 0.3f)
            cv_line(cx + (int32_t)(fm_cosf(a) * q), cy + (int32_t)(fm_sinf(a) * q * 0.85f), x, y, col);
        cv_rect(x, y, z > 0.6f ? 3 : 2, z > 0.6f ? 3 : 2, z > 0.85f ? K_TEXT : col);
    }
}
static void vz_rain(void)                        /* rain: a drop of the theme's colour a column, the hats make more of it */
{
    const int cols = VZ_W / 6;
    int c, d;
    for (c = 0; c < cols; c++)
        for (d = 0; d < 2; d++) {
            const uint32_t id = (uint32_t)c * 2u + (uint32_t)d;
            const float sp = 0.5f + vz_h(id + 500u) * 1.1f + vz.hat * 0.4f, ph = vz_h(id + 900u);
            const float u = vz.tt * sp * 0.22f + ph;
            const int32_t y = (int32_t)((u - fm_floorf(u)) * ((float)vz.h + 48.0f)) - 36;
            int j;
            if (d && vz_h(id) > 0.35f + vz.hat * 0.5f)
                continue;
            for (j = 0; j < 6; j++)
                if (vz_vis(y - j * 6, 6) && y - j * 6 > -6 && y - j * 6 < vz.h)
                    cv_rect(c * 6, y - j * 6, 3, 6, j == 0 ? K_TEXT : mix(K_BG, vpal((float)c / (float)cols + vz.ph), 16 - j * 3));
        }
}
static void vz_storm(void)                       /* a bolt of lightning on the snare and kick (and now and then when it is quiet), over the rain */
{
    float lit = fm_maxf(vz.snare, vz.kick * 0.9f);
    uint32_t id = en_bars;
    int32_t x, y = 0;
    int k;
    if (!en_playing) {
        const float q = vz.tt * 0.09f;
        lit = (q - fm_floorf(q)) < 0.06f ? 1.0f : 0.0f;
        id = (uint32_t)q;
    }
    {   /* a thin rain behind it */
        int c;
        for (c = 0; c < VZ_W / 12; c++) {
            const float u = vz.tt * (0.6f + vz_h((uint32_t)c + 40u)) * 0.3f + vz_h((uint32_t)c + 80u);
            const int32_t yy = (int32_t)((u - fm_floorf(u)) * (float)vz.h);
            if (vz_vis(yy, 8))
                cv_rect(c * 12 + 3, yy, 1, 8, mix(K_BG, K_PAD_T, 8));
        }
    }
    if (lit < 0.12f)
        return;
    x = 40 + (int32_t)(vz_h(id * 5u + 1u) * 160.0f);
    for (k = 0; y < vz.h; k++) {
        const int32_t nx = x + (int32_t)((vz_h(id * 31u + (uint32_t)k) - 0.5f) * 36.0f), ny = y + 10 + (int32_t)(vz_h(id * 17u + (uint32_t)k) * 10.0f);
        const uint16_t col = lit > 0.5f ? K_TEXT : vpal(vz.ph + 0.3f);
        cv_line(x, y, nx, ny, col);
        cv_line(x + 1, y, nx + 1, ny, col);
        if (k % 3 == 2 && vz_h(id * 7u + (uint32_t)k) > 0.5f)
            cv_line(nx, ny, nx + (vz_h(id + (uint32_t)k) > 0.5f ? 22 : -22), ny + 18, mix(K_BG, col, 10));
        x = nx, y = ny;
    }
}
static void vz_ripples(void)                     /* rings spreading from places on the screen, the kick leans on them */
{
    int k;
    for (k = 0; k < 6; k++) {
        const float t = vz.tt * 0.16f + (float)k * (1.0f / 6.0f), age = t - fm_floorf(t);
        const uint32_t ep = (uint32_t)t, id = (uint32_t)k * 13u + ep * 7u;
        const int32_t cx = 30 + (int32_t)(vz_h(id + 1u) * 180.0f), cy = 20 + (int32_t)(vz_h(id + 2u) * ((float)vz.h - 40.0f));
        const float r = age * ((float)vz.h * 0.85f + vz.kick * 12.0f);
        const uint16_t col = mix(vpal(vz_h(id + 3u) + vz.ph), K_BG, (int)(age * 15.0f));
        vz_ring(cx, cy, r + 1.0f, col);
        vz_ring(cx, cy, r, col);
        if (age < 0.5f)
            vz_ring(cx, cy, r * 0.6f, mix(col, K_BG, 8));
    }
}
static void vz_radar(void)                       /* a radar: the sweep, and the blips it lights, the kick pings a ring */
{
    const int32_t cx = VZ_W / 2, cy = vz.h / 2, R = vz.h / 2 - 6;
    const float a = vz.tt * 1.1f;
    int i, j;
    vz_ring(cx, cy, (float)R, K_LINE);
    vz_ring(cx, cy, (float)R * 0.66f, K_LINE);
    vz_ring(cx, cy, (float)R * 0.33f, K_LINE);
    cv_line(cx - R, cy, cx + R, cy, K_LINE);
    cv_line(cx, cy - R, cx, cy + R, K_LINE);
    if (vz.kick > 0.15f)
        vz_ring(cx, cy, (float)R * (1.0f - vz.kick * 0.5f), mix(K_BG, K_ACC, (int)(vz.kick * 14.0f)));
    for (i = 0; i < 18; i++) {                   /* the blips: bright when the sweep has just passed */
        const uint32_t ep = (uint32_t)(a * (1.0f / FM_TWO_PI) * 0.25f), id = (uint32_t)i + 31u * ep;
        const float ab = vz_h(id * 2u) * FM_TWO_PI, rb = 0.2f + 0.75f * vz_h(id * 2u + 1u);
        float d = (a - ab) * (1.0f / FM_TWO_PI);
        const int32_t x = cx + (int32_t)(fm_cosf(ab) * rb * (float)R), y = cy + (int32_t)(fm_sinf(ab) * rb * (float)R);
        d = d - fm_floorf(d);
        if (d < 0.7f && vz_vis(y - 3, 8))
            cv_rect(x - 2, y - 2, 5, 5, mix(vpal(vz_h(id) + vz.ph), K_BG, (int)(d * 22.0f)));
    }
    for (j = 0; j < 14; j++) {                   /* the sweep and its fading trail */
        const float aa = a - (float)j * 0.06f;
        cv_line(cx, cy, cx + (int32_t)(fm_cosf(aa) * (float)R), cy + (int32_t)(fm_sinf(aa) * (float)R), j == 0 ? K_TEXT : mix(K_BG, K_PAD, 16 - j));
    }
}
#define WF_N 24
static void vz_waterfall(void)                   /* the spectrum's last moments, scrolling down: each row a moment, each column a band */
{
    const int rows = vz.h / 6;
    uint8_t (*w)[60] = vz.buf.fb[0];
    const uint32_t now = plat_ms();
    int r, i;
    if (!vz.pass && now - vz.life_ms >= 60u) {
        vz.life_ms = now;
        en_spectrum(vz.spec, WF_N);
        for (r = rows - 1; r > 0; r--)
            for (i = 0; i < WF_N; i++)
                w[r][i] = w[r - 1][i];
        for (i = 0; i < WF_N; i++)
            w[0][i] = (uint8_t)(fm_sqrtf(fm_clampf(vz.spec[i] * 1.4f, 0.0f, 1.0f)) * 255.0f);   /* (the square root: the quiet bands show) */
    }
    for (r = 0; r < rows; r++) {
        if (!vz_vis(r * 6, 6))
            continue;
        for (i = 0; i < WF_N; i++) {
            const int v = w[r][i];
            if (v > 14)
                cv_rect(i * 10, r * 6, 10, 6, mix(K_BG, vpal((float)v * 0.0028f + (float)i * 0.012f + vz.ph), v * 16 / 255 + 1 > 16 ? 16 : v * 16 / 255 + 1));
        }
    }
}
/* DARK-1 1.0.5: three made for dark techno. GRID is a dark floor of lines rushing at you under a horizon that flares on the kick; PULSE is square
 * frames thrown out of the middle, thumping with the kick and ticked by the hats; SCAN is laser beams sweeping from above, all of them lit by the
 * snare. They keep no state: each frame is drawn from the clock and the beat. */
static void vz_grid(void)
{
    const int32_t cx = VZ_W / 2, hy = vz.h * 36 / 100, span = vz.h - hy;
    const uint16_t dim = mix(K_BG, K_PAD_T, 9), mid = mix(K_BG, K_PAD, 11);
    int i;
    for (i = -9; i <= 9; i++)
        cv_line(cx, hy, cx + i * 38, vz.h, vz.kick > 0.5f ? mid : dim);
    for (i = 0; i < 9; i++) {
        float f = (float)i * (1.0f / 9.0f) + vz.ph2 * 0.22f;
        const float u = f - fm_floorf(f), d = u * u;
        const int32_t y = hy + (int32_t)(d * (float)span);
        cv_rect(0, y, VZ_W, 1, u > 0.8f ? mid : u > 0.35f ? dim : mix(K_BG, K_PAD_T, 5));
    }
    {   /* the horizon: a bar that grows with the energy and flares on the kick */
        const int32_t w = 20 + (int32_t)(vz.en * 70.0f) + (int32_t)(vz.kick * 40.0f);
        cv_rect(cx - w, hy - 1, 2 * w, 2, vz.kick > 0.3f ? K_ACC : mid);
        if (vz.kick > 0.3f)
            cv_rect(cx - w / 2, hy - 4, w, 2, mix(K_BG, K_ACC, 8));
    }
}
static void vz_pulse(void)
{
    const int32_t cx = VZ_W / 2, cy = vz.h / 2;
    int i;
    for (i = 0; i < 10; i++) {
        float f = (float)i * 0.1f + vz.ph2 * 0.3f;
        const float u = f - fm_floorf(f);
        const int32_t s = 4 + (int32_t)(u * u * (150.0f + vz.kick * 24.0f)), t = s * 2 / 3;
        const uint16_t c = u < 0.12f ? K_ACC : vpal(u + vz.ph * 2.0f);
        cv_rect(cx - s, cy - t, 2 * s, 1, c);
        cv_rect(cx - s, cy + t, 2 * s, 1, c);
        cv_rect(cx - s, cy - t, 1, 2 * t, c);
        cv_rect(cx + s, cy - t, 1, 2 * t, c);
    }
    for (i = 0; i < 6; i++)                      /* the hats tick the corners */
        if (vz.hat > 0.3f)
            cv_rect((int32_t)(vz_h((uint32_t)i * 7u + (uint32_t)(vz.tt * 8.0f)) * (float)VZ_W), (int32_t)(vz_h((uint32_t)i * 13u + (uint32_t)(vz.tt * 8.0f)) * (float)vz.h), 2, 2, K_TEXT);
}
static void vz_scan(void)
{
    const int32_t cx = VZ_W / 2;
    int i;
    for (i = 0; i < 7; i++) {
        const float a = fm_sinf(vz.tt * (0.35f + (float)i * 0.11f) + (float)i * 1.7f) * (0.55f + vz.kick * 0.25f);
        const int32_t x = cx + (int32_t)(fm_sinf(a) * (float)vz.h * 1.6f), y = (int32_t)(fm_cosf(a) * (float)vz.h * 1.2f);
        const uint16_t c = vz.snare > 0.3f ? K_ACC : vpal((float)i * 0.15f + vz.ph * 3.0f);
        cv_line(cx, -4, x, y > vz.h ? vz.h : y, c);
        if (vz.snare > 0.3f)
            cv_line(cx + 1, -4, x + 1, y > vz.h ? vz.h : y, mix(K_BG, K_ACC, 9));
    }
    cv_rect(cx - 4, 0, 9, 3, vz.en > 0.2f ? K_ACC : K_DIM);   /* the source */
}

static uint8_t vz_home;                         /* the home page shows a visualizer: the chord's name over it */
static void vz_home_overlay(void)
{
    char nm[16];
    int pcs[EN_NPAD + 1], k;
    int32_t w;
    pcs[0] = en_chord_root;
    for (k = 0; k < EN_NPAD; k++)
        pcs[k + 1] = en_chord[k] % 12;
    en_chord_name(pcs, EN_NPAD + 1, nm);
    w = text_w(&FONT_B, nm);
    cv_rect(VZ_W / 2 - w / 2 - 5, 4, w + 10, 20, K_BG);
    text_c(VZ_W / 2, 6, &FONT_B, nm, en_frozen ? K_FROST : K_TEXT);
    if (en_frozen) {
        cv_text(VZ_W - 8 - text_w(&FONT_XS, "held"), VZ_H - 14, &FONT_XS, "held", K_FROST);
    } else {
        const int32_t bw = 30, f = (int32_t)(en_drift * (float)bw + 0.5f);
        cv_rect(VZ_W - 8 - bw, VZ_H - 10, bw, 4, K_LINE);
        if (f > 0)
            cv_rect(VZ_W - 8 - bw, VZ_H - 10, f, 4, K_PAD);
    }
}
static void draw_viz(void)
{
    int mode = ui.viz % VZ_N;
    const int full = ui.saver_on;                  /* the screensaver: the whole screen, in two bands of 120 rows */
    const int32_t h = full ? 240 : VZ_H;
    int band;
    if (!vz.inited) {
        vz.inited = 1;
        vz.seed = 0x9E3779B9u ^ plat_ms();
        vz.last_ms = plat_ms();
        vz.cf[0] = 1.0f, vz.cf[1] = 1.3f, vz.cf[2] = 1.6f;
        vz.mode_seen = 255;
        vz.sec_seen = en_sec;
    }
    if (vz.h != h) {                               /* the page and the whole screen differ in size: the state starts fresh */
        vz.h = h;
        vz.mode_seen = 255;
    }
    vz.full = (uint8_t)full;
    vz_signals();
    if (mode == VZ_AUTO) {                         /* a new one on a section change and every 32 bars */
        if (en_sec != vz.sec_seen || en_bars - vz.auto_bar >= 512u) {
            vz.sec_seen = en_sec;
            vz.auto_bar = en_bars;
            vz.auto_cur = (uint8_t)((vz.auto_cur + 1u) % (VZ_N - 1));
        }
        mode = vz.auto_cur;
    }
    if (mode != vz.mode_seen) {                    /* a new one: its state fresh */
        vz.mode_seen = (uint8_t)mode;
        if (mode == VZ_WATER)
            memset(vz.buf.fb[0], 0, sizeof vz.buf.fb[0]);
    }
    vz_lut();
    for (band = 0; band < (full ? 2 : 1); band++) {
        vz.pass = (uint8_t)band;
        vz.y0 = band * 120;
        if (full) {
            cv_begin(240, 120, K_BG);
            cv_oy = -vz.y0;
        } else {
            cv_begin(VZ_W, VZ_H, K_BG);
        }
        switch (mode) {
        case VZ_BARS: vz_bars(); break;
        case VZ_VORTEX: vz_vortex(); break;
        case VZ_TUNNEL: vz_tunnel(); break;
        case VZ_STARS: vz_stars(); break;
        case VZ_RAIN: vz_rain(); break;
        case VZ_STORM: vz_storm(); break;
        case VZ_RIPPLES: vz_ripples(); break;
        case VZ_RADAR: vz_radar(); break;
        case VZ_WATER: vz_waterfall(); break;
        case VZ_GRID: vz_grid(); break;
        case VZ_PULSE: vz_pulse(); break;
        case VZ_SCAN: vz_scan(); break;
        default: vz_tunnel(); break;
        }
        if (vz_home && !full)
            vz_home_overlay();
        cv_blit(0, full ? (uint32_t)vz.y0 : MAIN_Y);
    }
    vz.pass = 0;
}
