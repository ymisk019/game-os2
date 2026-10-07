/* Host-side renderer: runs the real boot UI + panic-screen code against a fake
 * framebuffer and writes PPM images. Iterate on the UI without booting a VM. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "gfx.h"
#include "boot_ui.h"
#include "console.h"
#include "panic.h"

/* Stubs: on the host there is no serial port. */
void serial_puts(const char *s) { (void)s; }
void klog(const char *fmt, ...) { (void)fmt; }

static void write_ppm(const char *path, const uint8_t *buf, uint32_t w, uint32_t h) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (uint32_t i = 0; i < w * h; i++) {
        uint32_t px = ((const uint32_t *)buf)[i];
        fputc((px >> 16) & 0xFF, f); fputc((px >> 8) & 0xFF, f); fputc(px & 0xFF, f);
    }
    fclose(f);
    printf("wrote %s\n", path);
}

int main(int argc, char **argv) {
    const char *base = argc > 1 ? argv[1] : "preview";
    uint32_t w = argc > 2 ? (uint32_t)atoi(argv[2]) : 1024;
    uint32_t h = argc > 3 ? (uint32_t)atoi(argv[3]) : 768;
    uint8_t *buf = calloc(w * h, 4);
    char path[256];

    gfx_fb_t fb = { .base = buf, .pitch = w * 4, .width = w, .height = h, .bpp = 32,
                    .rpos = 16, .rsize = 8, .gpos = 8, .gsize = 8, .bpos = 0, .bsize = 8 };
    gfx_init(&fb);

    bootinfo_t bi; memset(&bi, 0, sizeof bi);
    strcpy(bi.loader, "GRUB 2.12-1ubuntu7.3");
    strcpy(bi.cpu_vendor, "GenuineIntel");
    bi.mem_usable = 264ULL << 20;
    bi.fb_w = w; bi.fb_h = h; bi.fb_bpp = 32; bi.fb_ok = 1;
    bi.kernel_start = 0x100000; bi.kernel_end = 0x118000;

    /* ---- screen 1: normal boot ---- */
    boot_ui_start(&bi);
    kok("IDT        : 32 CPU exception vectors, PICs masked");
    kok("Self-test  : #BP trap handled, execution resumed");
    kprintf("\n");
    kok("Phase 2 complete. Kernel idle (no scheduler yet).");
    kprintf("Next: physical memory manager, paging, heap.\n");
    snprintf(path, sizeof path, "%s-boot.ppm", base);
    write_ppm(path, buf, w, h);

    /* ---- screen 1b: scrolling stress (60 lines) ---- */
    for (int i = 0; i < 60; i++) kprintf("line %02d: scrolling test, the console keeps the newest output\n", i);
    snprintf(path, sizeof path, "%s-scroll.ppm", base);
    write_ppm(path, buf, w, h);

    /* ---- screen 2: page-fault panic ---- */
    regs_t r; memset(&r, 0, sizeof r);
    r.vector = 14; r.error = 0x2;
    r.rip = 0x10204A; r.cs = 0x08; r.ss = 0x10; r.rflags = 0x10006; r.rsp = 0x112FE0;
    r.rax = 1; r.rbx = 0xDEADBEEF; r.rcx = 0x180000000ULL; r.rbp = 0;
    panic_exception_screen(&r, 0x180000000ULL, 0x104000);
    snprintf(path, sizeof path, "%s-panic.ppm", base);
    write_ppm(path, buf, w, h);

    free(buf);
    return 0;
}
