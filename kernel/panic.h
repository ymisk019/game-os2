#pragma once
#include <stdint.h>
#include "regs.h"

/* Generic fatal error with a formatted message. */
void kpanic(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));

/* Fatal CPU exception: shows registers + backtrace. Called from the IDT handler. */
void kpanic_exception(const regs_t *r) __attribute__((noreturn));

/* Draw + log the exception screen without halting (used by the host preview). */
void panic_exception_screen(const regs_t *r, uint64_t cr2, uint64_t cr3);
