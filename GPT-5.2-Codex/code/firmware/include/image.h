#pragma once

#include <stdint.h>

#define IMAGE_MAGIC 0x474d495643534952ULL
#define IMAGE_VERSION 1U

struct image_header {
  uint64_t magic;
  uint32_t version;
  uint32_t header_size;
  uint64_t kernel_offset;
  uint64_t kernel_size;
  uint64_t initrd_offset;
  uint64_t initrd_size;
  uint64_t flags;
};