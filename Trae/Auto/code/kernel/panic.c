#include "panic.h"
#include "printf.h"
#include "sbi.h"
#include "riscv.h"

// 通用panic函数实现
void panic(const char *fmt, ...) {
    va_list ap;
    
    // 输出panic前缀
    kprintf("panic: ");
    
    // 输出格式化信息
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
    
    kprintf("\n");
    
    // 关闭中断
    csr_clear(sstatus, SSTATUS_SIE);
    
    // 调用SBI关机
    sbi_shutdown();
}

// trap相关panic函数实现
void panic_trap(uint64_t scause, uint64_t sepc, uint64_t stval, const char *msg) {
    // 输出panic前缀和消息
    kprintf("panic: ");
    if (msg) {
        kprintf("%s\n", msg);
    }
    
    // 输出trap相关信息
    kprintf("scause: 0x%lx\n", scause);
    kprintf("sepc: 0x%lx\n", sepc);
    kprintf("stval: 0x%lx\n", stval);
    
    // 关闭中断
    csr_clear(sstatus, SSTATUS_SIE);
    
    // 调用SBI关机
    sbi_shutdown();
}
