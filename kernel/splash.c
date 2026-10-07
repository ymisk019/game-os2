/* Yazan OS header band: gradient, original "Y" logo, title. Pure integer math. */
#include "splash.h"
#include "gfx.h"
#include "theme.h"
#include "kstring.h"

static rgb_t hdr_bg(int y, int hh) { return gfx_mix(BG_TOP, HDR_BOTTOM, (y * 256) / hh); }

/* Squared distance from point p to segment a-b. */
static int64_t seg_dist2(int64_t px, int64_t py, int64_t ax, int64_t ay, int64_t bx, int64_t by) {
    int64_t vx = bx - ax, vy = by - ay, wx = px - ax, wy = py - ay;
    int64_t den = vx * vx + vy * vy;
    int64_t dot = wx * vx + wy * vy;
    if (dot <= 0)   return wx * wx + wy * wy;
    if (dot >= den) { int64_t ex = px - bx, ey = py - by; return ex * ex + ey * ey; }
    int64_t cross = wx * vy - wy * vx;
    return (cross * cross) / den;
}

/* Is the (2x supersampled) point inside the "Y" glyph? Unit space = 1000. */
static int in_y(int sx, int sy, int s2) {
    int64_t hw = 78LL * s2 / 1000;
    int64_t hw2 = hw * hw;
    #define P(v) ((int64_t)(v) * s2 / 1000)
    if (seg_dist2(sx, sy, P(240), P(225), P(500), P(540)) <= hw2) return 1;
    if (seg_dist2(sx, sy, P(760), P(225), P(500), P(540)) <= hw2) return 1;
    if (seg_dist2(sx, sy, P(500), P(540), P(500), P(790)) <= hw2) return 1;
    #undef P
    return 0;
}

static int in_tile(int sx, int sy, int s2) {
    int r = s2 / 4;
    int dx = 0, dy = 0;
    if (sx < r) dx = r - sx; else if (sx >= s2 - r) dx = sx - (s2 - r) + 1;
    if (sy < r) dy = r - sy; else if (sy >= s2 - r) dy = sy - (s2 - r) + 1;
    return dx * dx + dy * dy <= r * r;
}

static void draw_logo(int x0, int y0, int size, int band_h) {
    int s2 = size * 2;
    for (int py = 0; py < size; py++) {
        rgb_t bg   = hdr_bg(y0 + py, band_h);
        rgb_t tile = gfx_mix(TILE_TOP, TILE_BOTTOM, (py * 256) / size);
        for (int px = 0; px < size; px++) {
            uint32_t r = 0, g = 0, b = 0;
            for (int k = 0; k < 4; k++) {
                int sx = px * 2 + (k & 1), sy = py * 2 + (k >> 1);
                rgb_t c = bg;
                if (in_tile(sx, sy, s2)) c = in_y(sx, sy, s2) ? COL_WHITE : tile;
                r += (c >> 16) & 0xFF; g += (c >> 8) & 0xFF; b += c & 0xFF;
            }
            gfx_pixel(x0 + px, y0 + py, ((r / 4) << 16) | ((g / 4) << 8) | (b / 4));
        }
    }
}

int splash_header(void) {
    int W = (int)gfx_width(), H = (int)gfx_height();
    int hh = H / 10; if (hh < 64) hh = 64;
    gfx_vgradient(0, 0, W, hh, BG_TOP, HDR_BOTTOM);
    gfx_fill_rect(0, hh, W, 2, TILE_BOTTOM);

    int logo = hh - 20;
    draw_logo(16, 10, logo, hh);

    int ts = hh >= 76 ? 3 : 2;
    int ss = W >= 900 ? 2 : 1;
    int total = 8 * ts + 6 + 8 * ss;
    int ty = (hh - total) / 2;
    int tx = 16 + logo + 16;
    gfx_text(tx + ts, ty + ts, "YAZAN OS", ts, 0x06101F);   /* soft shadow */
    gfx_text(tx, ty, "YAZAN OS", ts, COL_WHITE);
    gfx_text(tx, ty + 8 * ts + 6, "Version 0.2.0  |  Phase 2: IDT + Console", ss, COL_DIM);
    return hh + 2;
}
