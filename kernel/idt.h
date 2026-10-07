#pragma once
#include "regs.h"
void idt_init(void);
void exception_handler(regs_t *r);     /* called from isr.S */
extern volatile int idt_bp_hits;       /* number of #BP traps handled (self-test) */
