#include <stdint.h>
#include <stddef.h>

#include "bootinfo.h"
#include "disk.h"
#include "fdt.h"
#include "image.h"
#include "memlayout.h"
#include "printf.h"
#include "string.h"
#include "uart.h"
#include "virtio.h"

#define DTB_MAX_SIZE (64U * 1024U)
#define IMAGE_HEADER_OFFSET 0ULL
#define DEFAULT_TICK_HZ 100U
#define DEFAULT_MEM_BASE DRAM_BASE
#define DEFAULT_MEM_SIZE 0x40000000ULL /* 1 GiB */
#define DEFAULT_TIMEBASE_HZ 10000000U

extern void mtrap(void);
extern char trap_stack_top[];

static uint8_t g_dtb_copy[DTB_MAX_SIZE];
static struct bootinfo g_bootinfo;

static void panic(const char *msg) {
  fw_printf("fw: panic: %s\n", msg);
  for (;;) {
    __asm__ volatile("wfi");
  }
}

static inline uint32_t mmio_read32(uint64_t addr) {
  return *(volatile uint32_t *)addr;
}

static inline void clint_write_mtimecmp(uint64_t hartid, uint64_t val) {
  *(volatile uint64_t *)(CLINT_BASE + 0x4000UL + hartid * 8UL) = val;
}

static int image_header_valid(const struct image_header *hdr) {
  return hdr && hdr->magic == IMAGE_MAGIC && hdr->version == IMAGE_VERSION;
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

static uint64_t find_virtio_blk_base(const void *dtb, int have_dtb) {
  uint64_t bases[8];
  size_t count = 0;
  size_t i;

  if (have_dtb) {
    if (fdt_find_virtio_mmio(dtb, bases, 8, &count) != 0) {
      fw_printf("fw: dtb virtio scan failed, fallback to fixed list\n");
      count = scan_fixed_virtio_bases(bases, 8);
    }
  } else {
    count = scan_fixed_virtio_bases(bases, 8);
  }

  uint64_t first_blk = 0;
  for (i = 0; i < count; i++) {
    uint64_t base = bases[i];
    uint32_t magic = mmio_read32(base + 0x000);
    uint32_t dev_id = mmio_read32(base + 0x008);
    if (magic != 0x74726976U || dev_id != VIRTIO_DEV_BLK) {
      continue;
    }
    if (!have_dtb) {
      fw_printf("fw: probe blk base=0x%lx\n", base);
    }
    if (first_blk == 0) {
      first_blk = base;
    }
    if (disk_init(base) != 0) {
      continue;
    }
    struct image_header probe;
    if (disk_read_bytes(IMAGE_HEADER_OFFSET, &probe, sizeof(probe)) == 0 && image_header_valid(&probe)) {
      if (!have_dtb) {
        fw_printf("fw: selected blk base=0x%lx (image header)\n", base);
      }
      return base;
    }
  }

  return first_blk ? first_blk : VIRTIO0_BASE;
}

#define EI_NIDENT 16
#define EM_RISCV 243
#define PT_LOAD 1

struct elf64_ehdr {
  unsigned char e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
};

struct elf64_phdr {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
};

static void load_elf(uint64_t file_off,
                     uint64_t file_size,
                     uint64_t mem_base,
                     uint64_t mem_size,
                     uint64_t *entry_out,
                     uint64_t *end_out) {
  struct elf64_ehdr ehdr;
  uint16_t i;
  uint64_t kernel_end = 0;

  if (file_size < sizeof(ehdr)) {
    panic("kernel image too small");
  }

  if (disk_read_bytes(file_off, &ehdr, sizeof(ehdr)) != 0) {
    panic("read elf header failed");
  }

  if (ehdr.e_ident[0] != 0x7f || ehdr.e_ident[1] != 'E' || ehdr.e_ident[2] != 'L' || ehdr.e_ident[3] != 'F') {
    panic("bad elf magic");
  }
  if (ehdr.e_ident[4] != 2 || ehdr.e_ident[5] != 1) {
    panic("unsupported elf class/data");
  }
  if (ehdr.e_machine != EM_RISCV || ehdr.e_phentsize != sizeof(struct elf64_phdr)) {
    panic("unsupported elf machine/phdr");
  }

  for (i = 0; i < ehdr.e_phnum; i++) {
    struct elf64_phdr phdr;
    uint64_t phoff = file_off + ehdr.e_phoff + (uint64_t)i * ehdr.e_phentsize;

    if (disk_read_bytes(phoff, &phdr, sizeof(phdr)) != 0) {
      panic("read phdr failed");
    }

    if (phdr.p_type != PT_LOAD) {
      continue;
    }

    if (mem_size != 0) {
      uint64_t end = phdr.p_paddr + phdr.p_memsz;
      if (phdr.p_paddr < mem_base || end > mem_base + mem_size) {
        panic("kernel segment out of range");
      }
    }

    if (disk_read_bytes(file_off + phdr.p_offset, (void *)(uintptr_t)phdr.p_paddr, (size_t)phdr.p_filesz) != 0) {
      panic("load segment failed");
    }
    if (phdr.p_memsz > phdr.p_filesz) {
      uint64_t bss_start = phdr.p_paddr + phdr.p_filesz;
      uint64_t bss_len = phdr.p_memsz - phdr.p_filesz;
      memset((void *)(uintptr_t)bss_start, 0, (size_t)bss_len);
    }

    if (phdr.p_paddr + phdr.p_memsz > kernel_end) {
      kernel_end = phdr.p_paddr + phdr.p_memsz;
    }
  }

  if (entry_out) {
    *entry_out = ehdr.e_entry;
  }
  if (end_out) {
    *end_out = kernel_end;
  }
}

static void enter_kernel(uint64_t hartid, uint64_t entry, uint64_t dtb_paddr, struct bootinfo *bi) {
  uint64_t mstatus;

  __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));
  mstatus &= ~(3UL << 11);
  mstatus &= ~(1UL << 20);
  mstatus |= (1UL << 11);
  mstatus |= (1UL << 3);
  __asm__ volatile("csrw mstatus, %0" :: "r"(mstatus));
  __asm__ volatile("csrw mepc, %0" :: "r"(entry));
  __asm__ volatile("csrw satp, zero");

  __asm__ volatile("mv a0, %0" :: "r"(hartid));
  __asm__ volatile("mv a1, %0" :: "r"(dtb_paddr));
  __asm__ volatile("mv a2, %0" :: "r"(bi));

  __asm__ volatile("mret");
}

