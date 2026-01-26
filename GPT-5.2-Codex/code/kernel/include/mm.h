#pragma once

#include <stdint.h>

void kinit(uint64_t mem_base, uint64_t mem_size);
void *kalloc(void);
void kfree(void *pa);
