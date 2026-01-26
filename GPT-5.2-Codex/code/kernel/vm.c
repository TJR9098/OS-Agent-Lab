#include <stdint.h>

#include "errno.h"
#include "memlayout.h"
#include "mm.h"
#include "riscv.h"
#include "string.h"
#include "vm.h"

#define PTE2PA(pte) (((pte) >> 10) << 12)
#define PA2PTE(pa) (((pa) >> 12) << 10)
#define PGROUNDDOWN(a) ((a) & ~(PAGE_SIZE - 1))
#define PGROUNDUP(a) (((a) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

static inline uint64_t satp_ppn(uint64_t satp) {
  return satp & ((1UL << 44) - 1);
}

static inline uint64_t va_sign_extend(uint64_t va) {
  if (va & (1UL << 38)) {
    va |= (~0UL << 39);
  }
  return va;
}

static pagetable_t kernel_pagetable(void) {
  uint64_t satp = read_csr(satp);
  return (pagetable_t)(uintptr_t)(satp_ppn(satp) << 12);
}

static pte_t *walk(pagetable_t pt, uint64_t va, int alloc) {
  for (int level = 2; level > 0; level--) {
    uint64_t idx = (level == 2) ? VPN2(va) : VPN1(va);
    pte_t *pte = &pt[idx];
    if (*pte & PTE_V) {
      pt = (pagetable_t)(uintptr_t)PTE2PA(*pte);
    } else {
      if (!alloc) {
        return NULL;
      }
      void *page = kalloc();
      if (!page) {
        return NULL;
      }
      memset(page, 0, PAGE_SIZE);
      *pte = PA2PTE((uint64_t)(uintptr_t)page) | PTE_V;
      pt = (pagetable_t)page;
    }
  }
  return &pt[VPN0(va)];
}

pagetable_t uvm_create(void) {
  pagetable_t pt = (pagetable_t)kalloc();
  if (!pt) {
    return NULL;
  }
  memset(pt, 0, PAGE_SIZE);

  pagetable_t kpt = kernel_pagetable();
  memcpy(pt, kpt, PAGE_SIZE);

  // Split the low 1GiB mapping so user pages can be mapped in that region.
  pagetable_t low_l1 = (pagetable_t)kalloc();
  if (!low_l1) {
    kfree(pt);
    return NULL;
  }
  memset(low_l1, 0, PAGE_SIZE);
  uint64_t mmio_flags = PTE_V | PTE_R | PTE_W | PTE_G | PTE_A | PTE_D;
  low_l1[VPN1(UART0_BASE)] = pte_make(UART0_BASE, mmio_flags);
  low_l1[VPN1(CLINT_BASE)] = pte_make(CLINT_BASE, mmio_flags);
  low_l1[VPN1(PLIC_BASE)] = pte_make(PLIC_BASE, mmio_flags);
  pt[VPN2(0x0)] = PA2PTE((uint64_t)(uintptr_t)low_l1) | PTE_V;
  return pt;
}

static void freewalk(pagetable_t pt, int level, uint64_t va_base) {
  for (int i = 0; i < 512; i++) {
    pte_t pte = pt[i];
    if ((pte & PTE_V) == 0) {
      continue;
    }
    uint64_t va = va_base + ((uint64_t)i << (12 + 9 * level));
    uint64_t sva = va_sign_extend(va);
    if (level == 0 || (pte & (PTE_R | PTE_W | PTE_X))) {
      if ((pte & PTE_U) && sva < KERNEL_VIRT_BASE) {
        void *pa = (void *)(uintptr_t)PTE2PA(pte);
        kfree(pa);
      }
      pt[i] = 0;
      continue;
    }
    if (sva >= KERNEL_VIRT_BASE) {
      continue;
    }
    pagetable_t child = (pagetable_t)(uintptr_t)PTE2PA(pte);
    freewalk(child, level - 1, va);
    kfree(child);
    pt[i] = 0;
  }
}

void uvm_free(pagetable_t pt) {
  if (!pt) {
    return;
  }
  freewalk(pt, 2, 0);
  kfree(pt);
}

int uvm_map(pagetable_t pt, uint64_t va, uint64_t pa, uint64_t sz, uint64_t perm) {
  uint64_t a = va & ~(PAGE_SIZE - 1);
  uint64_t last = (va + sz - 1) & ~(PAGE_SIZE - 1);
  for (;;) {
    pte_t *pte = walk(pt, a, 1);
    if (!pte) {
      return -ENOMEM;
    }
    if (*pte & PTE_V) {
      return -EEXIST;
    }
    *pte = PA2PTE(pa) | perm | PTE_V;
    if (a == last) {
      break;
    }
    a += PAGE_SIZE;
    pa += PAGE_SIZE;
  }
  return 0;
}

int uvm_unmap(pagetable_t pt, uint64_t va, uint64_t npages) {
  uint64_t a = va & ~(PAGE_SIZE - 1);
  for (uint64_t i = 0; i < npages; i++, a += PAGE_SIZE) {
    pte_t *pte = walk(pt, a, 0);
    if (!pte || (*pte & PTE_V) == 0) {
      continue;
    }
    if (*pte & PTE_U) {
      void *pa = (void *)(uintptr_t)PTE2PA(*pte);
      kfree(pa);
    }
    *pte = 0;
  }
  return 0;
}

