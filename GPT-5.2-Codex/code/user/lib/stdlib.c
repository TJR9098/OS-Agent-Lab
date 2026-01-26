#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

char **environ;

int atoi(const char *s) {
  int neg = 0;
  int val = 0;
  if (!s) {
    return 0;
  }
  if (*s == '-') {
    neg = 1;
    s++;
  }
  while (*s >= '0' && *s <= '9') {
    val = val * 10 + (*s - '0');
    s++;
  }
  return neg ? -val : val;
}

long atol(const char *s) {
  return strtol(s, NULL, 10);
}

void *calloc(size_t nmemb, size_t size) {
  size_t total = nmemb * size;
  void *p = malloc(total);
  if (!p) {
    return NULL;
  }
  memset(p, 0, total);
  return p;
}

static int digit_val(int c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

long strtol(const char *s, char **end, int base) {
  const char *p = s;
  while (isspace((unsigned char)*p)) {
    p++;
  }
  int neg = 0;
  if (*p == '-' || *p == '+') {
    neg = (*p == '-');
    p++;
  }
  if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
    base = 16;
    p += 2;
  } else if (base == 0 && p[0] == '0') {
    base = 8;
    p++;
  } else if (base == 0) {
    base = 10;
  }
  long val = 0;
  int d;
  while ((d = digit_val((unsigned char)*p)) >= 0 && d < base) {
    val = val * base + d;
    p++;
  }
  if (end) {
    *end = (char *)p;
  }
  return neg ? -val : val;
}

unsigned long strtoul(const char *s, char **end, int base) {
  const char *p = s;
  while (isspace((unsigned char)*p)) {
    p++;
  }
  if (*p == '+') {
    p++;
  }
  if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
    base = 16;
    p += 2;
  } else if (base == 0 && p[0] == '0') {
    base = 8;
    p++;
  } else if (base == 0) {
    base = 10;
  }
  unsigned long val = 0;
  int d;
  while ((d = digit_val((unsigned char)*p)) >= 0 && d < base) {
    val = val * (unsigned long)base + (unsigned long)d;
    p++;
  }
  if (end) {
    *end = (char *)p;
  }
  return val;
}

long long strtoll(const char *s, char **end, int base) {
  return (long long)strtol(s, end, base);
}

unsigned long long strtoull(const char *s, char **end, int base) {
  return (unsigned long long)strtoul(s, end, base);
}

static void swap_bytes(unsigned char *a, unsigned char *b, size_t size) {
  for (size_t i = 0; i < size; i++) {
    unsigned char tmp = a[i];
    a[i] = b[i];
    b[i] = tmp;
  }
}

static void qsort_impl(unsigned char *base, size_t nmemb, size_t size,
                       int (*compar)(const void *, const void *)) {
  if (nmemb < 2 || !compar) {
    return;
  }
  size_t pivot = nmemb / 2;
  unsigned char *pivot_elem = base + pivot * size;
  size_t i = 0;
  size_t j = nmemb - 1;
  while (i <= j) {
    while (compar(base + i * size, pivot_elem) < 0) {
      i++;
    }
    while (compar(base + j * size, pivot_elem) > 0) {
      if (j == 0) {
        break;
      }
      j--;
    }
    if (i <= j) {
      swap_bytes(base + i * size, base + j * size, size);
      i++;
      if (j == 0) {
        break;
      }
      j--;
    }
  }
  if (j + 1 > 1) {
    qsort_impl(base, j + 1, size, compar);
  }
  if (i < nmemb) {
    qsort_impl(base + i * size, nmemb - i, size, compar);
  }
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
  if (!base || size == 0) {
    return;
  }
  qsort_impl((unsigned char *)base, nmemb, size, compar);
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *)) {
  if (!key || !base || size == 0 || !compar) {
    return NULL;
  }
  size_t lo = 0;
  size_t hi = nmemb;
  const unsigned char *b = (const unsigned char *)base;
  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    const void *elem = b + mid * size;
    int cmp = compar(key, elem);
    if (cmp == 0) {
      return (void *)elem;
    }
    if (cmp < 0) {
      hi = mid;
    } else {
      lo = mid + 1;
    }
  }
  return NULL;
}

char *getenv(const char *name) {
  (void)name;
  return NULL;
}

int putenv(char *string) {
  (void)string;
  return 0;
}

int setenv(const char *name, const char *value, int overwrite) {
  (void)name;
  (void)value;
  (void)overwrite;
  return 0;
}

int unsetenv(const char *name) {
  (void)name;
  return 0;
}

int clearenv(void) {
  return 0;
}

static unsigned long rand_state = 1;

void srand(unsigned int seed) {
  rand_state = seed ? seed : 1;
}

int rand(void) {
  rand_state = rand_state * 1103515245u + 12345u;
  return (int)((rand_state >> 16) & 0x7fff);
}

int mkstemp(char *template) {
  if (!template) {
    errno = EINVAL;
    return -1;
  }
  size_t len = strlen(template);
  if (len < 6 || strcmp(template + len - 6, "XXXXXX") != 0) {
    errno = EINVAL;
    return -1;
  }
  static const char letters[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  char *suffix = template + len - 6;
  for (int attempt = 0; attempt < 128; attempt++) {
    unsigned long v = (unsigned long)rand();
    for (int i = 0; i < 6; i++) {
      suffix[i] = letters[(v + (unsigned long)i * 31u) % (sizeof(letters) - 1)];
    }
    int fd = open(template, O_RDWR | O_CREAT | O_EXCL);
    if (fd >= 0) {
      return fd;
    }
    if (errno != EEXIST) {
      return -1;
    }
  }
  errno = EEXIST;
  return -1;
}

char *realpath(const char *path, char *resolved_path) {
  if (!path || !resolved_path) {
    errno = EINVAL;
    return NULL;
  }
  size_t len = strlen(path);
  if (len >= 4096) {
    errno = ERANGE;
    return NULL;
  }
  memcpy(resolved_path, path, len + 1);
  return resolved_path;
}

int system(const char *command) {
  (void)command;
  errno = ENOSYS;
  return -1;
}

int abs(int x) {
  return (x < 0) ? -x : x;
}

long labs(long x) {
  return (x < 0) ? -x : x;
}

void abort(void) {
  exit(1);
}
