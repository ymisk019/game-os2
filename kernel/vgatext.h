#pragma once
void vga_clear(unsigned char attr);
void vga_puts_at(int col, int row, const char *s, unsigned char attr);
