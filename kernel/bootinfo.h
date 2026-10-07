#pragma once
#include <stdint.h>

/* Everything the kernel learns from the bootloader / CPU in early boot. */
typedef struct {
    char      loader[64];
    char      cpu_vendor[16];
    uint64_t  mem_usable;          /* bytes of RAM marked available by the firmware */
    uint32_t  fb_w, fb_h, fb_bpp;
    int       fb_ok;
    uintptr_t kernel_start, kernel_end;
} bootinfo_t;
