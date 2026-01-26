#include "trap.h"
#include "printf.h"
#include "panic.h"
#include "riscv.h"

extern char kernel_trap_vector;

void trap_init(void) {
    w_stvec((uint64_t)&kernel_trap_vector);
}

void trap_enable(void) {
    uint64_t sstatus = r_sstatus();
    sstatus |= 1UL << 1;
    w_sstatus(sstatus);
    
    uint64_t sie = r_sie();
    sie |= 1UL << 5;
    w_sie(sie);
}

void kernel_trap_handler(struct trapframe *tf) {
    uint64_t scause = r_scause();
    uint64_t sepc = r_sepc();
    uint64_t stval = r_stval();
    int is_interrupt = (scause >> 63) & 1;
    
    if (is_interrupt) {
        uint64_t interrupt_id = scause & 0x7FFFFFFFFFFFFFFFULL;
        if (interrupt_id == 5) {
            extern void timer_handle(void);
            timer_handle();
            return;
        }
    } else {
        switch (scause) {
            case 2:
                kprintf("illegal instruction\n");
                kprintf("scause: 0x%lx, sepc: 0x%lx, stval: 0x%lx\n", scause, sepc, stval);
                panic_trap(scause, sepc, stval, "illegal instruction");
                break;
            case 5:
                kprintf("load access fault\n");
                kprintf("scause: 0x%lx, sepc: 0x%lx, stval: 0x%lx\n", scause, sepc, stval);
                panic_trap(scause, sepc, stval, "load access fault");
                break;
            case 7:
                kprintf("store/amo access fault\n");
                kprintf("scause: 0x%lx, sepc: 0x%lx, stval: 0x%lx\n", scause, sepc, stval);
                panic_trap(scause, sepc, stval, "store/amo access fault");
                break;
            case 8:
                kprintf("ecall from U-mode\n");
                kprintf("scause: 0x%lx, sepc: 0x%lx, stval: 0x%lx\n", scause, sepc, stval);
                w_sepc(sepc + 4);
                return;
            default:
                kprintf("unknown exception\n");
                kprintf("scause: 0x%lx, sepc: 0x%lx, stval: 0x%lx\n", scause, sepc, stval);
                panic_trap(scause, sepc, stval, "unknown exception");
                break;
        }
    }
    
    panic_trap(scause, sepc, stval, "unhandled trap");
}
