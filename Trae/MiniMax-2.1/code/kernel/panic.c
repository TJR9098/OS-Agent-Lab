#include "panic.h"
#include "printf.h"
#include "sbi.h"
#include "riscv.h"
#include <stdarg.h>

static uint64_t current_hartid = 0;

void set_hartid(uint64_t hartid) {
    current_hartid = hartid;
}

void panic(const char *fmt, ...) {
    va_list ap;
    
    kprintf("panic: ");
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
    kprintf("\n");
    kprintf("hartid: %lu\n", current_hartid);
    
    w_sstatus(0);
    sbi_shutdown();
}

void panic_trap(uint64_t scause, uint64_t sepc, uint64_t stval, const char *msg) {
    kprintf("panic: %s\n", msg);
    kprintf("scause: 0x%lx\n", scause);
    kprintf("sepc: 0x%lx\n", sepc);
    kprintf("stval: 0x%lx\n", stval);
    kprintf("hartid: %lu\n", current_hartid);
    
    w_sstatus(0);
    sbi_shutdown();
}
