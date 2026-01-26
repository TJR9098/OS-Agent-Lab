#pragma once

#include <stdint.h>

int blk_init(const void *dtb, uint32_t root_index);
int blk_read(uint64_t lba, void *dst, uint32_t count);
int blk_write(uint64_t lba, const void *src, uint32_t count);
uint64_t blk_capacity_sectors(void);
