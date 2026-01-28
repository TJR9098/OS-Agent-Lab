#include "vm.h"
#include "mem.h"
#include "memlayout.h"
#include "printf.h"
#include "panic.h"
#include "riscv.h"
#include "trap.h"

// 外部符号声明
extern char __kernel_end[];

// 手动实现内存清零
static void mem_zero(void *dst, uint64_t size) {
    char *d = (char *)dst;
    for (uint64_t i = 0; i < size; i++) {
        d[i] = 0;
    }
}

// 手动实现内存复制
static void *memcpy(void *dst, const void *src, size_t size) {
    char *d = (char *)dst;
    const char *s = (const char *)src;
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
    return dst;
}

// 创建页表
pagetable_t pagetable_create(void) {
    uint64_t *pagetable = (uint64_t *)kalloc();
    if (!pagetable) {
        panic("pagetable_create: kalloc failed");
    }
    return pagetable;
}

// 销毁页表
void pagetable_destroy(pagetable_t pagetable) {
    kfree(pagetable);
}

// 按Sv39三层索引遍历页表
uint64_t *walk(pagetable_t pagetable, uint64_t va, int alloc) {
    uint64_t vpn[3];
    vpn[0] = VA_VPN0(va);
    vpn[1] = VA_VPN1(va);
    vpn[2] = VA_VPN2(va);

    for (int level = 2; level >= 0; level--) {
        uint64_t *pte = &pagetable[vpn[level]];
        if (*pte & PTE_V) {
            // 检查是否为大页（设置了PTE_PS位，只在L1级有效）
            if (level == 1 && (*pte & PTE_PS)) {
                return pte;
            }
            uint64_t pa = PPN_PA(PTE_PPN(*pte));
            if (level == 0) {
                return pte;
            }
            pagetable = (uint64_t *)pa;
        } else {
            if (!alloc) {
                return NULL;
            }
            uint64_t *new_pagetable = (uint64_t *)kalloc();
            if (!new_pagetable) {
                return NULL;
            }
            // 注意：在启用虚拟内存后，不能直接访问物理地址，所以移除mem_zero调用
            // 页表页的清零将由使用它的代码负责
            *pte = PPN_PTE(PA_PPN((uint64_t)new_pagetable)) | PTE_V;
            if (level == 0) {
                return pte;
            }
            pagetable = new_pagetable;
        }
    }
    return NULL;
}

