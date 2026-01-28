#ifndef VM_H
#define VM_H

#include <stdint.h>

// 页大小
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_MASK (PAGE_SIZE - 1)

// 大页大小
#define HUGE_PAGE_SIZE 0x200000  // 2MB
#define HUGE_PAGE_SHIFT 21
#define HUGE_PAGE_MASK (HUGE_PAGE_SIZE - 1)

// Sv39 页表相关定义
#define SV39_LEVELS 3
#define SV39_PTE_BITS 8
#define SV39_PTES_PER_PAGE (PAGE_SIZE / SV39_PTE_BITS)
#define SV39_VPN_BITS 9
#define SV39_PPN_BITS 44

// 虚拟地址分解
#define VA_VPN0(va) (((va) >> 12) & 0x1FF)
#define VA_VPN1(va) (((va) >> 21) & 0x1FF)
#define VA_VPN2(va) (((va) >> 30) & 0x1FF)
#define VA_SIGN_EXT(va) ((va) | (((va) & 0x8000000000) ? 0xFFFFFFFFFFFFFF00000000 : 0))

// 物理地址分解
#define PA_PPN(pa) ((pa) >> PAGE_SHIFT)
#define PPN_PA(ppn) ((ppn) << PAGE_SHIFT)

// PTE 权限位
#define PTE_V (1ULL << 0)  // 有效位
#define PTE_R (1ULL << 1)  // 读权限
#define PTE_W (1ULL << 2)  // 写权限
#define PTE_X (1ULL << 3)  // 执行权限
#define PTE_U (1ULL << 4)  // 用户权限
#define PTE_G (1ULL << 5)  // 全局页
#define PTE_A (1ULL << 6)  // 访问位
#define PTE_D (1ULL << 7)  // 脏位

// PTE 页大小位（Sv39支持）
// 注意：PTE_PS与PTE_D共享第7位，只在L1页表项中有效
#define PTE_PS (1ULL << 7)  // 页大小位，1表示使用大页

// PTE PPN 字段
#define PTE_PPN(pte) ((pte) >> 10)
#define PPN_PTE(ppn) ((ppn) << 10)

// 页表类型
typedef uint64_t *pagetable_t;

// 函数声明
pagetable_t pagetable_create(void);
void pagetable_destroy(pagetable_t pagetable);
uint64_t *walk(pagetable_t pagetable, uint64_t va, int alloc);
void map_pages(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t size, uint64_t perm);
void unmap_pages(pagetable_t pagetable, uint64_t va, uint64_t npages, int do_free);
int copyin(pagetable_t pagetable, void *dst, uint64_t srcva, uint64_t len);
int copyout(pagetable_t pagetable, uint64_t dstva, const void *src, uint64_t len);

// 内核页表创建
pagetable_t kernel_pagetable_create(void);

// satp 相关
#define SATP_MODE_SV39 (8ULL << 60)
#define SATP_PPN(ppn) ((ppn) << 0)
#define MAKE_SATP(mode, ppn) ((mode) | (ppn))

// 地址空间切换
void switch_satp(pagetable_t pagetable);

// satp 相关函数
uint64_t read_satp(void);

// 自检函数
void satp_precheck(pagetable_t pagetable);
void vm_selftest(void);

#endif // VM_H
