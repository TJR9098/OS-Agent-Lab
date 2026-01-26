#include "string.h"

void *memset(void *dst, int c, size_t n) {
  unsigned char *p = (unsigned char *)dst;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  while (n--) {
    *d++ = *s++;
  }
  return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *pa = (const unsigned char *)a;
  const unsigned char *pb = (const unsigned char *)b;
  while (n--) {
    if (*pa != *pb) {
      return (*pa < *pb) ? -1 : 1;
    }
    pa++;
    pb++;
  }
  return 0;
}

size_t strlen(const char *s) {
  size_t n = 0;
  while (s[n]) {
    n++;
  }
  return n;
}

int strcmp(const char *a, const char *b) {
  while (*a && *b) {
    if (*a != *b) {
      return (*a < *b) ? -1 : 1;
    }
    a++;
    b++;
  }
  if (*a == *b) {
    return 0;
  }
  return (*a < *b) ? -1 : 1;
}