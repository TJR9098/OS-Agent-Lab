#ifndef _RISCV_H
#define _RISCV_H

#include <stdint.h>

static inline uint64_t r_mstatus(void) {
    uint64_t x;
    asm volatile("csrr %0, mstatus" : "=r"(x));
    return x;
}

static inline void w_mstatus(uint64_t x) {
    asm volatile("csrw mstatus, %0" : : "r"(x));
}

static inline uint64_t r_mie(void) {
    uint64_t x;
    asm volatile("csrr %0, mie" : "=r"(x));
    return x;
}

static inline void w_mie(uint64_t x) {
    asm volatile("csrw mie, %0" : : "r"(x));
}

static inline uint64_t r_mtvec(void) {
    uint64_t x;
    asm volatile("csrr %0, mtvec" : "=r"(x));
    return x;
}

static inline void w_mtvec(uint64_t x) {
    asm volatile("csrw mtvec, %0" : : "r"(x));
}

static inline uint64_t r_mscratch(void) {
    uint64_t x;
    asm volatile("csrr %0, mscratch" : "=r"(x));
    return x;
}

static inline void w_mscratch(uint64_t x) {
    asm volatile("csrw mscratch, %0" : : "r"(x));
}

static inline uint64_t r_mepc(void) {
    uint64_t x;
    asm volatile("csrr %0, mepc" : "=r"(x));
    return x;
}

static inline void w_mepc(uint64_t x) {
    asm volatile("csrw mepc, %0" : : "r"(x));
}

static inline uint64_t r_mcause(void) {
    uint64_t x;
    asm volatile("csrr %0, mcause" : "=r"(x));
    return x;
}

static inline uint64_t r_mtval(void) {
    uint64_t x;
    asm volatile("csrr %0, mtval" : "=r"(x));
    return x;
}

static inline uint64_t r_mhartid(void) {
    uint64_t x;
    asm volatile("csrr %0, mhartid" : "=r"(x));
    return x;
}

static inline uint64_t r_sstatus(void) {
    uint64_t x;
    asm volatile("csrr %0, sstatus" : "=r"(x));
    return x;
}

static inline void w_sstatus(uint64_t x) {
    asm volatile("csrw sstatus, %0" : : "r"(x));
}

static inline uint64_t r_sie(void) {
    uint64_t x;
    asm volatile("csrr %0, sie" : "=r"(x));
    return x;
}

static inline void w_sie(uint64_t x) {
    asm volatile("csrw sie, %0" : : "r"(x));
}

static inline uint64_t r_stvec(void) {
    uint64_t x;
    asm volatile("csrr %0, stvec" : "=r"(x));
    return x;
}

static inline void w_stvec(uint64_t x) {
    asm volatile("csrw stvec, %0" : : "r"(x));
}

static inline uint64_t r_sscratch(void) {
    uint64_t x;
    asm volatile("csrr %0, sscratch" : "=r"(x));
    return x;
}

static inline void w_sscratch(uint64_t x) {
    asm volatile("csrw sscratch, %0" : : "r"(x));
}

static inline uint64_t r_sepc(void) {
    uint64_t x;
    asm volatile("csrr %0, sepc" : "=r"(x));
    return x;
}

static inline void w_sepc(uint64_t x) {
    asm volatile("csrw sepc, %0" : : "r"(x));
}

static inline uint64_t r_scause(void) {
    uint64_t x;
    asm volatile("csrr %0, scause" : "=r"(x));
    return x;
}

static inline uint64_t r_stval(void) {
    uint64_t x;
    asm volatile("csrr %0, stval" : "=r"(x));
    return x;
}

static inline uint64_t r_satp(void) {
    uint64_t x;
    asm volatile("csrr %0, satp" : "=r"(x));
    return x;
}

static inline void w_satp(uint64_t x) {
    asm volatile("csrw satp, %0" : : "r"(x));
}

static inline void sfence_vma(void) {
    asm volatile("sfence.vma" : : );
}

static inline void w_sepc_val(uint64_t x) {
    asm volatile("csrw sepc, %0" : : "r"(x));
}

static inline void w_sstatus_val(uint64_t x) {
    asm volatile("csrw sstatus, %0" : : "r"(x));
}

static inline uint64_t r_time(void) {
    uint64_t x;
    asm volatile("csrr %0, time" : "=r"(x));
    return x;
}

static inline void w_timecmp(uint64_t x) {
    asm volatile("csrw timecmp, %0" : : "r"(x));
}

static inline void asm_wfi(void) {
    asm volatile("wfi" : : );
}

static inline void asm_mret(void) {
    asm volatile("mret" : : );
}

static inline void asm_sret(void) {
    asm volatile("sret" : : );
}

static inline void asm_nop(void) {
    asm volatile("nop" : : );
}

#endif
