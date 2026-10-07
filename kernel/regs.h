#pragma once
#include <stdint.h>

/* Layout MUST match the push order in isr.S (lowest address first). */
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector, error;                 /* pushed by our stub */
    uint64_t rip, cs, rflags, rsp, ss;      /* pushed by the CPU */
} regs_t;

_Static_assert(sizeof(regs_t) == 22 * 8, "regs_t layout must match isr.S");
