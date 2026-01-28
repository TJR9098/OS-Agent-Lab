#ifndef MEMLAYOUT_H
#define MEMLAYOUT_H

#include <stdint.h>
#include "vm.h"

// DRAM 基本地址
#define DRAM_BASE 0x80000000
#define DRAM_SIZE 0x8000000  // 128MB

// 内核加载地址
#define KERNEL_PHYS_BASE 0x80200000

// QEMU virt 设备 MMIO 映射常量

// UART
#define UART_BASE 0x10000000
#define UART_SIZE 0x100

// PLIC (Platform Level Interrupt Controller)
#define PLIC_BASE 0xc000000
#define PLIC_SIZE 0x400000

// CLINT (Core Local Interruptor)
#define CLINT_BASE 0x2000000
#define CLINT_SIZE 0x10000

// Virtio MMIO
#define VIRTIO_BASE 0x10001000
#define VIRTIO_SIZE 0x1000

// 设备映射权限
#define DEV_PERM (PTE_R | PTE_W)

// 内核映射权限
#define KERNEL_PERM (PTE_R | PTE_W | PTE_X)

// 用户映射权限
#define USER_PERM (PTE_R | PTE_W | PTE_X | PTE_U)

#endif // MEMLAYOUT_H
