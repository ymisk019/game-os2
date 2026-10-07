/* Kernel panic screens (framebuffer or VGA text) + serial dump, then halt. */
#include "panic.h"
#include "gfx.h"
#include "serial.h"
#include "vgatext.h"
#include "kstring.h"
#include "io.h"

#define PANIC_BG   0x7F1D1D
#define PANIC_FG   0xFFFFFF
#define PANIC_DIM  0xFFC9C9
#define PANIC_HEAD 0xFFE08A

void kpanic(const char *fmt, ...) {
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    klog("\n*** KERNEL PANIC: %s ***\n", msg);

    if (gfx_ready()) {
        int W = (int)gfx_width();
        gfx_clear(PANIC_BG);
        gfx_text(40, 40, "YAZAN OS - KERNEL PANIC", 3, PANIC_FG);
        gfx_text(40, 40 + 8 * 3 + 24, "The system has been halted to protect your data.", 2, PANIC_DIM);
        int cols = (W - 80) / 16; if (cols < 8) cols = 8;
        int y = 40 + 8 * 3 + 24 + 16 * 2 + 16;
        for (const char *p = msg; *p; ) {
            char line[128]; int n = 0;
            while (p[n] && n < cols && n < 127) n++;
            memcpy(line, p, (size_t)n); line[n] = 0;
            gfx_text(40, y, line, 2, PANIC_FG);
            y += 8 * 2 + 6; p += n;
        }
    } else {
        vga_clear(0x4F);
        vga_puts_at(2, 2, "YAZAN OS - KERNEL PANIC", 0x4F);
        vga_puts_at(2, 4, msg, 0x4F);
    }
    cpu_halt_forever();
}

