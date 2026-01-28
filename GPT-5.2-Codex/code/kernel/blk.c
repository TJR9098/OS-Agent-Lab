#include <stdint.h>

#include "blk.h"
#include "fdt.h"
#include "log.h"
#include "memlayout.h"
#include "string.h"
#include "virtio_blk.h"

#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_DEV_BLK 2U

static struct virtio_blk_dev g_root_dev;
static uint8_t g_bounce[512] __attribute__((aligned(16)));

static inline uint32_t mmio_read32(uint64_t addr) {
  return *(volatile uint32_t *)addr;
}

static size_t scan_fixed_virtio_bases(uint64_t *out, size_t max_out) {
  size_t count = 0;
  for (int i = 7; i >= 0 && count < max_out; i--) {
    uint64_t base = VIRTIO0_BASE + (uint64_t)(uint32_t)i * 0x1000U;
    if (mmio_read32(base + 0x000) == 0x74726976U) {
      out[count++] = base;
    }
  }
  return count;
}

int blk_init(const void *dtb, uint32_t root_index) {
  uint64_t bases[8];
  uint64_t blk_bases[8];
  size_t count = 0;
  size_t blk_count = 0;
  size_t i;

  if (dtb && fdt_find_virtio_mmio(dtb, bases, 8, &count) == 0) {
    /* DTB path succeeded. */
  } else {
    if (dtb) {
      log_error("blk: dtb virtio scan failed, fallback to fixed list");
    } else {
      log_info("blk: dtb missing, using fixed virtio-mmio list");
    }
    count = scan_fixed_virtio_bases(bases, 8);
  }

  for (i = 0; i < count; i++) {
    uint64_t base = bases[i];
    if (mmio_read32(base + VIRTIO_MMIO_DEVICE_ID) == VIRTIO_DEV_BLK) {
      blk_bases[blk_count++] = base;
    }
  }

  if (blk_count == 0) {
    log_error("blk: no virtio-blk devices found");
    return -1;
  }

  uint32_t selected = root_index;
#ifdef CONFIG_STAGE2_TEST
  int fallback = 0;
#endif
  if (selected >= blk_count) {
    selected = 0;
#ifdef CONFIG_STAGE2_TEST
    fallback = 1;
#endif
  }
#ifdef CONFIG_STAGE2_TEST
  log_info("blk: devices=%lu requested=%u selected=%u%s",
           (uint64_t)blk_count,
           root_index,
           selected,
           fallback ? " fallback" : "");
#endif

  if (virtio_blk_init(&g_root_dev, blk_bases[selected]) != 0) {
    log_error("blk: init failed for base 0x%lx", blk_bases[selected]);
    return -1;
  }

  log_info("blk: root virtio-blk base=0x%lx capacity=%lu sectors",
           blk_bases[selected], g_root_dev.capacity_sectors);

  return 0;
}

int blk_read(uint64_t lba, void *dst, uint32_t count) {
  uint32_t i;
  uint8_t *p = (uint8_t *)dst;

  for (i = 0; i < count; i++) {
    if (virtio_blk_read(&g_root_dev, lba + i, g_bounce, 1) != 0) {
      return -1;
    }
    memcpy(p + i * 512U, g_bounce, 512U);
  }
  return 0;
}

int blk_write(uint64_t lba, const void *src, uint32_t count) {
  uint32_t i;
  const uint8_t *p = (const uint8_t *)src;

  for (i = 0; i < count; i++) {
    memcpy(g_bounce, p + i * 512U, 512U);
    if (virtio_blk_write(&g_root_dev, lba + i, g_bounce, 1) != 0) {
      return -1;
    }
  }
  return 0;
}

uint64_t blk_capacity_sectors(void) {
  return g_root_dev.capacity_sectors;
}
