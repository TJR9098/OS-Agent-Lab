#include <stdint.h>

#include "memlayout.h"
#include "mm.h"
#include "string.h"

struct run {
  struct run *next;
};

static struct run *g_freelist;

static uint64_t align_up(uint64_t val, uint64_t align) {
  return (val + align - 1) & ~(align - 1);
}

void kinit(uint64_t mem_base, uint64_t mem_size) {
  extern char __kernel_end[];
  uint64_t start = (uint64_t)(uintptr_t)__kernel_end - KERNEL_VIRT_OFFSET;
  uint64_t end = mem_base + mem_size;
  start = align_up(start, PAGE_SIZE);

  for (uint64_t p = start; p + PAGE_SIZE <= end; p += PAGE_SIZE) {
    kfree((void *)(uintptr_t)p);
  }
}

void kfree(void *pa) {
  uintptr_t p = (uintptr_t)pa;
  if ((p & (PAGE_SIZE - 1)) != 0) {
    return;
  }
  struct run *r = (struct run *)pa;
  r->next = g_freelist;
  g_freelist = r;
}

void *kalloc(void) {
  struct run *r = g_freelist;
  if (!r) {
    return NULL;
  }
  g_freelist = r->next;
  memset(r, 0, PAGE_SIZE);
  return r;
}
