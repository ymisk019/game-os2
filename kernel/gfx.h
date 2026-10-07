#pragma once
#include <stdint.h>

typedef uint32_t rgb_t;   /* 0xRRGGBB */

typedef struct {
    uint8_t *base;
    uint32_t pitch, width, height;
    uint8_t  bpp;
    uint8_t  rpos, rsize, gpos, gsize, bpos, bsize;
} gfx_fb_t;

void     gfx_init(const gfx_fb_t *fb);
int      gfx_ready(void);
uint32_t gfx_width(void);
uint32_t gfx_height(void);

rgb_t gfx_mix(rgb_t a, rgb_t b, int t256);     /* t=0 -> a, t=256 -> b */
void  gfx_pixel(int x, int y, rgb_t c);
void  gfx_fill_rect(int x, int y, int w, int h, rgb_t c);
void  gfx_fill_rrect(int x, int y, int w, int h, int r, rgb_t c);
void  gfx_vgradient(int x, int y, int w, int h, rgb_t top, rgb_t bottom);
void  gfx_clear(rgb_t c);

void gfx_text(int x, int y, const char *s, int scale, rgb_t c);
int  gfx_text_width(const char *s, int scale);
void gfx_text_center(int cx, int y, const char *s, int scale, rgb_t c);
void gfx_char(int x, int y, char ch, int scale, rgb_t fg, rgb_t bg);
void gfx_scroll_up(int x, int y, int w, int h, int dy, rgb_t fill);
