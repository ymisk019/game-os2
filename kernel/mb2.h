#pragma once
#include <stdint.h>

#define MB2_BOOT_MAGIC 0x36D76289

#define MB2_TAG_END      0
#define MB2_TAG_CMDLINE  1
#define MB2_TAG_LOADER   2
#define MB2_TAG_MMAP     6
#define MB2_TAG_FB       8

#define MB2_MMAP_AVAILABLE 1
#define MB2_FB_INDEXED 0
#define MB2_FB_RGB     1
#define MB2_FB_TEXT    2

typedef struct { uint32_t type, size; } __attribute__((packed)) mb2_tag_t;

typedef struct {
    mb2_tag_t tag;
    uint32_t  entry_size, entry_version;
} __attribute__((packed)) mb2_mmap_tag_t;

typedef struct {
    uint64_t base, len;
    uint32_t type, reserved;
} __attribute__((packed)) mb2_mmap_entry_t;

typedef struct {
    mb2_tag_t tag;
    uint64_t  addr;
    uint32_t  pitch, width, height;
    uint8_t   bpp, type;
    uint16_t  reserved;
    uint8_t   red_pos, red_size, green_pos, green_size, blue_pos, blue_size;
} __attribute__((packed)) mb2_fb_tag_t;
