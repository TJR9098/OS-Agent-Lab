#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int check_pattern(const char *buf, size_t len, unsigned char base) {
  for (size_t i = 0; i < len; i++) {
    if ((unsigned char)buf[i] != (unsigned char)(base + (unsigned char)i)) {
      return -1;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  size_t a_len = 32;
  size_t b_len = 64;
  size_t c_len = 128;

  char *a = (char *)malloc(a_len);
  char *b = (char *)malloc(b_len);
  char *c = (char *)malloc(c_len);
  if (!a || !b || !c) {
    dprintf(2, "memtest: malloc failed\n");
    return 1;
  }

  for (size_t i = 0; i < a_len; i++) {
    a[i] = (char)(0x10 + (unsigned char)i);
  }
  for (size_t i = 0; i < b_len; i++) {
    b[i] = (char)(0x20 + (unsigned char)i);
  }
  memset(c, 0x5a, c_len);

  size_t b_new = 160;
  char *b2 = (char *)realloc(b, b_new);
  if (!b2) {
    dprintf(2, "memtest: realloc failed\n");
    free(a);
    free(b);
    free(c);
    return 1;
  }
  b = b2;

  if (check_pattern(a, a_len, 0x10) != 0) {
    dprintf(2, "memtest: pattern A mismatch\n");
    return 1;
  }
  if (check_pattern(b, b_len, 0x20) != 0) {
    dprintf(2, "memtest: pattern B mismatch after realloc\n");
    return 1;
  }

  free(a);
  free(b);
  free(c);

  char *d = (char *)malloc(48);
  if (!d) {
    dprintf(2, "memtest: malloc after free failed\n");
    return 1;
  }
  free(d);

  printf("memtest: ok\n");
  return 0;
}
