/* Legacy 80x25 VGA text mode (0xB8000). Fallback when no framebuffer is given. */
#include "vgatext.h"
#include <stdint.h>

static volatile uint16_t *const VGA = (volatile uint16_t *)0xB8000;

void vga_clear(unsigned char attr) {
    for (int i = 0; i < 80 * 25; i++) VGA[i] = (uint16_t)(attr << 8) | ' ';
}

void vga_puts_at(int col, int row, const char *s, unsigned char attr) {
    for (; *s && row < 25; s++) {
        if (*s == '\n') { col = 0; row++; continue; }
        if (col >= 80) { col = 0; row++; if (row >= 25) return; }
        VGA[row * 80 + col++] = (uint16_t)(attr << 8) | (uint8_t)*s;
    }
}
