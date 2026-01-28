#ifndef TRAP_H
#define TRAP_H

#include "riscv.h"

// 陷阱帧结构，字段顺序必须与trapvec保存顺序严格一致
typedef struct trapframe {
    // 通用寄存器
    uint64_t ra;
    uint64_t sp;
    uint64_t gp;
    uint64_t tp;
    uint64_t t0;
    uint64_t t1;
    uint64_t t2;
    uint64_t s0;
    uint64_t s1;
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t a4;
    uint64_t a5;
    uint64_t a6;
    uint64_t a7;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;
    uint64_t t3;
    uint64_t t4;
    uint64_t t5;
    uint64_t t6;
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t usp;
} trapframe_t;

// 外部符号声明
extern void kernel_trap_vector(void);

// 函数声明
void trap_init(void);
void trap_enable(void);
void trap_handler(trapframe_t *tf);

#endif // TRAP_H