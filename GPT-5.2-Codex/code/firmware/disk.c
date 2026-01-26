#include <stddef.h>
#include <stdint.h>

#include "disk.h"
#include "string.h"
#include "virtio.h"

#define SECTOR_SIZE 512U

static uint8_t g_sector_buf[SECTOR_SIZE] __attribute__((aligned(SECTOR_SIZE)));

int disk_init(uint64_t virtio_base) {
  return virtio_blk_init(virtio_base);
}

int disk_read_bytes(uint64_t offset, void *dst, size_t len) {
  uint8_t *out = (uint8_t *)dst;

  while (len > 0) {
    uint64_t sector = offset / SECTOR_SIZE;
    uint32_t sector_off = (uint32_t)(offset % SECTOR_SIZE);
    uint32_t chunk = SECTOR_SIZE - sector_off;

    if (chunk > len) {
      chunk = (uint32_t)len;
    }

    if (virtio_blk_read(sector, g_sector_buf, 1) != 0) {
      return -1;
    }

    memcpy(out, g_sector_buf + sector_off, chunk);

    out += chunk;
    offset += chunk;
    len -= chunk;
  }

  return 0;
}
