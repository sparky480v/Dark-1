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
    const uint16_t stop[6] = {K_PAD_T, K_PAD, K_ACID, K_TEXT, K_ACC, K_PAD_T};
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

static void vz_plasma(void)
{
    const float t = vz.tt;
    const int rows = VZ_ROWS;
    int cx, cy;
    for (cy = 0; cy < rows; cy++) {
        if (!vz_vis(cy * 6, 6))
            continue;
        for (cx = 0; cx < VZ_CW; cx++) {
            const float x = (float)cx, y = (float)cy, dx = x - 20.0f, dy = (y - (float)rows * 0.5f) * 1.6f;
            float v = fm_sinf(x * 0.31f + t * 1.1f) + fm_sinf(y * 0.43f - t * 0.9f) + fm_sinf((x + y) * 0.21f + t * 0.7f) +
                      fm_sinf(fm_sqrtf(dx * dx + dy * dy) * 0.45f - t * 1.6f);
            cv_rect(cx * 6, cy * 6, 6, 6, vpal(v * 0.125f + 0.5f + vz.ph + vz.kick * 0.18f));
        }
    }
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

/* FLUX: the classic media-player light show. A low-resolution picture (60 x 36 cells) is fed back on itself every frame, zoomed out
 * and turned a little, and fading; the spectrum is drawn into it as a ring around the middle and a ribbon across it, and the kick adds a
 * ripple. Colour comes from how bright a cell still is and where it is, so the trails bloom outwards in the theme's colours. */
#define FL_W 60
#define FL_H (vz.h / 4)                        /* 36 on the page, 60 whole screen */
static void fl_plot(uint8_t (*b)[FL_W], int x, int y, int v)
{
    if ((unsigned)x < FL_W && (unsigned)y < (unsigned)FL_H && b[y][x] < v)
        b[y][x] = (uint8_t)v;
}
static void fl_line(uint8_t (*b)[FL_W], int x0, int y0, int x1, int y1, int v)
{
    const int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0, sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, guard = 200;
    while (guard--) {
        const int e2 = 2 * err;
        fl_plot(b, x0, y0, v);
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}
static void vz_flux_init(void)
{
    memset(vz.buf.fb, 0, sizeof vz.buf.fb);
    vz.fcur = 0;
}
static void vz_flux(void)
{
    uint8_t (*src)[FL_W] = vz.buf.fb[vz.fcur], (*dst)[FL_W] = vz.buf.fb[vz.fcur ^ 1u];
    const float mx = (float)FL_W * 0.5f, my = (float)FL_H * 0.5f;
    int x, y, k;
    if (!vz.pass) {
        const float ang = 0.05f + 0.035f * fm_sinf(vz.tt * 0.4f) + vz.kick * 0.05f, ca = fm_cosf(ang), sa = fm_sinf(ang),
                    zoom = 0.955f - 0.025f * vz.kick;
        for (y = 0; y < FL_H; y++)
            for (x = 0; x < FL_W; x++) {
                const float dx = (float)x + 0.5f - mx, dy = (float)y + 0.5f - my;
                const float fx = mx - 0.5f + (dx * ca - dy * sa) * zoom, fy = my - 0.5f + (dx * sa + dy * ca) * zoom;   /* where it was a frame ago (cell centres) */
                const int ix = (int)fm_floorf(fx), iy = (int)fm_floorf(fy), tx = (int)((fx - (float)ix) * 16.0f), ty = (int)((fy - (float)iy) * 16.0f);
                int v = 0;
                if (ix >= 0 && iy >= 0 && ix < FL_W - 1 && iy < FL_H - 1) {      /* smooth: the four cells round it */
                    const int a = src[iy][ix] * (16 - tx) + src[iy][ix + 1] * tx, b = src[iy + 1][ix] * (16 - tx) + src[iy + 1][ix + 1] * tx;
                    v = (a * (16 - ty) + b * ty) >> 8;
                }
                dst[y][x] = (uint8_t)((v * 237) >> 8);
            }
        vz.fcur ^= 1u;
        en_spectrum(vz.spec, 24);
        {   /* the ring: the spectrum round the middle, turning */
            int px = 0, py = 0;
            const float rs = (float)FL_H * (1.0f / 36.0f);
            for (k = 0; k <= 24; k++) {
                const float s = vz.spec[k % 24], a = (float)k * (FM_TWO_PI / 24.0f) + vz.tt * 0.35f, r = (4.0f + s * 13.0f + (en_playing ? 0.0f : 1.0f + fm_sinf(vz.tt) * 0.8f)) * rs;
                const int cx = (int)(mx + fm_cosf(a) * r), cy = (int)(my + fm_sinf(a) * r * 0.95f);
                if (k)
                    fl_line(dst, px, py, cx, cy, 255);
                px = cx, py = cy;
            }
        }
        {   /* the ribbon: a wave across, as tall as the spectrum under it */
            int px = 0, py = (int)my;
            const float rs = (float)FL_H * (1.0f / 36.0f);
            for (x = 0; x < FL_W; x += 2) {
                const float u = (float)x * (24.0f / (float)FL_W), s = vz.spec[(int)u > 23 ? 23 : (int)u];
                const int yy = (int)(my + fm_sinf((float)x * 0.28f + vz.tt * 2.2f) * (1.5f + s * 12.0f) * rs);
                if (x)
                    fl_line(dst, px, py, x, yy, 190);
                px = x, py = yy;
            }
        }
        if (vz.kick > 0.6f)                              /* a kick: a ripple */
            for (k = 0; k < 24; k++)
                fl_plot(dst, (int)(mx + fm_cosf((float)k * (FM_TWO_PI / 24.0f)) * 6.0f), (int)(my + fm_sinf((float)k * (FM_TWO_PI / 24.0f)) * 6.0f), 255);
    }
    dst = vz.buf.fb[vz.fcur];                            /* (the one just made) */
    for (y = 0; y < FL_H; y++) {
        if (!vz_vis(y * 4, 4))
            continue;
        for (x = 0; x < FL_W; x++) {
            const int v = dst[y][x];
            if (v > 10) {
                const float dx = (float)x - mx, dy = (float)y - my;
                cv_rect(x * 4, y * 4, 4, 4, mix(K_BG, vpal(vz.ph * 2.0f + (dx * dx + dy * dy) * 0.0009f + (float)v * 0.0018f), (v + 8) >> 4));
            }
        }
    }
}

/* KALEIDO: the other classic: curves swept out by the spectrum (a rose whose petals are the bands, and a ring of the highs), drawn into the
 * same feedback picture four times, mirrored left-right and top-bottom, while the picture slowly turns and spreads and fades: ribbons that
 * unfold from the middle. The kick throws a ring. */
static void vz_kaleido(void)
{
    uint8_t (*src)[FL_W] = vz.buf.fb[vz.fcur], (*dst)[FL_W] = vz.buf.fb[vz.fcur ^ 1u];
    const float mx = (float)FL_W * 0.5f, my = (float)FL_H * 0.5f;
    int x, y, k;
    if (!vz.pass) {
        const float ang = 0.035f + 0.025f * fm_sinf(vz.tt * 0.23f), ca = fm_cosf(ang), sa = fm_sinf(ang), zoom = 0.965f - 0.02f * vz.kick;
        const float rs = (float)FL_H * (1.0f / 36.0f);
        for (y = 0; y < FL_H; y++)
            for (x = 0; x < FL_W; x++) {
                const float dx = (float)x + 0.5f - mx, dy = (float)y + 0.5f - my;
                const float fx = mx - 0.5f + (dx * ca - dy * sa) * zoom, fy = my - 0.5f + (dx * sa + dy * ca) * zoom;
                const int ix = (int)fm_floorf(fx), iy = (int)fm_floorf(fy), tx = (int)((fx - (float)ix) * 16.0f), ty = (int)((fy - (float)iy) * 16.0f);
                int v = 0;
                if (ix >= 0 && iy >= 0 && ix < FL_W - 1 && iy < FL_H - 1) {
                    const int a = src[iy][ix] * (16 - tx) + src[iy][ix + 1] * tx, b = src[iy + 1][ix] * (16 - tx) + src[iy + 1][ix + 1] * tx;
                    v = (a * (16 - ty) + b * ty) >> 8;
                }
                dst[y][x] = (uint8_t)((v * 218) >> 8);
            }
        vz.fcur ^= 1u;
        en_spectrum(vz.spec, 24);
        {   /* the rose, mirrored four ways */
            const int N = 72;
            int px = 0, py = 0;
            for (k = 0; k <= N; k++) {
                const float u = (float)k * (1.0f / (float)N), s = vz.spec[(int)(u * 23.99f)], th = u * (FM_TWO_PI * 2.0f) + vz.tt * 0.4f;
                const float r = (6.0f + (6.0f + 16.0f * s) * (0.4f + 0.6f * fm_sinf(th * 2.5f + vz.tt * 0.9f))) * rs,
                            qx = fm_cosf(th) * r, qy = fm_sinf(th) * r * 0.9f;
                const int ix = (int)(mx + qx), iy = (int)(my + qy);
                if (k) {
                    fl_line(dst, px, py, ix + 1, iy, 170);
                    fl_line(dst, px, py, ix, iy, 255);
                    fl_line(dst, FL_W - 1 - px, py, FL_W - 1 - ix, iy, 255);
                    fl_line(dst, px, FL_H - 1 - py, ix, FL_H - 1 - iy, 255);
                    fl_line(dst, FL_W - 1 - px, FL_H - 1 - py, FL_W - 1 - ix, FL_H - 1 - iy, 255);
                }
                px = ix, py = iy;
            }
        }
        {   /* the highs: a small ring that breathes with the hats */
            int px = 0, py = 0;
            const float rr = (5.0f + 9.0f * vz.hat + 4.0f * vz.spec[18]) * (float)FL_H * (1.0f / 36.0f);
            for (k = 0; k <= 12; k++) {
                const float a = (float)k * (FM_TWO_PI / 12.0f) - vz.tt * 0.8f;
                const int ix = (int)(mx + fm_cosf(a) * rr), iy = (int)(my + fm_sinf(a) * rr);
                if (k)
                    fl_line(dst, px, py, ix, iy, 210);
                px = ix, py = iy;
            }
        }
        if (vz.kick > 0.6f)                              /* a kick: a ring */
            for (k = 0; k < 32; k++)
                fl_plot(dst, (int)(mx + fm_cosf((float)k * (FM_TWO_PI / 32.0f)) * 9.0f * (float)FL_H * (1.0f / 36.0f)),
                        (int)(my + fm_sinf((float)k * (FM_TWO_PI / 32.0f)) * 9.0f * (float)FL_H * (1.0f / 36.0f)), 255);
    }
    dst = vz.buf.fb[vz.fcur];
    for (y = 0; y < FL_H; y++) {
        if (!vz_vis(y * 4, 4))
            continue;
        for (x = 0; x < FL_W; x++) {
            const int v = dst[y][x];
            if (v > 10) {
                const float dx = (float)x - mx, dy = (float)y - my;
                cv_rect(x * 4, y * 4, 4, 4, mix(K_BG, vpal(vz.ph * 2.5f + fm_sqrtf(dx * dx + dy * dy) * 0.07f + (float)v * 0.0016f), (v + 8) >> 4));
            }
        }
    }
}

/* THE EYE (SIGIL, the fifth visualizer): a ritual circle with a turning pentagram and a demon eye in the middle. The sixteen dots
 * round it are the drum pattern's steps, the playing one lit. The eye is half open in a groove, its slit pupil widening on the kick and
 * its gaze flipping on the snare; shut in a breakdown; wide and trembling in a build; wide open with a shock ring on the drop. */
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

static void vz_eye(void)
{
    const float k = vz.full ? 1.75f : 1.0f, R = 46.0f * k;
    const int32_t cx = vz.full ? 104 : 84, cy = vz.floor_y - (int32_t)(54.0f * k);
    const int playing = en_playing, kk = (int)(vz.kick * 5.0f);
    static float gz;                                   /* the gaze (kept for the second band of the whole screen) */
    const char *label = !playing ? "IDLE" : en_sec == EN_SEC_BREAK ? "ASLEEP" : en_sec == EN_SEC_BUILD ? "WIDE" : "WATCHING";
    int i;
    float open, hmax, w, rr = R * (1.0f + 0.05f * vz.kick), gx, ir, pw;
    int32_t x;
    if (!vz.pass) {
        const int sedge = vz.snare > 0.6f && vz.sprev <= 0.6f;
        float tg;
        const uint32_t now = plat_ms();
        vz.sprev = vz.snare;
        if (en_sec != vz.dsec) {
            if (en_sec == EN_SEC_GROOVE && vz.dsec != EN_SEC_GROOVE)
                vz.drop_bar = en_bars;                 /* the drop */
            vz.dsec = en_sec;
        }
        if (sedge)
            vz.dpos ^= 1u;
        if (now >= vz.blink_next) {
            vz.blink_until = now + 140u;
            vz.blink_next = now + 2200u + vz_rnd() % 3800u;
        }
        tg = !playing ? 0.3f : en_sec == EN_SEC_BREAK ? 0.04f : en_sec == EN_SEC_BUILD ? 0.95f + (vz_rf() - 0.5f) * 0.12f :
             en_bars - vz.drop_bar < 16u ? 1.0f : 0.5f + 0.3f * vz.kick;
        if (now < vz.blink_until && en_sec != EN_SEC_BREAK)
            tg = 0.04f;
        vz.dh += (tg - vz.dh) * fm_minf(vz.dt * 12.0f, 1.0f);
        vz.dhv += ((1.0f + 3.5f * vz.kick + (en_sec == EN_SEC_BUILD ? 2.0f : 0.0f)) - vz.dhv) * fm_minf(vz.dt * 14.0f, 1.0f);
        gz += ((vz.dpos ? 0.14f : -0.14f) - gz) * fm_minf(vz.dt * 6.0f, 1.0f);
    }
    open = fm_clampf(vz.dh, 0.0f, 1.0f);
    hmax = 0.42f * R * open;
    w = 0.66f * R;
    gx = gz * R;
    ir = 0.34f * R;
    pw = vz.dhv * 0.5f * k;
    vz_ring(cx, cy, rr, mix(K_BG, vpal(vz.ph * 2.0f), 9 + kk));
    vz_ring(cx, cy, rr * 0.9f, mix(K_BG, K_LINE, 12));
    {   /* the pentagram, turning (the energy turns it faster) */
        int32_t sx[5], sy[5];
        const float rot = vz.tt * 0.25f;
        for (i = 0; i < 5; i++) {
            const float a = -FM_PI * 0.5f + rot + (float)i * (FM_TWO_PI / 5.0f);
            sx[i] = cx + (int32_t)(fm_cosf(a) * rr * 0.9f), sy[i] = cy + (int32_t)(fm_sinf(a) * rr * 0.9f);
        }
        for (i = 0; i < 5; i++) {
            const uint16_t col = mix(K_BG, vpal(vz.ph * 3.0f + (float)i * 0.07f), 7 + kk);
            cv_line(sx[i], sy[i], sx[(i + 2) % 5], sy[(i + 2) % 5], col);
            cv_line(sx[i] + 1, sy[i], sx[(i + 2) % 5] + 1, sy[(i + 2) % 5], col);
        }
    }
    for (i = 0; i < 16; i++) {                         /* the pattern's sixteen steps round the circle */
        const float a = -FM_PI * 0.5f + (float)i * (FM_TWO_PI / 16.0f);
        const int on = playing && i == (int)en_drum_pos;
        cv_rect(cx + (int32_t)(fm_cosf(a) * R * 1.14f) - 1, cy + (int32_t)(fm_sinf(a) * R * 1.14f) - 1, on ? 4 : 2, on ? 4 : 2, on ? K_TEXT : (i & 3) ? K_LINE : K_DIM);
    }
    {   /* the drop's shock ring */
        const uint32_t age = en_bars - vz.drop_bar;
        if (playing && vz.drop_bar && age < 16u)
            vz_ring(cx, cy, R * (1.0f + (float)age * 0.09f), mix(K_BG, K_TEXT, 16 - (int)age));
    }
    for (x = -(int32_t)w; x <= (int32_t)w; x++) {     /* the eye: an almond, the iris and its slit pupil */
        const float u = (float)x / w, hh = hmax * (1.0f - u * u), dxg = (float)x - gx;
        const int32_t h = (int32_t)hh;
        if (hh < 0.6f) {
            cv_rect(cx + x, cy, 1, 1, K_TEXT);
            continue;
        }
        cv_rect(cx + x, cy - h, 1, 2 * h + 1, mix(K_BG, K_TEXT, 13));
        if (fm_fabsf(dxg) < ir) {
            const float ih = fm_minf(hh, fm_sqrtf(ir * ir - dxg * dxg) * 1.15f);
            cv_rect(cx + x, cy - (int32_t)ih, 1, 2 * (int32_t)ih + 1, vpal(vz.ph * 2.0f + 0.1f));
            if (fm_fabsf(dxg) < pw)
                cv_rect(cx + x, cy - (int32_t)ih, 1, 2 * (int32_t)ih + 1, K_BG);
        }
        cv_rect(cx + x, cy - h - 1, 1, 1, K_TEXT);
        cv_rect(cx + x, cy + h + 1, 1, 1, K_TEXT);
    }
    cv_text(172, 76, &FONT_XS, label, K_DIM);          /* the eye's mood, under the pattern */
}

static void vz_disco(void)
{
    int i, l, s;
    const int kk = (int)(vz.kick * 5.0f), step4 = en_playing ? (int)(en_drum_pos >> 1) : 0;
    const int frows = vz.full ? 4 : 2;                         /* the floor's rows of tiles */
    const int32_t flo = vz.h - frows * 11 - 1, bx = vz.full ? 104 : 84, by = vz.full ? 22 : 13;
    const float hs = (float)vz.h * (1.0f / 144.0f);
    vz.floor_y = flo - 1;
    {   /* the back wall, breathing with the kick; a disco floor below it, tiles chasing along with the pattern */
        int c, r;
        cv_rect(0, 0, 240, flo - 1, mix(K_BG, vpal(vz.ph * 2.0f), 1 + kk / 2));
        for (r = 0; r < frows; r++)
            for (c = 0; c < 12; c++) {
                const int lit = ((c + r * 3 + step4) & 3) == 0 && en_playing;
                const uint16_t col = lit ? mix(K_PANEL, vpal(vz.ph * 2.0f + (float)c * 0.07f + (float)r * 0.12f), 9 + kk) :
                                           mix(K_BG, K_PANEL, ((c + r) & 1) ? 12 : 7);
                cv_rect(c * 20, flo + 2 + r * 11, 19, 10, col);
            }
    }
    {   /* the beams from the ball, sweeping; their spots where they meet the floor */
        int k;
        for (k = 0; k < 6; k++) {
            const float a = -1.15f + (float)k * 0.46f + fm_sinf(vz.tt * 0.7f + (float)k * 1.3f) * 0.3f;
            const uint16_t col = mix(K_BG, vpal(vz.ph * 3.0f + (float)k * 0.17f), 5 + kk);
            const int32_t ex = bx + (int32_t)(fm_sinf(a) * 140.0f * hs), ey = by + (int32_t)(fm_cosf(a) * 140.0f * hs);
            cv_line(bx, by, ex, ey, col);
            cv_line(bx + 1, by, ex + 1, ey, col);
            cv_line(bx - 1, by, ex - 1, ey, col);
            if (a > -1.3f && a < 1.3f && fm_cosf(a) > 0.2f) {
                const int32_t sx = bx + (int32_t)(fm_sinf(a) / fm_cosf(a) * (float)(flo + 1 - by));
                cv_disc(sx, flo + 1, 4, mix(K_BG, vpal(vz.ph * 3.0f + (float)k * 0.17f), 8 + kk));
            }
        }
    }
    for (i = 0; i < 12; i++) {                     /* sparkles on the wall */
        const uint32_t h = hash(2166136261u, (uint32_t)i * 977u + (plat_ms() >> 8));
        cv_rect((int32_t)(h % 164u), (int32_t)((h >> 8) % (uint32_t)(flo - 20)), 2, 2, (h >> 20 & 1u) ? K_TEXT : vpal((float)i * 0.07f));
    }
    cv_line(bx, 0, bx, by - 8, K_LINE);            /* the ball: a rope, a sphere of mirrors turning */
    cv_disc(bx, by, 9, K_LINE);
    for (i = 0; i < 16; i++) {
        const float a = (float)i * (FM_TWO_PI / 16.0f) + vz.tt * 1.4f, r = (i & 1) ? 3.0f : 6.0f;
        const int on = (int)((hash(77u, (uint32_t)i + (uint32_t)(vz.tt * 3.0f)) >> 9) & 3u);
        cv_rect(bx + (int32_t)(fm_cosf(a) * r) - 1, by + (int32_t)(fm_sinf(a) * r * 0.8f) - 1, 3, 3, on == 0 ? K_TEXT : on == 1 ? vpal(vz.ph * 3.0f + (float)i * 0.06f) : K_DIM);
    }
    vz_eye();
    {   /* the drum pattern, small, at the right (as on the home screen), the step playing lit */
        const int pos = en_drum_pos, pat = en_pat_now;
        char tt[8] = "PAT ";
        cv_round(168, 6, 70, 62, 3, K_PANEL);
        for (l = 0; l < EN_NLANE; l++) {
            const uint16_t bits = en_pats[pat][l];
            const int g = q16(en_drum_glow[l]);
            for (s = 0; s < 16; s++) {
                const int32_t x = 172 + s * 4, y = 10 + l * 9;
                const int on = (int)(bits >> s & 1u), cur = en_playing && s == pos;
                if (on)
                    cv_rect(x, y, 3, 6, en_pulse_rest ? K_LINE : cur ? K_TEXT : mix(K_LINE, LANE_COL(l), 8 + g / 2));
                else
                    cv_rect(x + 1, y + 2, 1, 2, cur ? K_DIM : K_LINE);
            }
        }
        itoa_u((uint32_t)pat + 1u, tt + 4);
        cv_text(172, 64 - 2, &FONT_XS, tt, K_DIM);
    }
}

static void vz_life_seed(void)
{
    int x, y;
    for (y = 0; y < VZ_CH; y++)
        for (x = 0; x < VZ_CW; x++)
            vz.buf.life[vz.lcur][y][x] = y < VZ_ROWS && (vz_rnd() & 7u) == 0u ? 1u : 0u;
}
static void vz_life_step(void)
{
    uint8_t (*a)[VZ_CW] = vz.buf.life[vz.lcur];
    uint8_t (*b)[VZ_CW] = vz.buf.life[vz.lcur ^ 1u];
    int x, y, pop = 0;
    const int rows = VZ_ROWS;
    for (y = 0; y < rows; y++)
        for (x = 0; x < VZ_CW; x++) {
            const int xm = (x + VZ_CW - 1) % VZ_CW, xp = (x + 1) % VZ_CW, ym = (y + rows - 1) % rows, yp = (y + 1) % rows;
            const int n = (a[ym][xm] > 0) + (a[ym][x] > 0) + (a[ym][xp] > 0) + (a[y][xm] > 0) + (a[y][xp] > 0) + (a[yp][xm] > 0) +
                          (a[yp][x] > 0) + (a[yp][xp] > 0);
            if (a[y][x])
                b[y][x] = (n == 2 || n == 3) ? (uint8_t)(a[y][x] < 250 ? a[y][x] + 1 : 250) : 0;
            else
                b[y][x] = n == 3 ? 1u : 0u;
            pop += b[y][x] > 0;
        }
    vz.lcur ^= 1u;
    if (pop < 14 + (VZ_ROWS - 24) / 2)              /* gone quiet: the drums start it again */
        vz_life_seed();
}
static void vz_life(void)
{
    int x, y, k;
    const uint32_t now = plat_ms();
    const int rows = VZ_ROWS;
    uint8_t (*a)[VZ_CW] = vz.buf.life[vz.lcur];
    if (!vz.pass && (en_playing ? (vz.last_step != en_drum_pos) : (now - vz.life_ms > 220u))) {   /* a step for every sixteenth */
        vz.last_step = en_drum_pos;
        vz.life_ms = now;
        if (vz.kick > 0.6f) {                      /* a kick: a cluster in the middle, nudged about */
            const int cx = 14 + (int)(vz_rnd() % 12u), cy = rows / 2 - 4 + (int)(vz_rnd() % 8u);
            for (k = 0; k < 7; k++)
                a[cy + (int)(vz_rnd() % 3u)][cx + (int)(vz_rnd() % 3u)] = 1u;
        }
        if (vz.snare > 0.6f)
            for (k = 0; k < 10; k++)
                a[vz_rnd() % (uint32_t)rows][vz_rnd() % VZ_CW] = 1u;
        if (vz.hat > 0.6f)
            a[vz_rnd() % (uint32_t)rows][vz_rnd() % VZ_CW] = 1u;
        vz_life_step();
        a = vz.buf.life[vz.lcur];
    }
    for (y = 0; y < rows; y++)
        for (x = 0; x < VZ_CW; x++)
            if (!vz_vis(y * 6, 6))
                continue;
            else if (a[y][x])
                cv_rect(x * 6, y * 6, 5, 5, vpal((float)a[y][x] * 0.025f + vz.ph + (float)x * 0.004f));
            else if (((x + y) & 1) == 0)
                cv_rect(x * 6 + 2, y * 6 + 2, 1, 1, K_LINE);
}

static void vz_orbit(void)
{
    const int N = 360;
    const float sy = (float)vz.h * (1.0f / 144.0f);
    int i;
    for (i = 0; i < 3 && !vz.pass; i++) {         /* the chord's notes (their pitch classes) as the curve's frequencies, eased to */
        const float tg = 1.0f + (float)(en_chord[i] % 12u) * (1.0f / 12.0f);
        vz.cf[i] += (tg - vz.cf[i]) * fm_minf(vz.dt * 2.0f, 1.0f);
    }
    {
        int32_t px = 0, py = 0;
        for (i = 0; i <= N; i++) {
            const float th = (float)i * (FM_TWO_PI * 3.0f / (float)N), amp = 1.0f + vz.kick * 0.25f;
            const int32_t x = 118 + (int32_t)(100.0f * fm_sinf(th * vz.cf[0] * 2.0f + vz.ph2 * 0.7f) * amp + 14.0f * fm_sinf(th * vz.cf[2] * 3.0f)),
                          y = vz.h / 2 - 2 + (int32_t)(sy * (58.0f * fm_sinf(th * vz.cf[1] * 2.0f + vz.tt * 0.3f) + 12.0f * fm_cosf(th * vz.cf[2] * 3.0f)));
            if (i)
                cv_line(px, py, x, y, vpal((float)i * (1.0f / (float)N) + vz.ph * 2.0f));
            px = x, py = y;
        }
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
        if (mode == VZ_FLUX || mode == VZ_KALEIDO)
            vz_flux_init();
        if (mode == VZ_LIFE)
            vz_life_seed();
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
        case VZ_PLASMA: vz_plasma(); break;
        case VZ_KALEIDO: vz_kaleido(); break;
        case VZ_BARS: vz_bars(); break;
        case VZ_FLUX: vz_flux(); break;
        case VZ_DISCO: vz_disco(); break;
        case VZ_LIFE: vz_life(); break;
        case VZ_VORTEX: vz_vortex(); break;
        case VZ_TUNNEL: vz_tunnel(); break;
        case VZ_STARS: vz_stars(); break;
        case VZ_RAIN: vz_rain(); break;
        case VZ_STORM: vz_storm(); break;
        case VZ_RIPPLES: vz_ripples(); break;
        case VZ_RADAR: vz_radar(); break;
        case VZ_WATER: vz_waterfall(); break;
        default: vz_orbit(); break;
        }
        cv_blit(0, full ? (uint32_t)vz.y0 : MAIN_Y);
    }
    vz.pass = 0;
}
