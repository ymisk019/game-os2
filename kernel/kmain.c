/* Yazan OS kernel entry (64-bit). Called from boot/boot.S in long mode. */
#include <stdint.h>
#include "mb2.h"
#include "bootinfo.h"
#include "gfx.h"
#include "boot_ui.h"
#include "console.h"
#include "idt.h"
#include "serial.h"
#include "vgatext.h"
#include "panic.h"
#include "io.h"
#include "kstring.h"

#ifndef TEST_FAULT
#define TEST_FAULT 0
#endif

extern char __kernel_start[], __kernel_end[];

static void cpu_vendor(char out[13]) {
    uint32_t a, b, c, d;
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(0));
    memcpy(out + 0, &b, 4);
    memcpy(out + 4, &d, 4);
    memcpy(out + 8, &c, 4);
    out[12] = 0;
}

static void parse_mb2(uintptr_t mbi, bootinfo_t *bi, gfx_fb_t *fb) {
    const uint8_t *p = (const uint8_t *)mbi + 8;           /* skip total_size + reserved */
    for (;;) {
        const mb2_tag_t *t = (const mb2_tag_t *)p;
        if (t->type == MB2_TAG_END) break;

        if (t->type == MB2_TAG_LOADER) {
            const char *s = (const char *)(p + 8);
            size_t n = t->size - 8; if (n > sizeof bi->loader) n = sizeof bi->loader;
            size_t i = 0; for (; i + 1 < n && s[i]; i++) bi->loader[i] = s[i];
            bi->loader[i] = 0;
        } else if (t->type == MB2_TAG_MMAP) {
            const mb2_mmap_tag_t *m = (const mb2_mmap_tag_t *)t;
            const uint8_t *e = p + sizeof *m, *end = p + t->size;
            for (; e + m->entry_size <= end; e += m->entry_size) {
                const mb2_mmap_entry_t *me = (const mb2_mmap_entry_t *)e;
                if (me->type == MB2_MMAP_AVAILABLE) bi->mem_usable += me->len;
            }
        } else if (t->type == MB2_TAG_FB) {
            const mb2_fb_tag_t *f = (const mb2_fb_tag_t *)t;
            bi->fb_w = f->width; bi->fb_h = f->height; bi->fb_bpp = f->bpp;
            if (f->type == MB2_FB_RGB && (f->bpp == 32 || f->bpp == 24 || f->bpp == 16)) {
                fb->base = (uint8_t *)(uintptr_t)f->addr;
                fb->pitch = f->pitch; fb->width = f->width; fb->height = f->height; fb->bpp = f->bpp;
                fb->rpos = f->red_pos;   fb->rsize = f->red_size;
                fb->gpos = f->green_pos; fb->gsize = f->green_size;
                fb->bpos = f->blue_pos;  fb->bsize = f->blue_size;
                bi->fb_ok = 1;
            }
        }
        p += (t->size + 7u) & ~7u;                          /* tags are 8-byte aligned */
    }
}

/* Deliberate crash for testing the panic screen: build with  make iso FAULT=div0|ud|pf|gp */
static void test_fault(void) {
#if TEST_FAULT == 1
    kwarn("TEST: triggering divide-by-zero (#DE)");
    volatile int zero = 0; volatile int q = 1 / zero; (void)q;
#elif TEST_FAULT == 2
    kwarn("TEST: executing an invalid opcode (#UD)");
    __asm__ volatile("ud2");
#elif TEST_FAULT == 3
    kwarn("TEST: touching unmapped memory (#PF)");
    *(volatile uint64_t *)0x180000000ULL = 1;
#elif TEST_FAULT == 4
    kwarn("TEST: non-canonical address access (#GP)");
    *(volatile uint64_t *)0xFFFF800000000000ULL = 1;
#endif
}

void kmain(uintptr_t mbi, uint32_t magic) {
    serial_init();
    klog("\n=== Yazan OS 0.2.0 (x86_64) ===\n");

    if (magic != MB2_BOOT_MAGIC) kpanic("Bad Multiboot2 magic: 0x%x", magic);

    bootinfo_t bi; memset(&bi, 0, sizeof bi);
    gfx_fb_t fb;   memset(&fb, 0, sizeof fb);
    bi.kernel_start = (uintptr_t)__kernel_start;
    bi.kernel_end   = (uintptr_t)__kernel_end;
    cpu_vendor(bi.cpu_vendor);
    parse_mb2(mbi, &bi, &fb);

    if (bi.fb_ok) {
        gfx_init(&fb);
        boot_ui_start(&bi);                     /* header + console + boot-info lines */
    } else {
        klog("[boot] no RGB framebuffer, using VGA text mode\n");
        vga_clear(0x1F);
        vga_puts_at(2, 1, "YAZAN OS 0.2.0 - Phase 2 (VGA text fallback)", 0x1F);
        vga_puts_at(2, 3, "See the serial log for boot details.", 0x1F);
    }

    /* ---- Phase 2: IDT + exception handling ---- */
    idt_init();
    kok("IDT        : 32 CPU exception vectors, PICs masked");

    __asm__ volatile("int3");                   /* self-test: must trap, log, and resume here */
    if (idt_bp_hits != 1) kpanic("IDT self-test failed: breakpoint trap did not return");
    kok("Self-test  : #BP trap handled, execution resumed");

    kprintf("\n");
    kok("Phase 2 complete. Kernel idle (no scheduler yet).");
    kprintf("Next: physical memory manager, paging, heap.\n");

    test_fault();

    cpu_halt_forever();
}
