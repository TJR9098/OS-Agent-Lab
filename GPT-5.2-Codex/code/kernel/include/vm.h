#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint64_t pte_t;
typedef uint64_t *pagetable_t;

pagetable_t uvm_create(void);
void uvm_free(pagetable_t pt);
int uvm_map(pagetable_t pt, uint64_t va, uint64_t pa, uint64_t sz, uint64_t perm);
int uvm_unmap(pagetable_t pt, uint64_t va, uint64_t npages);
int uvm_alloc(pagetable_t pt, uint64_t oldsz, uint64_t newsz, uint64_t perm);
int uvm_dealloc(pagetable_t pt, uint64_t oldsz, uint64_t newsz);
int uvm_copy(pagetable_t src, pagetable_t dst, uint64_t sz);
int uvm_copy_range(pagetable_t src, pagetable_t dst, uint64_t start, uint64_t end);
uint64_t walkaddr(pagetable_t pt, uint64_t va);

int copyin(pagetable_t pt, void *dst, uint64_t srcva, size_t len);
int copyout(pagetable_t pt, uint64_t dstva, const void *src, size_t len);
int copyinstr(pagetable_t pt, char *dst, uint64_t srcva, size_t max);
