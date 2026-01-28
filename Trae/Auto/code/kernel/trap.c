#include "trap.h"
#include "riscv.h"
#include "printf.h"
#include "panic.h"
#include "timer.h"

// 故障捕获结构（与test_vm.c中的定义保持一致）
typedef struct {
    int armed;
    int hit;
    uint64_t scause;
    uint64_t stval;
    uint64_t sepc;
} vmtest_fault;

// 声明全局故障捕获变量（在test_vm.c中定义）
extern vmtest_fault fault_harness;

// trap初始化
void trap_init(void) {
    // 设置stvec为kernel_trap_vector，使用direct mode
    csr_write(stvec, (uint64_t)kernel_trap_vector);
    
    // 初始化sscratch为kernel栈顶
    extern char kernel_stack_top[];
    csr_write(sscratch, (uint64_t)kernel_stack_top);
    
    kprintf("trap_init ok: stvec=0x%lx, sscratch=0x%lx\n", 
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

// 判断是否为用户模式
static int is_user_mode(void) {
    return (csr_read(sstatus) & SSTATUS_SPP) == 0;
}

// page fault处理
static void handle_page_fault(uint64_t scause, uint64_t sepc, uint64_t stval, trapframe_t *tf) {
    int user_fault = is_user_mode();
    char *fault_kind = "unknown";

    // 确定fault类型
    switch (scause) {
        case EXC_INST_PAGE_FAULT:
            fault_kind = "inst";
            break;
        case EXC_LOAD_PAGE_FAULT:
            fault_kind = "load";
            break;
        case EXC_STORE_PAGE_FAULT:
            fault_kind = "store";
            break;
        default:
            fault_kind = "unknown";
            break;
    }

    // 输出符合要求的格式
    kprintf("[VMFAULT] kind=%s scause=%ld sepc=0x%lx stval=0x%lx mode=%s\n", 
            fault_kind, scause, sepc, stval, user_fault ? "U" : "S");

    if (user_fault) {
        kprintf("user fault, kill\n");
        // 这里简化处理，实际应该终止用户进程
        // 跳过错误指令
        tf->sepc += 4;
    } else {
        // tf->sepc += 4;
        kprintf("tf->sepc = 0x%lx\n", tf->sepc);
        panic_trap(scause, sepc, stval, "kernel page fault");
    }
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
        kprintf("trap_handler called: scause=0x%lx, sepc=0x%lx, stval=0x%lx\n", 
                scause, sepc, stval);
        kprintf("  interrupt: irq=0x%lx\n", irq);
        
        // 其他中断暂不处理
        kprintf("  unhandled interrupt: 0x%lx\n", irq);
        panic_trap(scause, sepc, stval, "unhandled interrupt");
    } else {
        // 处理异常，打印信息
        uint64_t exception = scause & ~SCAUSE_INTERRUPT;
        
        // 处理page fault
        switch (exception) {
            case EXC_INST_PAGE_FAULT:
            case EXC_LOAD_PAGE_FAULT:
            case EXC_STORE_PAGE_FAULT:
                handle_page_fault(scause, sepc, stval, tf);
                return;
            
            case EXC_ILLEGAL_INST:
                kprintf("trap_handler called: scause=0x%lx, sepc=0x%lx, stval=0x%lx\n", 
                        scause, sepc, stval);
                kprintf("  exception: illegal instruction\n");
                panic_trap(scause, sepc, stval, "illegal instruction");
                break;
                
            case EXC_LOAD_ACCESS:
                kprintf("trap_handler called: scause=0x%lx, sepc=0x%lx, stval=0x%lx\n", 
                        scause, sepc, stval);
                kprintf("  exception: load access fault\n");
                panic_trap(scause, sepc, stval, "load access fault");
                break;
                
            case EXC_STORE_ACCESS:
                kprintf("trap_handler called: scause=0x%lx, sepc=0x%lx, stval=0x%lx\n", 
                        scause, sepc, stval);
                kprintf("  exception: store/amo access fault\n");
                panic_trap(scause, sepc, stval, "store/amo access fault");
                break;
                
            case EXC_ECALL_U:
                kprintf("trap_handler called: scause=0x%lx, sepc=0x%lx, stval=0x%lx\n", 
                        scause, sepc, stval);
                kprintf("  exception: ecall from U-mode\n");
                // 跳过这条指令
                csr_write(sepc, sepc + 4);
                break;
                
            default:
                kprintf("trap_handler called: scause=0x%lx, sepc=0x%lx, stval=0x%lx\n", 
                        scause, sepc, stval);
                kprintf("  exception: unhandled exception=0x%lx\n", exception);
                panic_trap(scause, sepc, stval, "unhandled exception");
                break;
        }
    }
}
