#include "timer.h"
#include "sbi.h"
#include "riscv.h"
#include "printf.h"

// 定时器频率 (Hz)
#define TICK_HZ 1

// QEMU virt的timebase-frequency (10MHz)
#define TIMEBASE_FREQ 10000000

// 全局tick计数器
static uint64_t ticks = 0;

// 初始化定时器
void timer_init(void) {
    // 计算定时器间隔 (10MHz / 100Hz = 100000 ticks per interrupt)
    uint64_t interval = TIMEBASE_FREQ / TICK_HZ;
    
    // 设置首次定时器中断
    sbi_set_timer(csr_read(time) + interval);
    
    kprintf("tick_hz: %d\n", TICK_HZ);
    kprintf("timer_init ok\n");
}

// 定时器中断处理
void timer_handle(void) {
    // 递增tick计数器
    ticks++;
    
    // 计算下一次定时器中断时间
    uint64_t interval = TIMEBASE_FREQ / TICK_HZ;
    sbi_set_timer(csr_read(time) + interval);
    
    // 打印每次tick信息，便于调试
    kprintf("timer: tick=%ld\n", ticks);
    
    // 每秒打印一次完整tick信息
    if (ticks % TICK_HZ == 0) {
        kprintf("timer: second=%ld\n", ticks / TICK_HZ);
    }
}

// 获取当前tick数
uint64_t timer_get_tick(void) {
    return ticks;
}