static const char *const exc_names[32] = {
    "Divide Error (#DE)", "Debug (#DB)", "Non-Maskable Interrupt (NMI)", "Breakpoint (#BP)",
    "Overflow (#OF)", "Bound Range Exceeded (#BR)", "Invalid Opcode (#UD)", "Device Not Available (#NM)",
    "Double Fault (#DF)", "Coprocessor Segment Overrun", "Invalid TSS (#TS)", "Segment Not Present (#NP)",
    "Stack-Segment Fault (#SS)", "General Protection Fault (#GP)", "Page Fault (#PF)", "Reserved",
    "x87 Floating-Point (#MF)", "Alignment Check (#AC)", "Machine Check (#MC)", "SIMD Floating-Point (#XM)",
    "Virtualization (#VE)", "Control Protection (#CP)", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection (#HV)", "VMM Communication (#VC)", "Security (#SX)", "Reserved",
};

/* Walk the RBP chain (kernel is built with frame pointers). Stays inside the
 * identity-mapped first 4 GiB so the panic path can never fault itself. */
static int backtrace(uint64_t rbp, uint64_t *out, int max) {
    int n = 0;
    while (n < max && rbp >= 0x100000 && rbp < 0x100000000ULL && !(rbp & 7)) {
        const uint64_t *f = (const uint64_t *)(uintptr_t)rbp;
        uint64_t ret = f[1], next = f[0];
        if (!ret) break;
        out[n++] = ret;
        if (next <= rbp) break;
        rbp = next;
    }
    return n;
}

void panic_exception_screen(const regs_t *r, uint64_t cr2, uint64_t cr3) {
    const char *name = r->vector < 32 ? exc_names[r->vector] : "Unknown";
    char l[8][96];
    int nl = 0;

    ksnprintf(l[nl++], sizeof l[0], "%s  -  vector %lu, error code 0x%lx",
              name, (unsigned long)r->vector, (unsigned long)r->error);
    if (r->vector == 14) {
        ksnprintf(l[nl++], sizeof l[0], "%s of %s page at 0x%lx (%s mode%s)",
                  (r->error & 2) ? "Write" : "Read",
                  (r->error & 1) ? "protected" : "non-present",
                  (unsigned long)cr2,
                  (r->error & 4) ? "user" : "kernel",
                  (r->error & 16) ? ", instruction fetch" : "");
    }

    uint64_t bt[8];
    int nbt = backtrace(r->rbp, bt, 8);

    /* ---- serial dump (always) ---- */
    klog("\n*** KERNEL PANIC: %s ***\n", l[0]);
    for (int i = 1; i < nl; i++) klog("%s\n", l[i]);
    klog("RIP=%016lx RSP=%016lx RFLAGS=%016lx CS=%lx SS=%lx\n", (unsigned long)r->rip, (unsigned long)r->rsp,
         (unsigned long)r->rflags, (unsigned long)r->cs, (unsigned long)r->ss);
    klog("RAX=%016lx RBX=%016lx RCX=%016lx RDX=%016lx\n", (unsigned long)r->rax, (unsigned long)r->rbx,
         (unsigned long)r->rcx, (unsigned long)r->rdx);
    klog("RSI=%016lx RDI=%016lx RBP=%016lx CR2=%016lx CR3=%016lx\n", (unsigned long)r->rsi,
         (unsigned long)r->rdi, (unsigned long)r->rbp, (unsigned long)cr2, (unsigned long)cr3);
    for (int i = 0; i < nbt; i++) klog("  bt[%d] = 0x%lx\n", i, (unsigned long)bt[i]);
    klog("(resolve with: addr2line -e build/yazan.elf 0x%lx)\n", (unsigned long)r->rip);

    /* ---- screen ---- */
    if (!gfx_ready()) {
        char t[80];
        vga_clear(0x4F);
        vga_puts_at(2, 1, "YAZAN OS - KERNEL PANIC", 0x4F);
        vga_puts_at(2, 3, l[0], 0x4F);
        ksnprintf(t, sizeof t, "RIP=%016lx  RSP=%016lx", (unsigned long)r->rip, (unsigned long)r->rsp);
        vga_puts_at(2, 5, t, 0x4F);
        return;
    }

    int W = (int)gfx_width();
    int s = W >= 900 ? 2 : 1, lh = 8 * s + 6, x = 32, y = 24;
    gfx_clear(PANIC_BG);
    gfx_text(x, y, "YAZAN OS - KERNEL PANIC", s + 1, PANIC_FG);
    y += 8 * (s + 1) + 8;
    gfx_text(x, y, "The system was halted to protect your data.", s, PANIC_DIM);
    y += lh + 10;
    for (int i = 0; i < nl; i++) { gfx_text(x, y, l[i], s, PANIC_HEAD); y += lh; }
    y += 10;

    char line[96];
#define REGPAIR(an, av, bn, bv) do { \
        ksnprintf(line, sizeof line, an "=%016lx  " bn "=%016lx", (unsigned long)(av), (unsigned long)(bv)); \
        gfx_text(x, y, line, s, PANIC_FG); y += lh; } while (0)
    REGPAIR("RAX", r->rax, "RBX", r->rbx);
    REGPAIR("RCX", r->rcx, "RDX", r->rdx);
    REGPAIR("RSI", r->rsi, "RDI", r->rdi);
    REGPAIR("RBP", r->rbp, "RSP", r->rsp);
    REGPAIR("R8 ", r->r8,  "R9 ", r->r9);
    REGPAIR("R10", r->r10, "R11", r->r11);
    REGPAIR("R12", r->r12, "R13", r->r13);
    REGPAIR("R14", r->r14, "R15", r->r15);
    REGPAIR("RIP", r->rip, "FLG", r->rflags);
    REGPAIR("CS ", r->cs,  "SS ", r->ss);
    REGPAIR("CR2", cr2,    "CR3", cr3);
#undef REGPAIR

    y += 10;
    gfx_text(x, y, "Backtrace (return addresses):", s, PANIC_HEAD); y += lh;
    if (nbt == 0) { gfx_text(x, y, "  (none)", s, PANIC_FG); y += lh; }
    for (int i = 0; i < nbt; i++) {
        ksnprintf(line, sizeof line, "  #%d  0x%016lx", i, (unsigned long)bt[i]);
        gfx_text(x, y, line, s, PANIC_FG); y += lh;
    }
    y += 8;
    gfx_text(x, y, "Debug: addr2line -e build/yazan.elf <RIP>", s, PANIC_DIM);
}

void kpanic_exception(const regs_t *r) {
    uint64_t cr2, cr3;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    panic_exception_screen(r, cr2, cr3);
    cpu_halt_forever();
}
