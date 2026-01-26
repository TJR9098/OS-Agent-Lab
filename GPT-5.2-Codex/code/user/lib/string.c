#include <string.h>
#include <stdlib.h>
#include <ctype.h>

size_t strlen(const char *s) {
  size_t n = 0;
  while (s && s[n]) {
    n++;
  }
  return n;
}

size_t strnlen(const char *s, size_t n) {
  size_t i = 0;
  if (!s) {
    return 0;
  }
  for (; i < n && s[i]; i++) {
  }
  return i;
}

int strcmp(const char *a, const char *b) {
  while (*a && *b && *a == *b) {
    a++;
    b++;
  }
  return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
  for (size_t i = 0; i < n; i++) {
    unsigned char ca = (unsigned char)a[i];
    unsigned char cb = (unsigned char)b[i];
    if (ca != cb || ca == 0 || cb == 0) {
      return ca - cb;
    }
  }
  return 0;
}

char *strcpy(char *dst, const char *src) {
  char *d = dst;
  while ((*d++ = *src++) != '\0') {
  }
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i]; i++) {
    dst[i] = src[i];
  }
  for (; i < n; i++) {
    dst[i] = '\0';
  }
  return dst;
}

char *stpcpy(char *dst, const char *src) {
  char *d = dst;
  while ((*d = *src) != '\0') {
    d++;
    src++;
  }
  return d;
}

char *stpncpy(char *dst, const char *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i]; i++) {
    dst[i] = src[i];
  }
  if (i < n) {
    dst[i] = '\0';
    for (size_t j = i + 1; j < n; j++) {
      dst[j] = '\0';
    }
  }
  return dst + i;
}

char *strcat(char *dst, const char *src) {
  char *d = dst;
  while (*d) {
    d++;
  }
  while ((*d++ = *src++) != '\0') {
  }
  return dst;
}

char *strncat(char *dst, const char *src, size_t n) {
  char *d = dst;
  while (*d) {
    d++;
  }
  size_t i = 0;
  while (i < n && src[i]) {
    d[i] = src[i];
    i++;
  }
  d[i] = '\0';
  return dst;
}

char *strchr(const char *s, int c) {
  while (*s) {
    if (*s == (char)c) {
      return (char *)s;
    }
    s++;
  }
  if (c == 0) {
    return (char *)s;
  }
  return NULL;
}

char *strchrnul(const char *s, int c) {
  while (*s) {
    if (*s == (char)c) {
      return (char *)s;
    }
    s++;
  }
  return (char *)s;
}

char *strrchr(const char *s, int c) {
  const char *last = NULL;
  while (*s) {
    if (*s == (char)c) {
      last = s;
    }
    s++;
  }
  if (c == 0) {
    return (char *)s;
  }
  return (char *)last;
}

char *index(const char *s, int c) {
  return strchr(s, c);
}

char *rindex(const char *s, int c) {
  return strrchr(s, c);
}

char *strstr(const char *haystack, const char *needle) {
  if (!*needle) {
    return (char *)haystack;
  }
  for (const char *p = haystack; *p; p++) {
    const char *h = p;
    const char *n = needle;
    while (*h && *n && *h == *n) {
      h++;
      n++;
    }
    if (*n == '\0') {
      return (char *)p;
    }
  }
  return NULL;
}

static int in_set(char c, const char *set) {
  if (!set) {
    return 0;
  }
  for (const char *p = set; *p; p++) {
    if (*p == c) {
      return 1;
    }
  }
  return 0;
}

size_t strspn(const char *s, const char *accept) {
  size_t n = 0;
  while (s && s[n] && in_set(s[n], accept)) {
    n++;
  }
  return n;
}

size_t strcspn(const char *s, const char *reject) {
  size_t n = 0;
  while (s && s[n] && !in_set(s[n], reject)) {
    n++;
  }
  return n;
}

