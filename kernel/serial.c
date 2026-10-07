/* COM1 debug output (QEMU: -serial stdio). 115200 baud, 8N1. */
#include "serial.h"
#include "io.h"
#include "kstring.h"

#define COM1 0x3F8

void serial_init(void) {
    outb(COM1 + 1, 0x00);   /* disable interrupts */
    outb(COM1 + 3, 0x80);   /* DLAB on */
    outb(COM1 + 0, 0x01);   /* divisor 1 -> 115200 */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);   /* 8N1, DLAB off */
    outb(COM1 + 2, 0xC7);   /* enable FIFO */
    outb(COM1 + 4, 0x0B);   /* RTS/DSR set */
}

void serial_putc(char c) {
    for (int spin = 0; spin < 100000 && !(inb(COM1 + 5) & 0x20); spin++) {}
    outb(COM1, (uint8_t)c);
}

void serial_puts(const char *s) {
    for (; *s; s++) {
        if (*s == '\n') serial_putc('\r');
        serial_putc(*s);
    }
}

void klog(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    serial_puts(buf);
}
