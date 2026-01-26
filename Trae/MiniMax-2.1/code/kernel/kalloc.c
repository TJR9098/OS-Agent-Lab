#include "kalloc.h"
#include "printf.h"
#include "panic.h"
#include <stddef.h>

static struct run *g_freelist = NULL;
static uint64_t g_free_pages = 0;

static uint64_t alloc_start = 0;
static uint64_t alloc_end = 0;

static uint64_t align_up(uint64_t x, uint64_t align) {
    return (x + align - 1) & ~(align - 1);
}

static void memset_zero(void *ptr, uint64_t size) {
    uint64_t *p = (uint64_t *)ptr;
    for (uint64_t i = 0; i < size / 8; i++) {
        p[i] = 0;
    }
}

void kinit(uint64_t mem_base, uint64_t mem_size) {
    if (mem_base != DRAM_BASE || mem_size != DRAM_SIZE) {
        panic("kinit args");
    }

    g_freelist = NULL;
    g_free_pages = 0;

    uint64_t kernel_end_aligned = align_up((uint64_t)__kernel_end, PAGE_SIZE);
    alloc_start = kernel_end_aligned;
    alloc_end = DRAM_BASE + DRAM_SIZE;

    if (alloc_start >= alloc_end) {
        panic("bad memory range");
    }

    for (uint64_t pa = alloc_end - PAGE_SIZE; pa >= alloc_start; pa -= PAGE_SIZE) {
        struct run *r = (struct run *)pa;
        r->next = g_freelist;
        g_freelist = r;
        g_free_pages++;
    }

    kprintf("mem: base=0x%lx size=%lu\n", DRAM_BASE, DRAM_SIZE);
    kprintf("mem: kinit done, free_pages=%lu\n", g_free_pages);
}

void *kalloc(void) {
    if (g_freelist == NULL) {
        return NULL;
    }

    struct run *r = g_freelist;
    g_freelist = r->next;
    g_free_pages--;

    memset_zero(r, PAGE_SIZE);
    return r;
}

void kfree(void *pa) {
    uintptr_t addr = (uintptr_t)pa;

    if (addr % PAGE_SIZE != 0) {
        panic("kfree: not aligned");
    }

    if (addr < alloc_start || addr >= alloc_end) {
        panic("kfree: out of range");
    }

    struct run *r = (struct run *)pa;
    r->next = g_freelist;
    g_freelist = r;
    g_free_pages++;
}

uint64_t get_free_pages(void) {
    return g_free_pages;
}
