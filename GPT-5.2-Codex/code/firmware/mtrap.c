#include <stdint.h>

#include "memlayout.h"
#include "printf.h"

#define SBI_EXT_TIME 0x54494D45UL
#define SBI_EXT_SRST 0x53525354UL
#define SBI_ERR_NOT_SUPPORTED -2L

#define SIFIVE_TEST_BASE 0x100000UL
#define SIFIVE_TEST_SHUTDOWN 0x5555U
#define SIFIVE_TEST_REBOOT 0x7777U

struct mtrap_frame {
  uint64_t ra;
  uint64_t gp;
  uint64_t tp;
  uint64_t t0;
  uint64_t t1;
  uint64_t t2;
  uint64_t t3;
  uint64_t t4;
  uint64_t t5;
  uint64_t t6;
  uint64_t s0;
  uint64_t s1;
  uint64_t s2;
  uint64_t s3;
  uint64_t s4;
  uint64_t s5;
  uint64_t s6;
  uint64_t s7;
  uint64_t s8;
  uint64_t s9;
  uint64_t s10;
  uint64_t s11;
  uint64_t a0;
  uint64_t a1;
  uint64_t a2;
  uint64_t a3;
  uint64_t a4;
  uint64_t a5;
  uint64_t a6;
  uint64_t a7;
  uint64_t mepc;
  uint64_t mstatus;
  uint64_t mcause;
  uint64_t mtval;
};

static inline uint64_t read_mhartid(void) {
  uint64_t val;
  __asm__ volatile("csrr %0, mhartid" : "=r"(val));
  return val;
}

static inline void clint_write_mtimecmp(uint64_t hartid, uint64_t val) {
  *(volatile uint64_t *)(CLINT_BASE + 0x4000UL + hartid * 8UL) = val;
}

static inline void set_stip(void) {
  __asm__ volatile("csrs mip, %0" :: "r"(1UL << 5));
}

static inline void clear_stip(void) {
  __asm__ volatile("csrc mip, %0" :: "r"(1UL << 5));
}

static void sbi_set_timer(uint64_t stime) {
  uint64_t hartid = read_mhartid();
  clint_write_mtimecmp(hartid, stime);
  clear_stip();
}

static void sbi_system_reset(uint32_t reset_type) {
  uint32_t code = (reset_type == 0U) ? SIFIVE_TEST_SHUTDOWN : SIFIVE_TEST_REBOOT;
  *(volatile uint32_t *)(uintptr_t)SIFIVE_TEST_BASE = code;
}

void mtrap_handler(struct mtrap_frame *tf) {
  uint64_t mcause = tf->mcause;

  if ((mcause >> 63) != 0) {
    uint64_t code = mcause & 0xfffU;
    if (code == 7) {
      uint64_t hartid = read_mhartid();
      clint_write_mtimecmp(hartid, ~0ULL);
      set_stip();
      return;
    }
  } else if (mcause == 9) {
    uint64_t eid = tf->a7;
    uint64_t fid = tf->a6;

#ifdef CONFIG_STAGE3_DEBUG_BOOT
    fw_printf("fw: sbi ecall eid=0x%lx fid=0x%lx\n", eid, fid);
#endif

    if (eid == SBI_EXT_TIME && fid == 0) {
      sbi_set_timer(tf->a0);
      tf->a0 = 0;
      tf->a1 = 0;
    } else if (eid == SBI_EXT_SRST) {
      sbi_system_reset((uint32_t)tf->a0);
      tf->a0 = 0;
      tf->a1 = 0;
    } else if (eid == 0) {
      sbi_set_timer(tf->a0);
      tf->a0 = 0;
    } else {
      tf->a0 = (uint64_t)SBI_ERR_NOT_SUPPORTED;
      tf->a1 = 0;
    }

    tf->mepc += 4;
    return;
  }

  fw_printf("fw: trap mcause=0x%lx mepc=0x%lx mtval=0x%lx\n",
            mcause,
            tf->mepc,
            tf->mtval);
  for (;;) {
    __asm__ volatile("wfi");
  }
}