int uvm_alloc(pagetable_t pt, uint64_t oldsz, uint64_t newsz, uint64_t perm) {
  if (newsz < oldsz) {
    return 0;
  }
  uint64_t a = PGROUNDUP(oldsz);
  uint64_t last = PGROUNDUP(newsz);
  for (; a < last; a += PAGE_SIZE) {
    void *mem = kalloc();
    if (!mem) {
      return -ENOMEM;
    }
    if (uvm_map(pt, a, (uint64_t)(uintptr_t)mem, PAGE_SIZE, perm) != 0) {
      kfree(mem);
      return -ENOMEM;
    }
  }
  return 0;
}

int uvm_dealloc(pagetable_t pt, uint64_t oldsz, uint64_t newsz) {
  if (newsz >= oldsz) {
    return 0;
  }
  uint64_t a = PGROUNDUP(newsz);
  uint64_t end = PGROUNDUP(oldsz);
  if (a < end) {
    uvm_unmap(pt, a, (end - a) / PAGE_SIZE);
  }
  return 0;
}

int uvm_copy(pagetable_t src, pagetable_t dst, uint64_t sz) {
  for (uint64_t va = 0; va < sz; va += PAGE_SIZE) {
    pte_t *pte = walk(src, va, 0);
    if (!pte || (*pte & PTE_V) == 0) {
      continue;
    }
    uint64_t pa = PTE2PA(*pte);
    uint64_t flags = *pte & (PTE_R | PTE_W | PTE_X | PTE_U);
    void *mem = kalloc();
    if (!mem) {
      return -ENOMEM;
    }
    memcpy(mem, (void *)(uintptr_t)pa, PAGE_SIZE);
    if (uvm_map(dst, va, (uint64_t)(uintptr_t)mem, PAGE_SIZE, flags) != 0) {
      kfree(mem);
      return -ENOMEM;
    }
  }
  return 0;
}

int uvm_copy_range(pagetable_t src, pagetable_t dst, uint64_t start, uint64_t end) {
  if (end < start) {
    return -EINVAL;
  }
  uint64_t va_start = start & ~(PAGE_SIZE - 1);
  for (uint64_t va = va_start; va < end; va += PAGE_SIZE) {
    pte_t *pte = walk(src, va, 0);
    if (!pte || (*pte & PTE_V) == 0) {
      continue;
    }
    uint64_t pa = PTE2PA(*pte);
    uint64_t flags = *pte & (PTE_R | PTE_W | PTE_X | PTE_U);
    void *mem = kalloc();
    if (!mem) {
      return -ENOMEM;
    }
    memcpy(mem, (void *)(uintptr_t)pa, PAGE_SIZE);
    if (uvm_map(dst, va, (uint64_t)(uintptr_t)mem, PAGE_SIZE, flags) != 0) {
      kfree(mem);
      return -ENOMEM;
    }
  }
  return 0;
}

uint64_t walkaddr(pagetable_t pt, uint64_t va) {
  pte_t *pte = walk(pt, va, 0);
  if (!pte || (*pte & PTE_V) == 0) {
    return 0;
  }
  if ((*pte & PTE_U) == 0) {
    return 0;
  }
  return PTE2PA(*pte) + (va & (PAGE_SIZE - 1));
}

int copyin(pagetable_t pt, void *dst, uint64_t srcva, size_t len) {
  uint8_t *d = (uint8_t *)dst;
  while (len > 0) {
    uint64_t pa = walkaddr(pt, srcva);
    if (pa == 0) {
      return -EFAULT;
    }
    uint64_t n = PAGE_SIZE - (srcva & (PAGE_SIZE - 1));
    if (n > len) {
      n = len;
    }
    memcpy(d, (void *)(uintptr_t)pa, n);
    len -= n;
    d += n;
    srcva += n;
  }
  return 0;
}

int copyout(pagetable_t pt, uint64_t dstva, const void *src, size_t len) {
  const uint8_t *s = (const uint8_t *)src;
  while (len > 0) {
    uint64_t pa = walkaddr(pt, dstva);
    if (pa == 0) {
      return -EFAULT;
    }
    uint64_t n = PAGE_SIZE - (dstva & (PAGE_SIZE - 1));
    if (n > len) {
      n = len;
    }
    memcpy((void *)(uintptr_t)pa, s, n);
    len -= n;
    s += n;
    dstva += n;
  }
  return 0;
}

int copyinstr(pagetable_t pt, char *dst, uint64_t srcva, size_t max) {
  size_t n = 0;
  while (n < max) {
    uint64_t pa = walkaddr(pt, srcva);
    if (pa == 0) {
      return -EFAULT;
    }
    char c = *(char *)(uintptr_t)pa;
    dst[n++] = c;
    srcva++;
    if (c == '\0') {
      return 0;
    }
  }
  return -ENAMETOOLONG;
}
