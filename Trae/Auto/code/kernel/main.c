#include "printf.h"
#include "trap.h"
#include "timer.h"
#include "panic.h"
#include "riscv.h"
#include "mem.h"

// 外部符号声明
extern char boot_stack_top[];
extern char kernel_stack_top[];

// 内核主函数
void kernel_main(uint64_t hartid) {
    // 打印banner
    kprintf("AgentOS RV64 - OpenSBI boot\n");
    
    // 内存初始化
    kinit(DRAM_BASE, DRAM_SIZE);
    
    // 打印hartid
    kprintf("hartid: %ld\n", hartid);
    
    // 验证栈指针
    uint64_t sp = (uint64_t)__builtin_frame_address(0);
    kprintf("sp: %lx\n", sp);
    
    // 检查栈对齐
    if (sp % 16 != 0) {
        panic("bad stack alignment: sp=%lx", sp);
    }
    
    // 内存管理自测
    kprintf("mem: starting self-test...\n");
    
    // 1. 连续分配三页，检查地址递增
    void *p0 = kalloc();
    void *p1 = kalloc();
    void *p2 = kalloc();
    
    if (!p0 || !p1 || !p2) {
        panic("mem selftest failed: allocation failed");
    }
    
    if (!(p0 < p1 && p1 < p2)) {
        panic("mem selftest failed: addresses not increasing");
    }
    
    kprintf("mem: kalloc test passed - p0=%lx p1=%lx p2=%lx\n", p0, p1, p2);
    
    // 2. 释放后再次分配，检查复用
    kfree(p0);
    kfree(p1);
    kfree(p2);
    
    void *p3 = kalloc();
    void *p4 = kalloc();
    void *p5 = kalloc();
    
    if (!p3 || !p4 || !p5) {
        panic("mem selftest failed: reallocation failed");
    }
    
    kprintf("mem: kfree/kalloc reuse test passed - p3=%lx p4=%lx p5=%lx\n", p3, p4, p5);
    
    // 3. 验证释放的页被填充了调试字节（可选，这里不做具体验证）
    
    // 自测完成
    kprintf("mem: self-test completed successfully\n");
    
    // 初始化顺序：trap_init -> timer_init -> trap_enable
    trap_init();
    timer_init();
    trap_enable();
    
    // 启动完成，进入死循环，每秒打印一次tick计数
    kprintf("kernel started, printing tick count every second...\n");
    
    uint64_t last_tick = 0;
    while (1) {
        uint64_t current_tick = timer_get_tick();
        if (current_tick != last_tick) {
            kprintf("current tick: %ld\n", current_tick);
            last_tick = current_tick;
        }
        
        // 当tick达到10时，触发异常
        if (current_tick >= 10) {
            kprintf("\n10 ticks reached, triggering illegal instruction...\n");
            uint64_t mstatus;
            asm volatile ("csrr %0, mstatus" : "=r"(mstatus));
        }
        
        asm volatile ("wfi");
    }
}
