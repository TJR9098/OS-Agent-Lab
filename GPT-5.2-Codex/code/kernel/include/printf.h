#pragma once

#include <stdarg.h>

int kvprintf(const char *fmt, va_list ap);
int kprintf(const char *fmt, ...);