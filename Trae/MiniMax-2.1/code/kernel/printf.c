#include "printf.h"
#include "sbi.h"
#include <stdarg.h>
#include <stddef.h>

static void print_char(char c) {
    if (c == '\n') {
        sbi_console_putchar('\r');
    }
    sbi_console_putchar(c);
}

static void print_str(const char *s) {
    if (s == NULL) {
        s = "(null)";
    }
    while (*s) {
        print_char(*s++);
    }
}

static void print_uint(uint64_t x, int base) {
    char buf[32];
    int i = 0;
    
    if (x == 0) {
        print_char('0');
        return;
    }
    
    while (x > 0) {
        buf[i++] = "0123456789abcdef"[x % base];
        x /= base;
    }
    
    while (i-- > 0) {
        print_char(buf[i]);
    }
}

static void print_int(int64_t x, int base) {
    if (x < 0) {
        print_char('-');
        print_uint((uint64_t)(-x), base);
    } else {
        print_uint((uint64_t)x, base);
    }
}

void kvprintf(const char *fmt, va_list ap) {
    const char *p = fmt;
    
    while (*p) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 'c':
                    print_char(va_arg(ap, int));
                    break;
                case 's':
                    print_str(va_arg(ap, const char *));
                    break;
                case 'd':
                case 'i':
                    print_int(va_arg(ap, int64_t), 10);
                    break;
                case 'u':
                    print_uint(va_arg(ap, uint64_t), 10);
                    break;
                case 'x':
                case 'p':
                    print_uint(va_arg(ap, uint64_t), 16);
                    break;
                case 'l':
                    p++;
                    switch (*p) {
                        case 'd':
                        case 'i':
                            print_int(va_arg(ap, int64_t), 10);
                            break;
                        case 'u':
                            print_uint(va_arg(ap, uint64_t), 10);
                            break;
                        case 'x':
                            print_uint(va_arg(ap, uint64_t), 16);
                            break;
                        default:
                            p--;
                            break;
                    }
                    break;
                default:
                    p--;
                    break;
            }
        } else {
            print_char(*p);
        }
        p++;
    }
}

void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
}
