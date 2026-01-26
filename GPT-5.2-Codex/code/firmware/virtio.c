#include <stdint.h>

#include "virtio.h"
#include "printf.h"

#define VIRTIO_MMIO_MAGIC_VALUE 0x000
#define VIRTIO_MMIO_VERSION 0x004
#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_MMIO_VENDOR_ID 0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028
#define VIRTIO_MMIO_QUEUE_SEL 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM 0x038
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03c
#define VIRTIO_MMIO_QUEUE_PFN 0x040
#define VIRTIO_MMIO_QUEUE_READY 0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064
#define VIRTIO_MMIO_STATUS 0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW 0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW 0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH 0x0a4

#define VIRTIO_STATUS_ACKNOWLEDGE 0x01
#define VIRTIO_STATUS_DRIVER 0x02
#define VIRTIO_STATUS_DRIVER_OK 0x04
#define VIRTIO_STATUS_FEATURES_OK 0x08
#define VIRTIO_STATUS_FAILED 0x80

#define VIRTQ_DESC_F_NEXT 0x01
#define VIRTQ_DESC_F_WRITE 0x02

#define VIRTIO_BLK_T_IN 0

#define QUEUE_SIZE 1024
#define QUEUE_ALIGN 4096

struct virtq_desc {
  uint64_t addr;
  uint32_t len;
  uint16_t flags;
  uint16_t next;
};

struct virtq_avail {
  uint16_t flags;
  uint16_t idx;
  uint16_t ring[QUEUE_SIZE];
  uint16_t unused;
};

struct virtq_used_elem {
  uint32_t id;
  uint32_t len;
};

struct virtq_used {
  uint16_t flags;
  uint16_t idx;
  struct virtq_used_elem ring[QUEUE_SIZE];
  uint16_t avail_event;
};

struct virtio_blk_req {
  uint32_t type;
  uint32_t reserved;
  uint64_t sector;
};

static uint64_t g_base;

#define VRING_DESC_SIZE (sizeof(struct virtq_desc) * QUEUE_SIZE)
#define VRING_AVAIL_SIZE (sizeof(struct virtq_avail))
#define VRING_USED_SIZE (sizeof(struct virtq_used))
#define VRING_USED_OFFSET ((VRING_DESC_SIZE + VRING_AVAIL_SIZE + QUEUE_ALIGN - 1) & ~(QUEUE_ALIGN - 1))
#define VRING_SIZE (VRING_USED_OFFSET + VRING_USED_SIZE)

static uint8_t g_vring[VRING_SIZE] __attribute__((aligned(QUEUE_ALIGN)));

static struct virtq_desc *g_desc;
static volatile struct virtq_avail *g_avail;
static volatile struct virtq_used *g_used;
static uint16_t g_last_used;

static struct virtio_blk_req g_req __attribute__((aligned(16)));
static uint8_t g_status __attribute__((aligned(4)));

static inline void mmio_write32(uint64_t base, uint32_t off, uint32_t val) {
  *(volatile uint32_t *)(base + off) = val;
}

static inline uint32_t mmio_read32(uint64_t base, uint32_t off) {
  return *(volatile uint32_t *)(base + off);
}

static inline void mb(void) {
  __asm__ volatile("fence rw, rw" ::: "memory");
}

