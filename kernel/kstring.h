#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef HOST_PREVIEW
#include <string.h>
#else
void  *memset(void *dst, int c, size_t n);
void  *memcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
int    memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
#endif

int ksnprintf(char *buf, size_t n, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
int kvsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
