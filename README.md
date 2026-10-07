# Yazan OS

نظام تشغيل x86_64 حقيقي يُبنى من الصفر ويقلع كملف ISO على QEMU وVirtualBox (وبإذن الله على أجهزة حقيقية لاحقًا).

## الحالة: المرحلة 2 ✅ — IDT + استثناءات + Console

(المرحلة 1: Boot → Kernel → شاشة Yazan OS ✅ — اختُبرت على VirtualBox)

| الجزء | الملف | الوصف |
|---|---|---|
| Bootloader | GRUB 2 (Multiboot2) + `boot/boot.S` | GRUB يحمّل النواة ويجهّز Framebuffer؛ `boot.S` يتحقق من CPUID/Long Mode، يبني Page Tables (4GB identity map)، يفعّل PAE+LME+Paging، يحمّل GDT 64-bit ويقفز إلى C |
| Kernel | `kernel/kmain.c` | يقرأ معلومات الإقلاع (Multiboot2): اسم الـBootloader، خريطة الذاكرة، الـFramebuffer، والـCPU |
| Graphics | `kernel/gfx.c` | رسم بكسلات، مستطيلات، تدرّجات، نص 8x8 — بدون FPU وبدون Heap |
| Header/UI | `kernel/splash.c`, `kernel/boot_ui.c` | شريط Yazan OS بشعار أصلي + منطقة Console |
| Console | `kernel/console.c` | Console نصي فوق الـFramebuffer مع scrolling و`kprintf`/`kok`/`kwarn` (يطبع أيضًا على serial) |
| IDT | `kernel/idt.c`, `kernel/isr.S` | 32 استثناء CPU؛ `#BP` يُعالَج ويكمل (اختبار ذاتي)، والباقي Panic |
| Panic | `kernel/panic.c` | شاشة Panic مع كل الـregisters وCR2/CR3 وBacktrace + سجل serial |
| Debug | `kernel/serial.c` | سجل على COM1 (يظهر في الطرفية مع QEMU) |
| Fallback | `kernel/vgatext.c` | وضع نص 80x25 إن لم يتوفر Framebuffer |

## المتطلبات (Ubuntu / Debian / WSL2)

```bash
./scripts/setup-ubuntu.sh
# أو يدويًا:
sudo apt install build-essential grub-pc-bin grub-efi-amd64-bin xorriso mtools qemu-system-x86
```

> macOS: استخدم مترجمًا متقاطعًا `x86_64-elf-gcc` ثم `make CROSS=x86_64-elf-`.
> Windows: استخدم WSL2 للبناء، ثم شغّل الـISO في VirtualBox من Windows.

## البناء والتشغيل

```bash
make            # يبني build/yazan.elf
make check      # يتحقق من الـELF وترويسة Multiboot2
make iso        # ينتج build/yazanos.iso
make run        # تشغيل في QEMU (سجل serial في نفس الطرفية)
make run-uefi   # تشغيل في QEMU بنمط UEFI (يحتاج ovmf)
make vbox       # ينشئ ويشغّل VM في VirtualBox
make iso FAULT=pf  # ISO يتعمّد الانهيار ليُظهر شاشة Panic (FAULT = div0 | ud | pf | gp)
make test       # اختبار كود الاستثناءات على جهازك بدون VM
make preview    # يرسم الواجهة على جهازك إلى build/preview-*.ppm بدون VM
```

أمر QEMU اليدوي:
```bash
qemu-system-x86_64 -cdrom build/yazanos.iso -m 256M -vga std -serial stdio
```

VirtualBox يدويًا: New → Type: Other / Version: Other/Unknown (64-bit) → RAM 256MB → بدون قرص → Settings → Storage → اختر `yazanos.iso` → Start.
(يجب أن يكون Graphics Controller = VBoxVGA، وبدون EFI في هذه المرحلة.)

## النتيجة المتوقعة عند الإقلاع
شريط علوي بشعار Y وعنوان YAZAN OS، وتحته Console فيه أسطر `[ OK ]` للـBootloader والـCPU والشاشة والذاكرة والـKernel والـIDT والاختبار الذاتي، وآخرها `Phase 2 complete`.

ومع `FAULT=pf` تظهر بدلها شاشة حمراء `KERNEL PANIC` فيها نوع الاستثناء والـregisters.
إذا ظهر على الشاشة `ERR:M` أو `ERR:C` أو `ERR:L` فهذا خطأ مبكر من `boot.S` (Magic / CPUID / Long Mode).

## هيكل المشروع
```
boot/boot.S        مدخل Multiboot2 + الانتقال إلى 64-bit
boot/grub.cfg      إعداد GRUB داخل الـISO
linker.ld          النواة عند 1MB
kernel/            كود النواة بلغة C
tools/preview.c    معاينة الواجهة على الجهاز المضيف
tools/isr_test.c   اختبار stubs الاستثناءات على الجهاز المضيف
scripts/qemu-test.sh  اختبار إقلاع آلي بـQEMU (يستعمله GitHub Actions)
scripts/           إعداد البيئة + VirtualBox
Makefile
```

## حدود معروفة في المرحلة 1 (مقصودة)
- لا يوجد IST/TSS بعد: Stack overflow داخل النواة سيسبب Double Fault ثم Triple Fault (إعادة تشغيل). سيُضاف مع إدارة الذاكرة.
- الـPIC مقفول بالكامل (لا مقاطعات عتاد) حتى مرحلة Timer/Keyboard.
- النواة والـPaging بنمط identity-map لأول 4GB؛ الفصل بين Kernel/User (Higher-half + Ring 3) يأتي مع إدارة الذاكرة والعمليات.
- الـFramebuffer مخزّن مؤقتًا بنمط WB الافتراضي؛ سنضبط PAT/Write-Combining لاحقًا للأجهزة الحقيقية.
- الـFramebuffer يجب أن يكون تحت 4GB (كل الأجهزة الشائعة وVM كذلك).

## خارطة الطريق
1. ✅ Boot → Kernel + شاشة Yazan OS (اختُبر على VirtualBox)
2. ✅ IDT + استثناءات + Panic حقيقي + Console
3. ⏭ Physical Memory Manager + Paging + Heap (kmalloc)
4. PIC/APIC + Timer (PIT/HPET) + Keyboard
5. Framebuffer مزدوج (Back buffer) ورسم أسرع
6. Mouse (PS/2)
7. GUI toolkit
8. Window Manager
9. Desktop + Taskbar + Start menu
10. File System (RAM disk ثم FAT32)
11. التطبيقات (Terminal, File Manager, Settings, Text Editor, Calculator, About)
12. تحسين الأداء والاستقرار (Ring 3, syscalls, Threads)