char *strpbrk(const char *s, const char *accept) {
  if (!s || !accept) {
    return NULL;
  }
  for (; *s; s++) {
    if (in_set(*s, accept)) {
      return (char *)s;
    }
  }
  return NULL;
}

char *strtok_r(char *s, const char *delim, char **save) {
  if (!save) {
    return NULL;
  }
  if (!s) {
    s = *save;
  }
  if (!s) {
    return NULL;
  }
  while (*s && in_set(*s, delim)) {
    s++;
  }
  if (*s == '\0') {
    *save = NULL;
    return NULL;
  }
  char *token = s;
  while (*s && !in_set(*s, delim)) {
    s++;
  }
  if (*s) {
    *s = '\0';
    s++;
  }
  *save = s;
  return token;
}

char *strtok(char *s, const char *delim) {
  static char *save;
  return strtok_r(s, delim, &save);
}

char *strdup(const char *s) {
  if (!s) {
    return NULL;
  }
  size_t len = strlen(s) + 1;
  char *p = (char *)malloc(len);
  if (!p) {
    return NULL;
  }
  memcpy(p, s, len);
  return p;
}

char *strndup(const char *s, size_t n) {
  if (!s) {
    return NULL;
  }
  size_t len = 0;
  while (len < n && s[len]) {
    len++;
  }
  char *p = (char *)malloc(len + 1);
  if (!p) {
    return NULL;
  }
  memcpy(p, s, len);
  p[len] = '\0';
  return p;
}

int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *pa = (const unsigned char *)a;
  const unsigned char *pb = (const unsigned char *)b;
  for (size_t i = 0; i < n; i++) {
    if (pa[i] != pb[i]) {
      return (int)pa[i] - (int)pb[i];
    }
  }
  return 0;
}

void *memcpy(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }
  return dst;
}

void *mempcpy(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }
  return d + n;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  if (d == s || n == 0) {
    return dst;
  }
  if (d < s) {
    for (size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  } else {
    for (size_t i = n; i > 0; i--) {
      d[i - 1] = s[i - 1];
    }
  }
  return dst;
}

void *memset(void *dst, int c, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  for (size_t i = 0; i < n; i++) {
    d[i] = (unsigned char)c;
  }
  return dst;
}

void *memchr(const void *s, int c, size_t n) {
  const unsigned char *p = (const unsigned char *)s;
  for (size_t i = 0; i < n; i++) {
    if (p[i] == (unsigned char)c) {
      return (void *)(p + i);
    }
  }
  return NULL;
}

void *memrchr(const void *s, int c, size_t n) {
  const unsigned char *p = (const unsigned char *)s;
  for (size_t i = n; i > 0; i--) {
    if (p[i - 1] == (unsigned char)c) {
      return (void *)(p + i - 1);
    }
  }
  return NULL;
}

int strcasecmp(const char *a, const char *b) {
  while (*a && *b) {
    int ca = tolower((unsigned char)*a);
    int cb = tolower((unsigned char)*b);
    if (ca != cb) {
      return ca - cb;
    }
    a++;
    b++;
  }
  return (unsigned char)*a - (unsigned char)*b;
}

int strncasecmp(const char *a, const char *b, size_t n) {
  for (size_t i = 0; i < n; i++) {
    unsigned char ca = (unsigned char)a[i];
    unsigned char cb = (unsigned char)b[i];
    if (ca == 0 || cb == 0) {
      return ca - cb;
    }
    ca = (unsigned char)tolower(ca);
    cb = (unsigned char)tolower(cb);
    if (ca != cb) {
      return ca - cb;
    }
  }
  return 0;
}

int strcoll(const char *a, const char *b) {
  return strcmp(a, b);
}

size_t strxfrm(char *dst, const char *src, size_t n) {
  size_t len = strlen(src);
  if (dst && n > 0) {
    size_t copy = len < (n - 1) ? len : (n - 1);
    memcpy(dst, src, copy);
    dst[copy] = '\0';
  }
  return len;
}

char *strsignal(int sig) {
  (void)sig;
  return "signal";
}
