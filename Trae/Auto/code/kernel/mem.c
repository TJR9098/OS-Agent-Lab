#include "mem.h"
#include "printf.h"
#include "panic.h"

// 外部符号声明：内核结束地址
extern char __kernel_end[];

// 全局空闲链表头
struct run *g_freelist = NULL;

// 可分配区间的全局变量
static uint64_t alloc_start = 0;
static uint64_t alloc_end = 0;

// 辅助函数：向上对齐到页大小
static uint64_t align_up(uint64_t addr, uint64_t size) {
    return (addr + size - 1) & ~(size - 1);
}

// 手动实现内存清零
static void mem_zero(void *dst, uint64_t size) {
    char *d = (char *)dst;
    for (uint64_t i = 0; i < size; i++) {
        d[i] = 0;
    }
}

// 手动实现内存填充
static void mem_fill(void *dst, uint8_t value, uint64_t size) {
    char *d = (char *)dst;
    for (uint64_t i = 0; i < size; i++) {
        d[i] = value;
    }
}

// 内存初始化函数
void kinit(uint64_t mem_base, uint64_t mem_size) {
    // 验证参数
    if (mem_base != DRAM_BASE || mem_size != DRAM_SIZE) {
        panic("kinit args");
    }
    
    // 打印内存信息
    kprintf("mem: base=0x%lx size=0x%lx\n", mem_base, mem_size);
    kprintf("mem: __kernel_end=0x%lx\n", (uint64_t)__kernel_end);
    
    // 初始化空闲链表
    g_freelist = NULL;
    
    // 计算可分配区间
    alloc_start = align_up((uint64_t)__kernel_end, PAGE_SIZE);
    alloc_end = DRAM_BASE + DRAM_SIZE;
    
    // 打印可分配区间
    kprintf("mem: alloc_start=0x%lx alloc_end=0x%lx\n", alloc_start, alloc_end);
    
    // 合法性检查
    if (alloc_start >= alloc_end) {
        panic("bad memory range");
    }
    
    // 计算可分配页数
    uint64_t free_pages = (alloc_end - alloc_start) / PAGE_SIZE;
    kprintf("mem: free_pages=%ld\n", free_pages);
    
    // 对可分配区间按页遍历并调用kfree()
    // 注意：这里按从alloc_end到alloc_start的顺序遍历，确保连续分配时地址递增
    // kprintf("mem: adding pages to freelist...\n");
    uint64_t count = 0;
    for (uint64_t pa = alloc_end - PAGE_SIZE; pa >= alloc_start; pa -= PAGE_SIZE) {
        kfree((void *)pa);
        count++;
        if (count % 100 == 0) {
            // kprintf("mem: added %ld pages\n", count);
        }
    }
    
    // 打印可分配页数
    kprintf("mem: kinit done, total free_pages=%ld\n", count);
    kprintf("mem: final freelist=0x%lx\n", (uint64_t)g_freelist);
}

// 物理页分配函数
void *kalloc(void) {
    struct run *r = g_freelist;
    
    // kprintf("mem: kalloc - freelist=0x%lx\n", (uint64_t)g_freelist);
    
    if (r) {
        g_freelist = r->next;
        // kprintf("mem: kalloc returns 0x%lx, next freelist=0x%lx\n", (uint64_t)r, (uint64_t)g_freelist);
        // 注意：在启用虚拟内存后，不能直接访问物理地址，所以移除mem_zero调用
        // 分配出的页将由使用它的代码负责清零
    } else {
        kprintf("mem: kalloc returns NULL - no free pages\n");
    }
    
    return r;
}

// 物理页释放函数
void kfree(void *pa) {
    uint64_t p = (uint64_t)pa;
    
    // kprintf("mem: kfree(0x%lx)\n", p);
    
    // 约束检查
    if (p % PAGE_SIZE != 0) {
        panic("kfree: pa not aligned");
    }
    
    if (p < alloc_start || p >= alloc_end) {
        panic("kfree: pa out of range");
    }
    
    // 插入到空闲链表头部
    struct run *r = (struct run *)pa;
    r->next = g_freelist;
    g_freelist = r;
    // kprintf("mem: kfree done, freelist=0x%lx\n", (uint64_t)g_freelist);
}
