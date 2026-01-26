#include "trap.h"
#include "riscv.h"
#include "printf.h"
#include "panic.h"
#include "timer.h"

// trap初始化
void trap_init(void) {
    // 设置stvec为kernel_trap_vector，使用direct mode
    csr_write(stvec, (uint64_t)kernel_trap_vector);
    
    // 初始化sscratch为kernel栈顶
    extern char kernel_stack_top[];
    csr_write(sscratch, (uint64_t)kernel_stack_top);
    
    kprintf("trap_init ok: stvec=%lx, sscratch=%lx\n", 
            csr_read(stvec), csr_read(sscratch));
}

// 启用trap
void trap_enable(void) {
    // 打开SIE (Supervisor Interrupt Enable)
    csr_set(sstatus, SSTATUS_SIE);
    
    // 打开STIE (Supervisor Timer Interrupt Enable)
    csr_set(sie, SIE_STIE);
    
    kprintf("trap_enable ok\n");
}

// trap处理函数
void trap_handler(trapframe_t *tf) {
    uint64_t scause = csr_read(scause);
    uint64_t sepc = csr_read(sepc);
    uint64_t stval = csr_read(stval);
    
    // 检查是否为中断
    if (scause & SCAUSE_INTERRUPT) {
        uint64_t irq = scause & ~SCAUSE_INTERRUPT;
        
        // 处理定时器中断，不需要打印信息
        if (irq == IRQ_S_TIMER) {
            timer_handle();
            return;
        }
        
        // 其他中断打印信息
        kprintf("trap_handler called: scause=%lx, sepc=%lx, stval=%lx\n", 
                scause, sepc, stval);
        kprintf("  interrupt: irq=%lx\n", irq);
        
        // 其他中断暂不处理
        kprintf("  unhandled interrupt: %lx\n", irq);
        panic_trap(scause, sepc, stval, "unhandled interrupt");
    } else {
        // 处理异常，打印信息
        uint64_t exception = scause & ~SCAUSE_INTERRUPT;
        kprintf("trap_handler called: scause=%lx, sepc=%lx, stval=%lx\n", 
                scause, sepc, stval);
        kprintf("  exception: type=%lx\n", exception);
        
        switch (exception) {
            case EXC_ILLEGAL_INST:
                kprintf("  illegal instruction exception\n");
                panic_trap(scause, sepc, stval, "illegal instruction");
                break;
                
            case EXC_LOAD_ACCESS:
                kprintf("  load access fault\n");
                panic_trap(scause, sepc, stval, "load access fault");
                break;
                
            case EXC_STORE_ACCESS:
                kprintf("  store/amo access fault\n");
                panic_trap(scause, sepc, stval, "store/amo access fault");
                break;
                
            case EXC_ECALL_U:
                kprintf("  ecall from U-mode\n");
                // 跳过这条指令
                csr_write(sepc, sepc + 4);
                break;
                
            default:
                kprintf("  unhandled exception: %lx\n", exception);
                panic_trap(scause, sepc, stval, "unhandled exception");
                break;
        }
    }
}
