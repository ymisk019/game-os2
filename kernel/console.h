#pragma once
#include "gfx.h"

void console_init(int x, int y, int w, int h, int scale, rgb_t fg, rgb_t bg);
int  console_ready(void);
void console_set_fg(rgb_t fg);
void console_putc(char c);
void console_puts(const char *s);

/* Print to the on-screen console AND the serial port. */
void kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void kok(const char *fmt, ...)     __attribute__((format(printf, 1, 2)));   /* [ OK ] line */
void kwarn(const char *fmt, ...)   __attribute__((format(printf, 1, 2)));   /* [WARN] line */
