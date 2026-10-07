/* Minimal linear-framebuffer graphics: pixels, rects, gradients, 8x8 text.
 * No floating point and no heap, so it is safe in early boot. */
#include "gfx.h"
#include "font8x8.h"
#include "kstring.h"

static gfx_fb_t fb;
static int ready;

void gfx_init(const gfx_fb_t *f) { fb = *f; ready = 1; }
int  gfx_ready(void)  { return ready; }
uint32_t gfx_width(void)  { return fb.width; }
uint32_t gfx_height(void) { return fb.height; }

static inline uint32_t pack(rgb_t c) {
    uint32_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
    return ((r >> (8 - fb.rsize)) << fb.rpos) |
           ((g >> (8 - fb.gsize)) << fb.gpos) |
           ((b >> (8 - fb.bsize)) << fb.bpos);
}

static inline void put_packed(int x, int y, uint32_t v) {
    uint8_t *p = fb.base + (size_t)y * fb.pitch;
    switch (fb.bpp) {
    case 32: ((uint32_t *)p)[x] = v; break;
    case 24: p += x * 3; p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); break;
    case 16: ((uint16_t *)p)[x] = (uint16_t)v; break;
    default: break;
    }
}

rgb_t gfx_mix(rgb_t a, rgb_t b, int t) {
    if (t < 0) t = 0;
    if (t > 256) t = 256;
    uint32_t ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
    uint32_t br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
    uint32_t r = (ar * (256 - t) + br * t) >> 8;
    uint32_t g = (ag * (256 - t) + bg * t) >> 8;
    uint32_t bl = (ab * (256 - t) + bb * t) >> 8;
    return (r << 16) | (g << 8) | bl;
}

void gfx_pixel(int x, int y, rgb_t c) {
    if (!ready || x < 0 || y < 0 || x >= (int)fb.width || y >= (int)fb.height) return;
    put_packed(x, y, pack(c));
}

void gfx_fill_rect(int x, int y, int w, int h, rgb_t c) {
    if (!ready) return;
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > (int)fb.width)  x1 = (int)fb.width;
    if (y1 > (int)fb.height) y1 = (int)fb.height;
    if (x >= x1 || y >= y1) return;
    uint32_t v = pack(c);
    for (int j = y; j < y1; j++) {
        if (fb.bpp == 32) {
            uint32_t *row = (uint32_t *)(fb.base + (size_t)j * fb.pitch);
            for (int i = x; i < x1; i++) row[i] = v;
        } else {
            for (int i = x; i < x1; i++) put_packed(i, j, v);
        }
    }
}

void gfx_fill_rrect(int x, int y, int w, int h, int r, rgb_t c) {
    if (!ready || w <= 0 || h <= 0) return;
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    uint32_t v = pack(c);
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int dx = 0, dy = 0;
            if (i < r) dx = r - 1 - i; else if (i >= w - r) dx = i - (w - r);
            if (j < r) dy = r - 1 - j; else if (j >= h - r) dy = j - (h - r);
            if (dx * dx + dy * dy >= r * r && (dx || dy)) continue;
            int px = x + i, py = y + j;
            if (px < 0 || py < 0 || px >= (int)fb.width || py >= (int)fb.height) continue;
            put_packed(px, py, v);
        }
    }
}

void gfx_vgradient(int x, int y, int w, int h, rgb_t top, rgb_t bottom) {
    if (h <= 0) return;
    for (int j = 0; j < h; j++)
        gfx_fill_rect(x, y + j, w, 1, gfx_mix(top, bottom, (j * 256) / h));
}

void gfx_clear(rgb_t c) { gfx_fill_rect(0, 0, (int)fb.width, (int)fb.height, c); }

int gfx_text_width(const char *s, int scale) { return (int)strlen(s) * 8 * scale; }

void gfx_text(int x, int y, const char *s, int scale, rgb_t c) {
    if (scale < 1) scale = 1;
    for (; *s; s++, x += 8 * scale) {
        unsigned ch = (uint8_t)*s;
        if (ch < 0x20 || ch > 0x7E) ch = '?';
        const uint8_t *g = font8x8[ch - 0x20];
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if (g[row] & (1u << col))
                    gfx_fill_rect(x + col * scale, y + row * scale, scale, scale, c);
    }
}

void gfx_text_center(int cx, int y, const char *s, int scale, rgb_t c) {
    gfx_text(cx - gfx_text_width(s, scale) / 2, y, s, scale, c);
}

void gfx_char(int x, int y, char ch, int scale, rgb_t fg, rgb_t bg) {
    unsigned u = (uint8_t)ch;
    if (u < 0x20 || u > 0x7E) u = '?';
    const uint8_t *g = font8x8[u - 0x20];
    gfx_fill_rect(x, y, 8 * scale, 8 * scale, bg);
    for (int row = 0; row < 8; row++)
        for (int col = 0; col < 8; col++)
            if (g[row] & (1u << col))
                gfx_fill_rect(x + col * scale, y + row * scale, scale, scale, fg);
}

/* Move the rectangle's contents up by dy pixel rows and clear the freed strip. */
void gfx_scroll_up(int x, int y, int w, int h, int dy, rgb_t fill) {
    if (!ready || dy <= 0 || w <= 0 || h <= 0) return;
    if (x < 0 || y < 0 || x + w > (int)fb.width || y + h > (int)fb.height) return;
    if (dy > h) dy = h;
    size_t bpp = fb.bpp / 8, bytes = (size_t)w * bpp;
    for (int j = y; j < y + h - dy; j++)
        memcpy(fb.base + (size_t)j * fb.pitch + (size_t)x * bpp,
               fb.base + (size_t)(j + dy) * fb.pitch + (size_t)x * bpp, bytes);
    gfx_fill_rect(x, y + h - dy, w, dy, fill);
}
