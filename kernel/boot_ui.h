#pragma once
#include "bootinfo.h"
/* Header band + console; prints the boot-info lines. Call once framebuffer is ready. */
void boot_ui_start(const bootinfo_t *bi);
