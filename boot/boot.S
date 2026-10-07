/* ============================================================================
 * Yazan OS - boot/boot.S
 * Multiboot2 entry point (32-bit protected mode) -> x86_64 long mode -> kmain
 *
 * GRUB loads the kernel, switches to 32-bit protected mode, sets up a
 * framebuffer (see the framebuffer tag below) and jumps to _start with:
 *     EAX = 0x36D76289 (Multiboot2 magic)
 *     EBX = physical address of the Multiboot2 info structure
 * We then: validate -> check CPUID/long mode -> build page tables ->
 * enable PAE+LME+PG -> load a 64-bit GDT -> far-jump to 64-bit code -> kmain().
 * ========================================================================== */
.intel_syntax noprefix

.set MB2_MAGIC,      0xE85250D6
.set MB2_BOOT_MAGIC, 0x36D76289
.set FB_WIDTH,       1024
.set FB_HEIGHT,      768
.set FB_DEPTH,       32

/* ---------------------------- Multiboot2 header --------------------------- */
.section .multiboot2, "a"
.align 8
mb2_header:
    .long MB2_MAGIC
    .long 0                                   /* architecture: i386 protected */
    .long mb2_header_end - mb2_header         /* header length */
    .long -(MB2_MAGIC + 0 + (mb2_header_end - mb2_header))   /* checksum */

    /* Framebuffer request tag (optional: falls back to VGA text if refused) */
    .align 8
    .word 5                                   /* type  = framebuffer */
    .word 1                                   /* flags = optional */
    .long 20                                  /* size */
    .long FB_WIDTH
    .long FB_HEIGHT
    .long FB_DEPTH

    /* End tag */
    .align 8
    .word 0
    .word 0
    .long 8
mb2_header_end:

/* ------------------------------ 32-bit entry ------------------------------ */
.section .text.boot, "ax"
.code32
.global _start
_start:
    cli
    cld
    mov esp, offset boot_stack_top

    cmp eax, MB2_BOOT_MAGIC
    jne .err_magic
    mov edi, ebx                 /* EDI = mbi (1st arg of kmain), kept safe */
    mov esi, eax                 /* ESI = magic (2nd arg of kmain) */

    /* ---- CPUID available? (flip EFLAGS.ID, bit 21) ---- */
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz .err_nocpuid

    /* ---- Long mode available? ---- */
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .err_nolong
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29            /* LM bit */
    jz .err_nolong

    /* ---- Zero the 6 page-table pages (PML4, PDPT, 4x PD) ---- */
    xor ecx, ecx
1:  mov dword ptr [p4_table + ecx*4], 0
    inc ecx
    cmp ecx, 6 * 1024
    jne 1b

    /* ---- Identity-map the first 4 GiB with 2 MiB pages ---- */
    mov eax, offset p3_table
    or  eax, 0x3                 /* present | writable */
    mov dword ptr [p4_table], eax

    xor ecx, ecx
2:  mov eax, ecx
    shl eax, 12                  /* each PD is 4 KiB */
    add eax, offset p2_tables
    or  eax, 0x3
    mov dword ptr [p3_table + ecx*8], eax
    inc ecx
    cmp ecx, 4
    jne 2b

    xor ecx, ecx
3:  mov eax, ecx
    shl eax, 21                  /* 2 MiB per entry */
    or  eax, 0x83                /* present | writable | huge */
    mov dword ptr [p2_tables + ecx*8], eax
    inc ecx
    cmp ecx, 4 * 512
    jne 3b

    /* ---- Enable PAE ---- */
    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    /* ---- Load PML4 ---- */
    mov eax, offset p4_table
    mov cr3, eax

    /* ---- Enable long mode (EFER.LME) ---- */
    mov ecx, 0xC0000080
    rdmsr
    or  eax, 1 << 8
    wrmsr

    /* ---- Enable paging (+ write-protect) ---- */
    mov eax, cr0
    or  eax, (1 << 31) | (1 << 16)
    mov cr0, eax

    /* ---- Load 64-bit GDT and far-jump into long mode ---- */
    lgdt [gdt64_ptr]
    push 0x08
    push offset long_mode_start
    retf

/* ------------------------- Early error reporting ------------------------- */
.err_magic:   mov al, 'M'
              jmp .err
.err_nocpuid: mov al, 'C'
              jmp .err
.err_nolong:  mov al, 'L'
.err:
    mov dword ptr [0xB8000], 0x4F524F45   /* "ER" white on red */
    mov dword ptr [0xB8004], 0x4F3A4F52   /* "R:" */
    mov ah, 0x4F
    mov word ptr [0xB8008], ax            /* error code letter */
.halt32:
    cli
    hlt
    jmp .halt32

/* ------------------------------ 64-bit entry ------------------------------ */
.code64
long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, offset boot_stack_top
    xor ebp, ebp
    call kmain                   /* kmain(mbi = RDI, magic = RSI) */
.halt64:
    cli
    hlt
    jmp .halt64

/* ---------------------------------- GDT ---------------------------------- */
.section .rodata
.align 16
gdt64:
    .quad 0x0000000000000000     /* null */
    .quad 0x00AF9A000000FFFF     /* 0x08: 64-bit code, ring 0 */
    .quad 0x00CF92000000FFFF     /* 0x10: data, ring 0 */
gdt64_end:
gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .long gdt64

/* ----------------------- Page tables and boot stack ----------------------- */
.section .bss
.align 4096
p4_table:  .skip 4096
p3_table:  .skip 4096
p2_tables: .skip 4096 * 4
.align 16
boot_stack_bottom:
    .skip 32768
boot_stack_top:

.section .note.GNU-stack, "", @progbits
