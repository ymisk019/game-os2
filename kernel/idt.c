/* Interrupt Descriptor Table: CPU exceptions 0-31.
 * #BP (int3) is a recoverable trap used as a self-test; everything else is fatal
 * and ends in a clear kernel-panic screen with registers + backtrace. */
#include <stdint.h>
#include "idt.h"
#include "panic.h"
#include "serial.h"
#include "io.h"

typedef struct {
    uint16_t off_lo, sel;
    uint8_t  ist, type_attr;
    uint16_t off_mid;
    uint32_t off_hi, zero;
} __attribute__((packed)) idt_entry_t;

_Static_assert(sizeof(idt_entry_t) == 16, "IDT entry must be 16 bytes");

extern const uint64_t isr_table[32];
static idt_entry_t idt[256] __attribute__((aligned(16)));
volatile int idt_bp_hits;

static void set_gate(int v, uint64_t handler) {
    idt[v].off_lo    = (uint16_t)handler;
    idt[v].sel       = 0x08;          /* kernel code segment from boot.S GDT */
    idt[v].ist       = 0;
    idt[v].type_attr = 0x8E;          /* present, DPL0, 64-bit interrupt gate */
    idt[v].off_mid   = (uint16_t)(handler >> 16);
    idt[v].off_hi    = (uint32_t)(handler >> 32);
    idt[v].zero      = 0;
}

void idt_init(void) {
    /* Mask both legacy PICs: no hardware IRQs until the timer/keyboard phase. */
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    for (int v = 0; v < 32; v++) set_gate(v, isr_table[v]);

    struct { uint16_t limit; uint64_t base; } __attribute__((packed)) idtr = {
        (uint16_t)(sizeof idt - 1), (uint64_t)(uintptr_t)idt
    };
    __asm__ volatile("lidt %0" :: "m"(idtr));
}

void exception_handler(regs_t *r) {
    if (r->vector == 3) {             /* breakpoint: log and resume */
        idt_bp_hits++;
        klog("[trap] #BP at 0x%lx\n", (unsigned long)r->rip);
        return;
    }
    kpanic_exception(r);
}
