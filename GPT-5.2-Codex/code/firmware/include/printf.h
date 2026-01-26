#pragma once

#include <stdarg.h>

int fw_vprintf(const char *fmt, va_list ap);
int fw_printf(const char *fmt, ...);