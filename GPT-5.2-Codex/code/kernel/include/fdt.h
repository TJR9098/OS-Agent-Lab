#pragma once

#include <stdint.h>
#include <stddef.h>

#define FDT_MAGIC 0xd00dfeedU

int fdt_get_memory(const void *dtb, uint64_t *base, uint64_t *size);
int fdt_find_virtio_mmio(const void *dtb, uint64_t *out_bases, size_t max_bases, size_t *out_count);
