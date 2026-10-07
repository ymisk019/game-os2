#pragma once
void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *s);
void klog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
