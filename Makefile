# ============================================================================
# Yazan OS - build system
#   make               build build/yazan.elf
#   make iso           build build/yazanos.iso (needs grub-mkrescue + xorriso)
#   make iso FAULT=pf  ISO that crashes on purpose to show the panic screen
#                      (FAULT = div0 | ud | pf | gp)
#   make run           boot the ISO in QEMU (serial log goes to this terminal)
#   make run-uefi      boot in QEMU with UEFI firmware (needs OVMF)
#   make vbox          create + start a VirtualBox VM
#   make preview       render the UI on the host to build/preview-*.ppm (no VM)
#   make test          host tests (exception stubs) - no VM needed
#   make check         sanity-check the ELF + Multiboot2 header
# ============================================================================
CROSS   ?=
CC      := $(CROSS)gcc
LD      := $(CROSS)ld
OBJDUMP := $(CROSS)objdump

FAULT    ?=
FAULT_ID := $(if $(filter div0,$(FAULT)),1,$(if $(filter ud,$(FAULT)),2,$(if $(filter pf,$(FAULT)),3,$(if $(filter gp,$(FAULT)),4,0))))

CFLAGS  := -std=gnu11 -O2 -g -Wall -Wextra -Werror \
           -ffreestanding -fno-stack-protector -fno-pic -fno-pie \
           -mno-red-zone -mgeneral-regs-only -mno-mmx -mno-sse -mno-sse2 \
           -fcf-protection=none -fno-asynchronous-unwind-tables -fno-omit-frame-pointer \
           -fno-tree-loop-distribute-patterns -DTEST_FAULT=$(FAULT_ID) -Ikernel
LDFLAGS := -nostdlib -static -no-pie -z max-page-size=0x1000 -z noexecstack -T linker.ld

BUILD   := build
KERNEL  := $(BUILD)/yazan.elf
ISO     := $(BUILD)/yazanos.iso
ISODIR  := $(BUILD)/iso

C_SRC   := $(wildcard kernel/*.c)
S_SRC   := $(wildcard kernel/*.S)
OBJS    := $(BUILD)/boot.o $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SRC)) $(patsubst kernel/%.S,$(BUILD)/%.o,$(S_SRC))

.PHONY: all iso run run-uefi vbox preview test check clean FORCE
all: $(KERNEL)

$(BUILD):
	mkdir -p $(BUILD)

# Rebuild C objects when the compiler flags change (e.g. FAULT=...)
$(BUILD)/flags: FORCE | $(BUILD)
	@echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@
FORCE:

$(BUILD)/boot.o: boot/boot.S | $(BUILD)
	$(CC) -c $< -o $@

$(BUILD)/%.o: kernel/%.S | $(BUILD)
	$(CC) -c $< -o $@

$(BUILD)/%.o: kernel/%.c $(wildcard kernel/*.h) $(BUILD)/flags | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

iso: $(ISO)
$(ISO): $(KERNEL) boot/grub.cfg
	rm -rf $(ISODIR)
	mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL) $(ISODIR)/boot/yazan.elf
	cp boot/grub.cfg $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISODIR)

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 256M -vga std -serial stdio

run-uefi: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 256M -vga std -serial stdio \
	  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE.fd

vbox: $(ISO)
	scripts/vbox.sh $(ISO)

preview: | $(BUILD)
	gcc -std=gnu11 -O1 -Wall -Wextra -DHOST_PREVIEW -Ikernel \
	    tools/preview.c kernel/gfx.c kernel/splash.c kernel/console.c kernel/boot_ui.c \
	    kernel/panic.c kernel/vgatext.c kernel/printf.c -o $(BUILD)/preview
	$(BUILD)/preview $(BUILD)/preview 1024 768

test: | $(BUILD)
	gcc -O1 -g -fno-omit-frame-pointer -no-pie -Ikernel tools/isr_test.c kernel/isr.S -o $(BUILD)/isr_test
	$(BUILD)/isr_test

check: $(KERNEL)
	@$(OBJDUMP) -f $(KERNEL) | grep -E 'file format|start address'
	@$(OBJDUMP) -h $(KERNEL) | grep -E 'multiboot2|\.text|\.bss'
	@if command -v grub-file >/dev/null; then \
	    grub-file --is-x86-multiboot2 $(KERNEL) && echo "grub-file: valid Multiboot2 image"; \
	  else echo "(grub-file not installed - skipped Multiboot2 validation)"; fi

clean:
	rm -rf $(BUILD)
