#include "sbi.h"
#include "riscv.h"
#include "printf.h"

// 简化的SBI调用实现
void sbi_console_putchar(int ch) {
    asm volatile (
        "mv a0, %0\n"
        "li a7, 0x1\n"
        "ecall"
        :
        : "r"(ch)
        : "memory", "a0", "a7"
    );
}

void sbi_set_timer(uint64_t stime) {
    asm volatile (
        "mv a0, %0\n"
        "li a7, 0x54494D45\n"
        "li a6, 0\n"
        "ecall"
        :
        : "r"(stime)
        : "memory", "a0", "a6", "a7"
    );
}

void sbi_shutdown() {
    // 打印即将关机的信息
    kprintf("[SBI] System shutting down...\n");

    // 尝试使用System Reset Extension
    asm volatile (
        "li a7, 0x53525354\n"
        "li a6, 0\n"
        "li a0, 0\n"
        "ecall"
        :
        :
        : "memory", "a0", "a6", "a7"
    );

    // 回退到legacy shutdown
    asm volatile (
        "li a7, 0x08\n"
        "ecall"
        :
        :
        : "memory", "a7"
    );

    // 关闭中断并进入停机循环
    csr_clear(sstatus, SSTATUS_SIE);
    while (1) {
        asm volatile ("wfi");
    }
}
