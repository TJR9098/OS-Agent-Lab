#pragma once

#include <stdint.h>
#include <stddef.h>

#define VIRTIO_DEV_BLK 2U

int virtio_blk_init(uint64_t base);
int virtio_blk_read(uint64_t sector, void *dst, uint32_t count);