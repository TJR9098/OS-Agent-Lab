#ifndef __PRINTF_H__
#define __PRINTF_H__

#include <stdarg.h>
#include <stddef.h>

// 内核打印函数
void kprintf(const char *fmt, ...);
void kvprintf(const char *fmt, va_list ap);

#endif /* __PRINTF_H__ */
