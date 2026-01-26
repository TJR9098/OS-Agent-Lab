#include "printf.h"
#include "sbi.h"

// 内部函数：输出字符串
static void putstr(const char *s) {
    if (!s) {
        putstr("(null)");
        return;
    }
    while (*s) {
        if (*s == '\n') {
            sbi_console_putchar('\r');
        }
        sbi_console_putchar(*s++);
    }
}

// 内部函数：输出无符号整数
static void putuint(uint64_t n, int base, int uppercase) {
    char buf[65];
    char *p = buf + sizeof(buf) - 1;
    *p = '\0';
    if (n == 0) {
        *--p = '0';
    } else {
        while (n > 0) {
            int digit = n % base;
            *--p = (digit < 10) ? (digit + '0') : (digit - 10 + (uppercase ? 'A' : 'a'));
            n /= base;
        }
    }
    putstr(p);
}

// 内部函数：输出有符号整数
static void putint(int64_t n, int base, int uppercase) {
    if (n < 0) {
        sbi_console_putchar('-');
        n = -n;
    }
    putuint((uint64_t)n, base, uppercase);
}

// 实现kvprintf函数
void kvprintf(const char *fmt, va_list ap) {
    while (*fmt) {
        if (*fmt != '%') {
            if (*fmt == '\n') {
                sbi_console_putchar('\r');
            }
            sbi_console_putchar(*fmt++);
            continue;
        }
        fmt++; // 跳过 '%'
        int is_long = 0;
        if (*fmt == 'l') {
            is_long = 1;
            fmt++;
        }
        switch (*fmt) {
            case 'c': {
                int c = va_arg(ap, int);
                sbi_console_putchar(c);
                break;
            }
            case 's': {
                char *s = va_arg(ap, char *);
                putstr(s);
                break;
            }
            case 'd': {
                if (is_long) {
                    int64_t n = va_arg(ap, int64_t);
                    putint(n, 10, 0);
                } else {
                    int n = va_arg(ap, int);
                    putint(n, 10, 0);
                }
                break;
            }
            case 'u': {
                if (is_long) {
                    uint64_t n = va_arg(ap, uint64_t);
                    putuint(n, 10, 0);
                } else {
                    uint32_t n = va_arg(ap, uint32_t);
                    putuint(n, 10, 0);
                }
                break;
            }
            case 'x': {
                if (is_long) {
                    uint64_t n = va_arg(ap, uint64_t);
                    putuint(n, 16, 0);
                } else {
                    uint32_t n = va_arg(ap, uint32_t);
                    putuint(n, 16, 0);
                }
                break;
            }
            case 'p': {
                void *p = va_arg(ap, void *);
                putstr("0x");
                putuint((uint64_t)p, 16, 0);
                break;
            }
            default:
                sbi_console_putchar(*fmt);
                break;
        }
        fmt++;
    }
}

// 实现kprintf函数
void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
}
