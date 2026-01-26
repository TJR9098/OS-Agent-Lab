#pragma once

#include <stdint.h>

struct virtio_blk_dev {
  uint64_t base;
  uint32_t version;
  uint16_t last_used;
  uint64_t capacity_sectors;
};

int virtio_blk_init(struct virtio_blk_dev *dev, uint64_t base);
int virtio_blk_read(struct virtio_blk_dev *dev, uint64_t sector, void *dst, uint32_t count);
int virtio_blk_write(struct virtio_blk_dev *dev, uint64_t sector, const void *src, uint32_t count);