// 按页映射
void map_pages(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, uint64_t perm) {
    uint64_t end = va + size;
    va = va & ~PAGE_MASK;
    pa = pa & ~PAGE_MASK;

    while (va < end) {
        uint64_t *pte = walk(pagetable, va, 1);
        if (!pte) {
            panic("map_pages: walk failed");
        }
        *pte = PPN_PTE(PA_PPN(pa)) | perm | PTE_V;
        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
}

// 使用2MB大页映射内存区域
// 在Sv39架构中，2MB大页直接设置在L1页表项上
static void map_pages_huge(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, uint64_t perm) {
    uint64_t end = va + size;
    va = va & ~HUGE_PAGE_MASK;
    pa = pa & ~HUGE_PAGE_MASK;

    while (va < end) {
        // 计算VPN2索引（顶级页表）
        uint64_t vpn2 = (va >> 30) & 0x1FF;
        // 计算VPN1索引（L1页表中的索引）
        uint64_t vpn1 = (va >> HUGE_PAGE_SHIFT) & 0x1FF;
        
        // 获取L2页表项
        uint64_t *l2_pte = &pagetable[vpn2];
        
        // 检查是否需要分配L1页表
        if (!(*l2_pte & PTE_V)) {
            // 分配L1页表
            uint64_t *l1_pagetable = (uint64_t *)kalloc();
            if (!l1_pagetable) {
                panic("map_pages_huge: kalloc failed");
            }
            mem_zero(l1_pagetable, PAGE_SIZE);
            *l2_pte = PPN_PTE(PA_PPN((uint64_t)l1_pagetable)) | PTE_V;
        }
        
        // 获取L1页表基地址
        uint64_t *l1_pagetable = (uint64_t *)PPN_PA(PTE_PPN(*l2_pte));
        
        // 获取L1页表项（用于2MB大页）
        uint64_t *l1_pte = &l1_pagetable[vpn1];
        
        // 设置2MB大页
        uint64_t ppn = (pa >> PAGE_SHIFT) & 0x7FFFFFF;  // 44位PPN中的低27位（对应2MB页）
        *l1_pte = (ppn << 10) | perm | PTE_V | PTE_PS;
        
        va += HUGE_PAGE_SIZE;
        pa += HUGE_PAGE_SIZE;
    }
}

// 解除映射
void unmap_pages(pagetable_t pagetable, uint64_t va, uint64_t npages, int do_free) {
    for (uint64_t i = 0; i < npages; i++) {
        uint64_t *pte = walk(pagetable, va + i * PAGE_SIZE, 0);
        if (!pte || !(*pte & PTE_V)) {
            panic("unmap_pages: not mapped");
        }
        if (do_free) {
            uint64_t pa = PPN_PA(PTE_PPN(*pte));
            kfree((void *)pa);
        }
        *pte = 0;
    }
}

// 检查地址是否可访问
static int check_access(pagetable_t pagetable, uint64_t va, uint64_t perm) {
    uint64_t *pte = walk(pagetable, va, 0);
    if (!pte || !(*pte & PTE_V)) {
        return 0;
    }
    if (((*pte & perm) & (PTE_R | PTE_W | PTE_X)) != (perm & (PTE_R | PTE_W | PTE_X))) {
        return 0;
    }
    return 1;
}

// 安全开启satp前的自检
void satp_precheck(pagetable_t pagetable) {
    // 检查PC所在页
    uint64_t pc;
    asm volatile ("auipc %0, 0" : "=r"(pc));
    if (!check_access(pagetable, pc, PTE_X)) {
        panic("satp_precheck: PC page not accessible");
    }

    // 检查当前栈页
    uint64_t sp;
    asm volatile ("mv %0, sp" : "=r"(sp));
    if (!check_access(pagetable, sp, PTE_R | PTE_W)) {
        panic("satp_precheck: stack page not accessible");
    }

    // 检查trap向量页
    uint64_t stvec;
    asm volatile ("csrr %0, stvec" : "=r"(stvec));
    if (!check_access(pagetable, stvec, PTE_X)) {
        panic("satp_precheck: trap vector page not accessible");
    }
}

// 读取satp寄存器
uint64_t read_satp(void) {
    uint64_t satp;
    asm volatile ("csrr %0, satp" : "=r"(satp));
    return satp;
}

// 切换地址空间
void switch_satp(pagetable_t pagetable) {
    // 计算PPN
    uint64_t ppn = PA_PPN((uint64_t)pagetable);
    // 构造satp值
    uint64_t satp = MAKE_SATP(SATP_MODE_SV39, ppn);
    // 写satp
    kprintf("switch_satp: writing satp = 0x%lx\n", satp);
    asm volatile ("csrw satp, %0" : : "r"(satp));
    // 刷新TLB
    kprintf("switch_satp: flushing TLB\n");
    asm volatile ("sfence.vma");
}

// 复制用户空间数据到内核空间
int copyin(pagetable_t pagetable, void *dst, uint64_t srcva, uint64_t len) {
    char *d = (char *)dst;
    uint64_t n = len;

    while (n > 0) {
        uint64_t va = srcva & ~PAGE_MASK;
        uint64_t offset = srcva - va;
        uint64_t bytes = PAGE_SIZE - offset;
        if (bytes > n) {
            bytes = n;
        }

        uint64_t *pte = walk(pagetable, srcva, 0);
        if (!pte || !(*pte & PTE_V) || !(*pte & PTE_R) || !(*pte & PTE_U)) {
            return -1;
        }

        uint64_t pa;
        if (*pte & PTE_PS) {
            // 大页：PPN占用更多的位
            pa = (PTE_PPN(*pte) << PAGE_SHIFT) + offset;
        } else {
            pa = PPN_PA(PTE_PPN(*pte)) + offset;
        }
        char *s = (char *)pa;
        for (uint64_t i = 0; i < bytes; i++) {
            d[i] = s[i];
        }

        n -= bytes;
        d += bytes;
        srcva += bytes;
    }

    return 0;
}

// 复制内核空间数据到用户空间
int copyout(pagetable_t pagetable, uint64_t dstva, const void *src, uint64_t len) {
    const char *s = (const char *)src;
    uint64_t n = len;

    while (n > 0) {
        uint64_t va = dstva & ~PAGE_MASK;
        uint64_t offset = dstva - va;
        uint64_t bytes = PAGE_SIZE - offset;
        if (bytes > n) {
            bytes = n;
        }

        uint64_t *pte = walk(pagetable, dstva, 0);
        if (!pte || !(*pte & PTE_V) || !(*pte & PTE_W) || !(*pte & PTE_U)) {
            return -1;
        }

        uint64_t pa;
        if (*pte & PTE_PS) {
            // 大页：PPN占用更多的位
            pa = (PTE_PPN(*pte) << PAGE_SHIFT) + offset;
        } else {
            pa = PPN_PA(PTE_PPN(*pte)) + offset;
        }
        char *d = (char *)pa;
        for (uint64_t i = 0; i < bytes; i++) {
            d[i] = s[i];
        }

        n -= bytes;
        s += bytes;
        dstva += bytes;
    }

    return 0;
}

// 初始化内核页表
pagetable_t kernel_pagetable_create(void) {
    pagetable_t pagetable = pagetable_create();

    // 使用2MB大页映射DRAM区域，大大减少页表页数量
    // 128MB / 2MB = 64个大页
    // 而使用4KB页需要32768个页表页
    map_pages_huge(pagetable, DRAM_BASE, DRAM_BASE, DRAM_SIZE, KERNEL_PERM);

    // 映射设备（使用普通4KB页）
    map_pages(pagetable, UART_BASE, UART_BASE, UART_SIZE, DEV_PERM);
    map_pages(pagetable, CLINT_BASE, CLINT_BASE, CLINT_SIZE, DEV_PERM);

    return pagetable;
}
