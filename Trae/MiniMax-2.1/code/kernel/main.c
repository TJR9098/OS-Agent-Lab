#include "printf.h"
#include "trap.h"
#include "timer.h"
#include "panic.h"
#include "kalloc.h"
#include "riscv.h"
#include "sbi.h"
#include "memlayout.h"
#include <stddef.h>

extern char boot_stack_top[];
extern char kernel_stack_top[];

static uint64_t hartid = 0;

void kernel_main(uint64_t mhartid) {
    hartid = mhartid;
    set_hartid(hartid);
    
    kprintf("AgentOS RV64 - OpenSBI boot\n");
    
    kinit(DRAM_BASE, DRAM_SIZE);
    
    kprintf("hartid: %lu\n", hartid);
    kprintf("timebase: 1000000\n");
    kprintf("tick_hz: %d\n", TICK_HZ);
    
    trap_init();
    kprintf("trap_init ok\n");
    
    timer_init();
    kprintf("timer_init ok\n");
    
    trap_enable();
    kprintf("trap_enable ok\n");
    
    kprintf("mem selftest...\n");
    void *p0 = kalloc();
    void *p1 = kalloc();
    void *p2 = kalloc();
    if (p0 == NULL || p1 == NULL || p2 == NULL) {
        panic("mem selftest failed");
    }
    if (!((uintptr_t)p0 < (uintptr_t)p1 && (uintptr_t)p1 < (uintptr_t)p2)) {
        panic("mem selftest failed");
    }
    
    kfree(p1);
    void *p3 = kalloc();
    if (p3 == NULL) {
        panic("mem selftest failed");
    }
    if ((uintptr_t)p3 != (uintptr_t)p1) {
        panic("mem selftest failed");
    }
    kprintf("mem selftest ok\n");
    
    kprintf("Waiting for 5 timer ticks...\n");
    while (get_ticks() < 5) {
        asm_wfi();
    }
    kprintf("Timer ticks complete\n");
    
    kprintf("Triggering illegal instruction test (reading mstatus from S-mode)...\n");
    uint64_t mstatus = r_mstatus();
    kprintf("mstatus: 0x%lx\n", mstatus);
    
    panic("manual panic");
}
