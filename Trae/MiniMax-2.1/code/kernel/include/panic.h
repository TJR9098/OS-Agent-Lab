#ifndef _PANIC_H
#define _PANIC_H

#include <stdarg.h>
#include <stdint.h>

void set_hartid(uint64_t hartid);
void panic(const char *fmt, ...);
void panic_trap(uint64_t scause, uint64_t sepc, uint64_t stval, const char *msg);

#endif
