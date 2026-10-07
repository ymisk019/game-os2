/* Yazan OS - linker script. Kernel is loaded at 1 MiB (identity mapped). */
ENTRY(_start)

SECTIONS
{
    . = 1M;
    __kernel_start = .;

    /* The Multiboot2 header must be within the first 32 KiB of the image */
    .multiboot2 : ALIGN(8) { KEEP(*(.multiboot2)) }

    .text : ALIGN(4K) { *(.text.boot) *(.text .text.*) }
    .rodata : ALIGN(4K) { *(.rodata .rodata.*) }
    .data : ALIGN(4K) { *(.data .data.*) }
    .bss : ALIGN(4K) { *(COMMON) *(.bss .bss.*) }

    . = ALIGN(4K);
    __kernel_end = .;

    /DISCARD/ : { *(.eh_frame) *(.comment) *(.note*) }
}
