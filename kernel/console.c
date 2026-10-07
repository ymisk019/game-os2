/* Scrolling text console on top of the framebuffer (8x8 font, integer scale). */
#include "console.h"
#include "serial.h"
#include "theme.h"
#include "kstring.h"

static struct {
    int x, y, cell, lh, cols, rows, cx, cy, ready;
    rgb_t fg, bg, fg_default;
} con;

void console_init(int x, int y, int w, int h, int scale, rgb_t fg, rgb_t bg) {
    con.x = x; con.y = y; con.cell = 8 * scale;
    con.lh = con.cell + 2 * scale;                 /* line height = glyph + small gap */
    con.cols = w / con.cell; con.rows = h / con.lh;
    con.cx = con.cy = 0;
    con.fg = con.fg_default = fg; con.bg = bg;
    gfx_fill_rect(x, y, w, h, bg);
    con.ready = con.cols > 0 && con.rows > 0;
}

int  console_ready(void) { return con.ready; }
void console_set_fg(rgb_t fg) { con.fg = fg; }

static void newline(void) {
    con.cx = 0;
    if (++con.cy >= con.rows) {
        con.cy = con.rows - 1;
        gfx_scroll_up(con.x, con.y, con.cols * con.cell, con.rows * con.lh, con.lh, con.bg);
    }
}

void console_putc(char c) {
    if (!con.ready) return;
    int scale = con.cell / 8;
    switch (c) {
    case '\n': newline(); break;
    case '\r': con.cx = 0; break;
    case '\t': do { console_putc(' '); } while (con.cx % 4); break;
    default:
        if (con.cx >= con.cols) newline();
        gfx_char(con.x + con.cx * con.cell, con.y + con.cy * con.lh + scale, c, scale, con.fg, con.bg);
        con.cx++;
    }
}

void console_puts(const char *s) { while (*s) console_putc(*s++); }

void kprintf(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    serial_puts(buf);
    console_set_fg(con.fg_default);
    console_puts(buf);
}

static void tagged(const char *tag, rgb_t tagcol, const char *msg) {
    console_set_fg(COL_DIM);  console_puts("[");
    console_set_fg(tagcol);   console_puts(tag);
    console_set_fg(COL_DIM);  console_puts("] ");
    console_set_fg(con.fg_default);
    console_puts(msg);
    console_putc('\n');
    serial_puts("["); serial_puts(tag); serial_puts("] "); serial_puts(msg); serial_puts("\n");
}

void kok(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    tagged(" OK ", COL_OK, buf);
}

void kwarn(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    tagged("WARN", COL_WARN, buf);
}
