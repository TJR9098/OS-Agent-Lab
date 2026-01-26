#ifndef __RISCV_H__
#define __RISCV_H__

#include <stdint.h>

// CSR寄存器操作
#define csr_read(csr) ({ u64 __v; asm volatile ("csrr %0, " #csr : "=r"(__v)); __v; })
#define csr_write(csr, val) ({ asm volatile ("csrw " #csr ", %0" :: "r"(val)); })
#define csr_set(csr, val) ({ asm volatile ("csrs " #csr ", %0" :: "r"(val)); })
#define csr_clear(csr, val) ({ asm volatile ("csrc " #csr ", %0" :: "r"(val)); })

// SSTATUS寄存器位定义
#define SSTATUS_SPP  (1L << 8)
#define SSTATUS_SPIE (1L << 5)
#define SSTATUS_UPIE (1L << 4)
#define SSTATUS_SIE  (1L << 1)
#define SSTATUS_UIE  (1L << 0)

// SIE寄存器位定义
#define SIE_SEIE (1L << 9)
#define SIE_STIE (1L << 5)
#define SIE_SSIE (1L << 1)

// SCAUSE寄存器相关
#define SCAUSE_INTERRUPT (1UL << 63)
#define SCAUSE_EXCEPTION (0UL << 63)

// 异常类型
#define EXC_INST_MISALIGNED 0
#define EXC_INST_ACCESS     1
#define EXC_ILLEGAL_INST    2
#define EXC_BREAKPOINT      3
#define EXC_LOAD_MISALIGNED 4
#define EXC_LOAD_ACCESS     5
#define EXC_STORE_MISALIGNED 6
#define EXC_STORE_ACCESS    7
#define EXC_ECALL_U         8
#define EXC_ECALL_S         9
#define EXC_ECALL_M         11

// 中断类型
#define IRQ_S_SOFT     1
#define IRQ_S_TIMER    5
#define IRQ_S_EXT      9

// 类型定义
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#endif /* __RISCV_H__ */