int virtio_blk_init(uint64_t base) {
  uint32_t magic;
  uint32_t version;
  uint32_t dev_id;
  uint32_t status;
  uint32_t max_queue;

  magic = mmio_read32(base, VIRTIO_MMIO_MAGIC_VALUE);
  version = mmio_read32(base, VIRTIO_MMIO_VERSION);
  dev_id = mmio_read32(base, VIRTIO_MMIO_DEVICE_ID);

  if (magic != 0x74726976U || dev_id != VIRTIO_DEV_BLK) {
    return -1;
  }

  g_base = base;

  mmio_write32(base, VIRTIO_MMIO_STATUS, 0);
  status = VIRTIO_STATUS_ACKNOWLEDGE;
  mmio_write32(base, VIRTIO_MMIO_STATUS, status);
  status |= VIRTIO_STATUS_DRIVER;
  mmio_write32(base, VIRTIO_MMIO_STATUS, status);

  mmio_write32(base, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 0);
  uint32_t features_lo = mmio_read32(base, VIRTIO_MMIO_DEVICE_FEATURES);
  mmio_write32(base, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
  uint32_t features_hi = mmio_read32(base, VIRTIO_MMIO_DEVICE_FEATURES);
  fw_printf("virtio: features lo=0x%x hi=0x%x\n", features_lo, features_hi);

  mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
  mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES, 0);

  mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
  if (version >= 2) {
    mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES, 1U);
  } else {
    mmio_write32(base, VIRTIO_MMIO_DRIVER_FEATURES, 0);
  }

  if (version >= 2) {
    status |= VIRTIO_STATUS_FEATURES_OK;
    mmio_write32(base, VIRTIO_MMIO_STATUS, status);

    if ((mmio_read32(base, VIRTIO_MMIO_STATUS) & VIRTIO_STATUS_FEATURES_OK) == 0) {
      return -1;
    }
  }

  mmio_write32(base, VIRTIO_MMIO_QUEUE_SEL, 0);
  max_queue = mmio_read32(base, VIRTIO_MMIO_QUEUE_NUM_MAX);
  if (max_queue < QUEUE_SIZE) {
    return -1;
  }
  fw_printf("virtio: queue max=%u\n", max_queue);

  if (version >= 2) {
    g_desc = (struct virtq_desc *)(void *)g_vring;
    g_avail = (volatile struct virtq_avail *)(void *)(g_vring + VRING_DESC_SIZE);
    g_used = (volatile struct virtq_used *)(void *)(g_vring + VRING_USED_OFFSET);

    mmio_write32(base, VIRTIO_MMIO_QUEUE_READY, 0);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_NUM, QUEUE_SIZE);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_DESC_LOW, (uint32_t)(uintptr_t)g_desc);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_DESC_HIGH, (uint32_t)((uintptr_t)g_desc >> 32));
    mmio_write32(base, VIRTIO_MMIO_QUEUE_AVAIL_LOW, (uint32_t)(uintptr_t)g_avail);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_AVAIL_HIGH, (uint32_t)((uintptr_t)g_avail >> 32));
    mmio_write32(base, VIRTIO_MMIO_QUEUE_USED_LOW, (uint32_t)(uintptr_t)g_used);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_USED_HIGH, (uint32_t)((uintptr_t)g_used >> 32));
    mmio_write32(base, VIRTIO_MMIO_QUEUE_READY, 1);
    fw_printf("virtio: desc=%p avail=%p used=%p\n",
              (void *)g_desc, (void *)g_avail, (void *)g_used);
  } else if (version == 1) {
    g_desc = (struct virtq_desc *)(void *)g_vring;
    g_avail = (volatile struct virtq_avail *)(void *)(g_vring + VRING_DESC_SIZE);
    g_used = (volatile struct virtq_used *)(void *)(g_vring + VRING_USED_OFFSET);

    mmio_write32(base, VIRTIO_MMIO_GUEST_PAGE_SIZE, QUEUE_ALIGN);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_NUM, QUEUE_SIZE);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_ALIGN, QUEUE_ALIGN);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_PFN, (uint32_t)(((uintptr_t)g_vring) >> 12));
    fw_printf("virtio: vring=%p desc=%p avail=%p used=%p pfn=0x%x\n",
              (void *)g_vring, (void *)g_desc, (void *)g_avail, (void *)g_used,
              (uint32_t)(((uintptr_t)g_vring) >> 12));
  } else {
    return -1;
  }

  g_avail->flags = 0;
  g_avail->idx = 0;
  g_used->flags = 0;
  g_used->idx = 0;
  g_last_used = 0;

  status |= VIRTIO_STATUS_DRIVER_OK;
  mmio_write32(base, VIRTIO_MMIO_STATUS, status);

  return 0;
}

static int virtio_blk_read_one(uint64_t sector, void *dst) {
  g_req.type = VIRTIO_BLK_T_IN;
  g_req.reserved = 0;
  g_req.sector = sector;
  g_status = 0xff;

  g_desc[0].addr = (uint64_t)(uintptr_t)&g_req;
  g_desc[0].len = sizeof(g_req);
  g_desc[0].flags = VIRTQ_DESC_F_NEXT;
  g_desc[0].next = 1;

  g_desc[1].addr = (uint64_t)(uintptr_t)dst;
  g_desc[1].len = 512;
  g_desc[1].flags = VIRTQ_DESC_F_WRITE | VIRTQ_DESC_F_NEXT;
  g_desc[1].next = 2;

  g_desc[2].addr = (uint64_t)(uintptr_t)&g_status;
  g_desc[2].len = 1;
  g_desc[2].flags = VIRTQ_DESC_F_WRITE;
  g_desc[2].next = 0;

  mb();

  uint16_t idx = g_avail->idx;
  g_avail->ring[idx % QUEUE_SIZE] = 0;
  mb();
  g_avail->idx = (uint16_t)(idx + 1);
  mb();

  mmio_write32(g_base, VIRTIO_MMIO_QUEUE_NOTIFY, 0);

  {
    uint32_t spins = 0;
    while (g_used->idx == g_last_used) {
      if (++spins > 10000000U) {
        fw_printf("virtio: timeout used=%u last=%u avail=%u\n",
                  g_used->idx, g_last_used, g_avail->idx);
        return -1;
      }
    }
  }
  mb();
  g_last_used = g_used->idx;

  return (g_status == 0) ? 0 : -1;
}

int virtio_blk_read(uint64_t sector, void *dst, uint32_t count) {
  uint32_t i;
  uint8_t *p = (uint8_t *)dst;

  for (i = 0; i < count; i++) {
    if (virtio_blk_read_one(sector + i, p + i * 512U) != 0) {
      return -1;
    }
  }
  return 0;
}
