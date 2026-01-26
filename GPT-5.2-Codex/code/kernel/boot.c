#include <stdint.h>

#include "memlayout.h"
#include "riscv.h"

__attribute__((section(".boot"), aligned(4096)))
static uint64_t boot_pgtbl_l0[512];

__attribute__((section(".boot"), aligned(4096)))
static uint64_t boot_pgtbl_l1_kernel[512];

__attribute__((section(".text.boot")))
uint64_t boot_pagetable_init(void) {
  uint64_t i;
  uint64_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_G | PTE_A | PTE_D;

  for (i = 0; i < 512; i++) {
    boot_pgtbl_l0[i] = 0;
    boot_pgtbl_l1_kernel[i] = 0;
  }

  boot_pgtbl_l0[VPN2(0x0)] = pte_make(0x0, flags);
  boot_pgtbl_l0[VPN2(DRAM_BASE)] = pte_make(DRAM_BASE, flags);
  boot_pgtbl_l0[VPN2(PAGE_OFFSET)] = pte_make(DRAM_BASE, flags);

  boot_pgtbl_l0[VPN2(KERNEL_VIRT_BASE)] = pte_make((uint64_t)boot_pgtbl_l1_kernel, PTE_V);

  for (i = 0; i < BOOT_KMAP_SIZE; i += SZ_2M) {
    uint64_t va = KERNEL_VIRT_BASE + i;
    uint64_t pa = KERNEL_PHYS_BASE + i;
    boot_pgtbl_l1_kernel[VPN1(va)] = pte_make(pa, flags);
  }

  return (uint64_t)boot_pgtbl_l0;
}
