#pragma once

#include <stdint.h>
#include <stddef.h>

#define FDT_MAGIC 0xd00dfeedU

int fdt_get_memory(const void *dtb, uint64_t *base, uint64_t *size);
int fdt_find_virtio_mmio(const void *dtb, uint64_t *out_bases, size_t max_bases, size_t *out_count);
int fdt_get_timebase(const void *dtb, uint32_t *out_hz);
uint32_t fdt_total_size(const void *dtb);
