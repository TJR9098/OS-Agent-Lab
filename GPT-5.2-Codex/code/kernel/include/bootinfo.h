#pragma once

#include <stdint.h>

#define BOOTINFO_MAGIC 0x424f4f54494e464fULL

struct bootinfo {
  uint64_t magic;
  uint64_t version;
  uint64_t mem_base;
  uint64_t mem_size;

  uint64_t dtb_paddr;
  uint64_t initrd_paddr;
  uint64_t initrd_size;

  uint64_t cmdline_paddr;
  uint32_t cmdline_len;

  uint32_t log_level;
  uint32_t tick_hz;
  uint32_t hart_count;
  uint32_t root_disk_index;
  uint32_t reserved;
  uint64_t timebase_hz;
};
