#pragma once

#include <stdint.h>

#define SATP_MODE_SV39 8UL

#define SSTATUS_SIE (1UL << 1)
#define SSTATUS_SPIE (1UL << 5)
#define SSTATUS_SPP (1UL << 8)


#define PTE_V (1UL << 0)
#define PTE_R (1UL << 1)
#define PTE_W (1UL << 2)
#define PTE_X (1UL << 3)
#define PTE_U (1UL << 4)
#define PTE_G (1UL << 5)
#define PTE_A (1UL << 6)
#define PTE_D (1UL << 7)

#define VPN2(va) (((uint64_t)(va) >> 30) & 0x1FF)
#define VPN1(va) (((uint64_t)(va) >> 21) & 0x1FF)
#define VPN0(va) (((uint64_t)(va) >> 12) & 0x1FF)

static inline uint64_t pte_make(uint64_t pa, uint64_t flags) {
  return ((pa >> 12) << 10) | flags;
}

#define read_csr(reg) ({ uint64_t __tmp; __asm__ volatile("csrr %0, " #reg : "=r"(__tmp)); __tmp; })
#define write_csr(reg, val) __asm__ volatile("csrw " #reg ", %0" :: "rK"(val))
#define set_csr(reg, bit) __asm__ volatile("csrs " #reg ", %0" :: "rK"(bit))
#define clear_csr(reg, bit) __asm__ volatile("csrc " #reg ", %0" :: "rK"(bit))

static inline uint64_t read_tp(void) {
  uint64_t val;
  __asm__ volatile("mv %0, tp" : "=r"(val));
  return val;
}

static inline void write_tp(uint64_t val) {
  __asm__ volatile("mv tp, %0" :: "r"(val));
}

static inline void sfence_vma(void) {
  __asm__ volatile("sfence.vma" ::: "memory");
}
