#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef int (*out_fn)(void *ctx, const char *buf, size_t len);

struct buf_ctx {
  char *buf;
  size_t size;
  size_t pos;
};

static int out_fd(void *ctx, const char *buf, size_t len) {
  int fd = *(int *)ctx;
  if (len == 0) {
    return 0;
  }
  return (int)write(fd, buf, len);
}

static int out_buf(void *ctx, const char *buf, size_t len) {
  struct buf_ctx *b = (struct buf_ctx *)ctx;
  size_t avail = 0;
  if (b->size > 0 && b->pos < b->size) {
    avail = b->size - b->pos - 1;
  }
  size_t copy = (len < avail) ? len : avail;
  if (copy > 0) {
    memcpy(b->buf + b->pos, buf, copy);
  }
  b->pos += len;
  return (int)len;
}

static int print_num(out_fn out, void *ctx, unsigned long long val, int base, int neg) {
  char tmp[32];
  const char *digits = "0123456789abcdef";
  int i = 0;
  if (neg) {
    tmp[i++] = '-';
  }
  char buf[32];
  int n = 0;
  if (val == 0) {
    buf[n++] = '0';
  } else {
    while (val && n < (int)sizeof(buf)) {
      buf[n++] = digits[val % (unsigned)base];
      val /= (unsigned)base;
    }
  }
  for (int j = n - 1; j >= 0; j--) {
    tmp[i++] = buf[j];
  }
  return out(ctx, tmp, (size_t)i);
}

static int vformat(out_fn out, void *ctx, const char *fmt, va_list ap) {
  int total = 0;
  for (const char *p = fmt; *p; p++) {
    if (*p != '%') {
      out(ctx, p, 1);
      total++;
      continue;
    }
    p++;
    int long_flag = 0;
    if (*p == 'l') {
      long_flag = 1;
      p++;
    }
    switch (*p) {
      case 's': {
        const char *s = va_arg(ap, const char *);
        if (!s) {
          s = "(null)";
        }
        size_t len = strlen(s);
        out(ctx, s, len);
        total += (int)len;
        break;
      }
      case 'c': {
        char c = (char)va_arg(ap, int);
        out(ctx, &c, 1);
        total++;
        break;
      }
      case 'd':
      case 'i': {
        long long v = long_flag ? va_arg(ap, long) : va_arg(ap, int);
        unsigned long long uv = (v < 0) ? (unsigned long long)(-v) : (unsigned long long)v;
        int n = print_num(out, ctx, uv, 10, v < 0);
        total += n;
        break;
      }
      case 'u': {
        unsigned long long v = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        int n = print_num(out, ctx, v, 10, 0);
        total += n;
        break;
      }
      case 'x': {
        unsigned long long v = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        int n = print_num(out, ctx, v, 16, 0);
        total += n;
        break;
      }
      case 'p': {
        unsigned long long v = (unsigned long long)(uintptr_t)va_arg(ap, void *);
        out(ctx, "0x", 2);
        total += 2;
        int n = print_num(out, ctx, v, 16, 0);
        total += n;
        break;
      }
      case '%': {
        out(ctx, "%", 1);
        total++;
        break;
      }
      default: {
        out(ctx, "%", 1);
        out(ctx, p, 1);
        total += 2;
        break;
      }
    }
  }
  return total;
}

int vprintf(const char *fmt, va_list ap) {
  int fd = 1;
  return vformat(out_fd, &fd, fmt, ap);
}

int dprintf(int fd, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vformat(out_fd, &fd, fmt, ap);
  va_end(ap);
  return n;
}

int fprintf(FILE *stream, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int fd = stream ? stream->fd : 1;
  int n = vformat(out_fd, &fd, fmt, ap);
  va_end(ap);
  return n;
}

int vfprintf(FILE *stream, const char *fmt, va_list ap) {
  int fd = stream ? stream->fd : 1;
  return vformat(out_fd, &fd, fmt, ap);
}

int vsprintf(char *buf, const char *fmt, va_list ap) {
  return vsnprintf(buf, (size_t)-1, fmt, ap);
}

int sprintf(char *buf, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsprintf(buf, fmt, ap);
  va_end(ap);
  return n;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, size, fmt, ap);
  va_end(ap);
  return n;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
  struct buf_ctx ctx = {
      .buf = buf,
      .size = size,
      .pos = 0,
  };
  int n = vformat(out_buf, &ctx, fmt, ap);
  if (size > 0) {
    size_t term = (ctx.pos < size) ? ctx.pos : (size - 1);
    ctx.buf[term] = '\0';
  }
  return n;
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vprintf(fmt, ap);
  va_end(ap);
  return n;
}

int puts(const char *s) {
  size_t len = strlen(s);
  int fd = 1;
  out_fd(&fd, s, len);
  out_fd(&fd, "\n", 1);
  return (int)(len + 1);
}

int putchar(int c) {
  char ch = (char)c;
  int fd = 1;
  out_fd(&fd, &ch, 1);
  return c;
}
