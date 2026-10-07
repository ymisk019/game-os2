#include "boot_ui.h"
#include "gfx.h"
#include "splash.h"
#include "console.h"
#include "theme.h"

void boot_ui_start(const bootinfo_t *bi) {
    int W = (int)gfx_width(), H = (int)gfx_height();
    int top = splash_header();
    gfx_fill_rect(0, top, W, H - top, COL_PANEL);
    int scale = W >= 900 ? 2 : 1;
    console_init(16, top + 12, W - 32, H - top - 24, scale, COL_TEXT, COL_PANEL);

    kok("Bootloader : %s", bi->loader[0] ? bi->loader : "Multiboot2");
    kok("CPU        : %s (x86_64, long mode)", bi->cpu_vendor);
    kok("Display    : %ux%ux%u framebuffer", bi->fb_w, bi->fb_h, bi->fb_bpp);
    kok("Memory     : %lu MB usable RAM", (unsigned long)(bi->mem_usable >> 20));
    kok("Kernel     : 0x%lx - 0x%lx (%lu KB)", (unsigned long)bi->kernel_start,
        (unsigned long)bi->kernel_end, (unsigned long)((bi->kernel_end - bi->kernel_start) >> 10));
}
