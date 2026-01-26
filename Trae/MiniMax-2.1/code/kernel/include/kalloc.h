#ifndef _KALLOC_H
#define _KALLOC_H

#include <stdint.h>

#define PAGE_SIZE 4096

#define DRAM_BASE 0x80000000UL
#define DRAM_SIZE (4096UL * 1024UL * 1024UL)

#define KERNEL_PHYS_BASE 0x80200000UL
#define KERNEL_VIRT_OFFSET 0UL

struct run {
    struct run *next;
};

extern char __kernel_end[];

void kinit(uint64_t mem_base, uint64_t mem_size);
void *kalloc(void);
void kfree(void *pa);
uint64_t get_free_pages(void);

#endif
