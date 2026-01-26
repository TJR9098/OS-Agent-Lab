#include <stdarg.h>
#include <stdint.h>

#include "printf.h"
#include "uart.h"

static int putch(char c) {
  uart_putc(c);
  return 1;
}

static int putsn(const char *s) {
  int n = 0;
  while (*s) {
    n += putch(*s++);
  }
  return n;
}

static int print_uint(uint64_t val, unsigned base, int upper) {
  char buf[32];
  int i = 0;
  int n = 0;

  if (val == 0) {
    return putch('0');
  }

  while (val) {
    uint64_t digit = val % base;
    if (digit < 10) {
      buf[i++] = (char)('0' + digit);
    } else {
      buf[i++] = (char)((upper ? 'A' : 'a') + (digit - 10));
    }
    val /= base;
  }

  while (i--) {
    n += putch(buf[i]);
  }
  return n;
}

static int print_int(int64_t val) {
  if (val < 0) {
    int n = putch('-');
    return n + print_uint((uint64_t)(-val), 10, 0);
  }
  return print_uint((uint64_t)val, 10, 0);
}

int fw_vprintf(const char *fmt, va_list ap) {
  int n = 0;

  while (*fmt) {
    if (*fmt != '%') {
      n += putch(*fmt++);
      continue;
    }

    fmt++;
    if (*fmt == '%') {
      n += putch('%');
      fmt++;
      continue;
    }

    int long_count = 0;
    while (*fmt == 'l') {
      long_count++;
      fmt++;
    }

    switch (*fmt) {
      case 'c': {
        int c = va_arg(ap, int);
        n += putch((char)c);
        break;
      }
      case 's': {
        const char *s = va_arg(ap, const char *);
        if (!s) {
          s = "(null)";
        }
        n += putsn(s);
        break;
      }
      case 'd': {
        if (long_count) {
          int64_t v = va_arg(ap, int64_t);
          n += print_int(v);
        } else {
          int v = va_arg(ap, int);
          n += print_int(v);
        }
        break;
      }
      case 'u': {
        if (long_count) {
          uint64_t v = va_arg(ap, uint64_t);
          n += print_uint(v, 10, 0);
        } else {
          unsigned int v = va_arg(ap, unsigned int);
          n += print_uint(v, 10, 0);
        }
        break;
      }
      case 'x': {
        if (long_count) {
          uint64_t v = va_arg(ap, uint64_t);
          n += print_uint(v, 16, 0);
        } else {
          unsigned int v = va_arg(ap, unsigned int);
          n += print_uint(v, 16, 0);
        }
        break;
      }
      case 'p': {
        uintptr_t v = (uintptr_t)va_arg(ap, void *);
        n += putsn("0x");
        n += print_uint((uint64_t)v, 16, 0);
        break;
      }
      default:
        n += putch('%');
        n += putch(*fmt);
        break;
    }

    fmt++;
  }

  return n;
}

int fw_printf(const char *fmt, ...) {
  va_list ap;
  int n;

  va_start(ap, fmt);
  n = fw_vprintf(fmt, ap);
  va_end(ap);

  return n;
}