#ifndef MEM_H
#define MEM_H

#include <stdint.h>

// 页大小固定为4096字节
#define PAGE_SIZE 4096

// 内存基地址（QEMU virt DRAM起始地址）
#define DRAM_BASE 0x80000000

// 内存大小（128MB，假设用户使用 -m 128）
#define DRAM_SIZE 0x8000000

// 内核物理基地址（固定）
#define KERNEL_PHYS_BASE 0x80200000

// 无虚拟内存
#define KERNEL_VIRT_OFFSET 0

// 空闲页链表结构
struct run {
    struct run *next;
};

// 全局空闲链表头
extern struct run *g_freelist;

// 内存管理函数声明
void kinit(uint64_t mem_base, uint64_t mem_size);
void *kalloc(void);
void kfree(void *pa);

#endif // MEM_H
