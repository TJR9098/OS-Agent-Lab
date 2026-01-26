#ifndef _PRINTF_H
#define _PRINTF_H

#include <stdarg.h>

void kprintf(const char *fmt, ...);
void kvprintf(const char *fmt, va_list ap);

#endif
