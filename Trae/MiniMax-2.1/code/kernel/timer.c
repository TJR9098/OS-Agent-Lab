#include "timer.h"
#include "sbi.h"
#include "printf.h"
#include "riscv.h"

static uint64_t tick = 0;
static uint64_t next_deadline = 0;

void timer_init(void) {
    tick = 0;
    next_deadline = r_time() + 10000000UL;
    sbi_set_timer(next_deadline);
}

void timer_handle(void) {
    tick++;
    next_deadline = r_time() + 10000000UL;
    sbi_set_timer(next_deadline);
    
    kprintf("timer: tick=%lu\n", tick);
}

uint64_t get_ticks(void) {
    return tick;
}