void firmware_main(uint64_t hartid, uintptr_t dtb_ptr) {
  uint64_t mem_base = 0;
  uint64_t mem_size = 0;
  uint64_t virtio_base;
  uint64_t entry = 0;
  uint64_t kernel_end = 0;
  struct image_header hdr;
  uint32_t dtb_size = 0;
  uint32_t timebase_hz = 0;
  uint64_t initrd_paddr = 0;
  int have_dtb = 0;

  if (hartid != 0) {
    for (;;) {
      __asm__ volatile("wfi");
    }
  }

  uart_init();
  fw_printf("fw: hart=%lu dtb=%p\n", hartid, (void *)dtb_ptr);

#ifdef CONFIG_FORCE_DTB_NULL
  fw_printf("fw: forcing dtb=null for selftest\n");
  dtb_ptr = 0;
#endif

  /* Install a trap handler early so faults during probing are visible. */
  __asm__ volatile("csrw mtvec, %0" :: "r"(&mtrap));
  __asm__ volatile("csrw mscratch, %0" :: "r"(trap_stack_top));

  if (dtb_ptr != 0) {
    uint32_t total = fdt_total_size((const void *)dtb_ptr);
    if (total != 0 && total <= DTB_MAX_SIZE) {
      have_dtb = 1;
      dtb_size = total;
    } else {
      fw_printf("fw: dtb invalid (size=%u), using defaults\n", total);
    }
  }

  if (have_dtb) {
    if (fdt_get_memory((const void *)dtb_ptr, &mem_base, &mem_size) != 0) {
      fw_printf("fw: failed to read memory from dtb, using defaults\n");
      mem_base = DEFAULT_MEM_BASE;
      mem_size = DEFAULT_MEM_SIZE;
    }
  } else {
    mem_base = DEFAULT_MEM_BASE;
    mem_size = DEFAULT_MEM_SIZE;
    fw_printf("fw: dtb missing, mem_base=0x%lx mem_size=0x%lx\n", mem_base, mem_size);
  }

  virtio_base = find_virtio_blk_base((const void *)dtb_ptr, have_dtb);
  fw_printf("fw: virtio-blk base=0x%lx\n", virtio_base);
  fw_printf("fw: virtio magic=0x%x version=%u dev=%u\n",
            mmio_read32(virtio_base + 0x000),
            mmio_read32(virtio_base + 0x004),
            mmio_read32(virtio_base + 0x008));
  {
    uint32_t cap_lo = mmio_read32(virtio_base + 0x100);
    uint32_t cap_hi = mmio_read32(virtio_base + 0x104);
    uint64_t cap = ((uint64_t)cap_hi << 32) | cap_lo;
    fw_printf("fw: virtio capacity=%lu sectors\n", cap);
  }
  {
    uint32_t i;
    for (i = 0; i < 8; i++) {
      uint64_t base = VIRTIO0_BASE + (uint64_t)i * 0x1000U;
      fw_printf("fw: virtio[%u] base=0x%lx id=%u\n", i, base, mmio_read32(base + 0x008));
    }
  }

  if (disk_init(virtio_base) != 0) {
    panic("virtio-blk init failed");
  }

  if (disk_read_bytes(IMAGE_HEADER_OFFSET, &hdr, sizeof(hdr)) != 0) {
    panic("read image header failed");
  }

  if (hdr.magic != IMAGE_MAGIC || hdr.version != IMAGE_VERSION) {
    panic("bad image header");
  }

  fw_printf("fw: kernel offset=0x%lx size=0x%lx\n", hdr.kernel_offset, hdr.kernel_size);

  load_elf(hdr.kernel_offset, hdr.kernel_size, mem_base, mem_size, &entry, &kernel_end);

  if (hdr.initrd_size) {
    uint64_t aligned_size = (hdr.initrd_size + 0xfffULL) & ~0xfffULL;
    if (mem_size == 0) {
      panic("initrd requires mem size");
    }
    initrd_paddr = (mem_base + mem_size - aligned_size) & ~0xfffULL;
    if (initrd_paddr <= kernel_end) {
      panic("initrd overlaps kernel");
    }
    if (disk_read_bytes(hdr.initrd_offset, (void *)(uintptr_t)initrd_paddr, (size_t)hdr.initrd_size) != 0) {
      panic("initrd load failed");
    }
  }

  if (have_dtb) {
    if (fdt_get_timebase((const void *)dtb_ptr, &timebase_hz) != 0) {
      timebase_hz = DEFAULT_TIMEBASE_HZ;
    }
    memcpy(g_dtb_copy, (const void *)dtb_ptr, dtb_size);
  } else {
    timebase_hz = DEFAULT_TIMEBASE_HZ;
  }

  g_bootinfo.magic = BOOTINFO_MAGIC;
  g_bootinfo.version = 3;
  g_bootinfo.mem_base = mem_base;
  g_bootinfo.mem_size = mem_size;
  g_bootinfo.dtb_paddr = have_dtb ? (uint64_t)(uintptr_t)g_dtb_copy : 0;
  g_bootinfo.initrd_paddr = initrd_paddr;
  g_bootinfo.initrd_size = hdr.initrd_size;
  g_bootinfo.cmdline_paddr = 0;
  g_bootinfo.cmdline_len = 0;
  g_bootinfo.log_level = 2;
  g_bootinfo.tick_hz = DEFAULT_TICK_HZ;
  g_bootinfo.hart_count = 1;
  g_bootinfo.root_disk_index = 1;
  g_bootinfo.reserved = 0;
  g_bootinfo.timebase_hz = timebase_hz;

  if (!have_dtb) {
    fw_printf("fw: dtb missing, using virt defaults\n");
  }

  {
    uint64_t medeleg = 0;
    medeleg |= 0xFF; // exceptions 0-7
    medeleg |= (1UL << 8) | (1UL << 12) | (1UL << 13) | (1UL << 15);
    __asm__ volatile("csrw medeleg, %0" :: "r"(medeleg));
  }
  {
    uint64_t mideleg = (1UL << 1) | (1UL << 5) | (1UL << 9);
    __asm__ volatile("csrw mideleg, %0" :: "r"(mideleg));
  }
  {
    uint64_t mcounteren = (1UL << 1);
    __asm__ volatile("csrw mcounteren, %0" :: "r"(mcounteren));
  }
  clint_write_mtimecmp(hartid, ~0ULL);
  {
    uint64_t mie = (1UL << 3) | (1UL << 7) | (1UL << 11);
    __asm__ volatile("csrw mie, %0" :: "r"(mie));
  }
  {
    uint64_t pmpaddr = ~0ULL >> 2;
    __asm__ volatile("csrw pmpaddr0, %0" :: "r"(pmpaddr));
    __asm__ volatile("li t0, 0x0f; csrw pmpcfg0, t0" ::: "t0");
  }

  fw_printf("fw: entry=0x%lx bootinfo=%p\n", entry, (void *)&g_bootinfo);

  enter_kernel(hartid, entry, g_bootinfo.dtb_paddr, &g_bootinfo);

  panic("returned from kernel");
}
