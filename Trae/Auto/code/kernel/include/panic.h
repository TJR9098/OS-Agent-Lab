#ifndef PANIC_H
#define PANIC_H

#include <stdint.h>
#include <stdarg.h>

// 普通panic函数
void panic(const char *fmt, ...);

// 陷阱panic函数
void panic_trap(uint64_t scause, uint64_t sepc, uint64_t stval, const char *msg);

#endif // PANIC_H