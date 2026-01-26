#include <stdint.h>

#include "sbi.h"

#define SBI_EXT_TIME 0x54494D45UL
#define SBI_FID_SET_TIMER 0UL

struct sbi_ret {
  long error;
  long value;
};

static inline struct sbi_ret sbi_call(uint64_t eid,
                                      uint64_t fid,
                                      uint64_t arg0,
                                      uint64_t arg1,
                                      uint64_t arg2,
                                      uint64_t arg3,
                                      uint64_t arg4,
                                      uint64_t arg5) {
  register uint64_t a0 asm("a0") = arg0;
  register uint64_t a1 asm("a1") = arg1;
  register uint64_t a2 asm("a2") = arg2;
  register uint64_t a3 asm("a3") = arg3;
  register uint64_t a4 asm("a4") = arg4;
  register uint64_t a5 asm("a5") = arg5;
  register uint64_t a6 asm("a6") = fid;
  register uint64_t a7 asm("a7") = eid;

  __asm__ volatile("ecall"
                   : "+r"(a0), "+r"(a1)
                   : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                   : "memory", "t0", "t1");

  return (struct sbi_ret){(long)a0, (long)a1};
}

void sbi_set_timer(uint64_t stime_value) {
  (void)sbi_call(SBI_EXT_TIME, SBI_FID_SET_TIMER, stime_value, 0, 0, 0, 0, 0);
}
