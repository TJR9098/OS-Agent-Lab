#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void __assert_fail(const char *expr, const char *file, int line, const char *func) {
  if (func) {
    dprintf(2, "assertion failed: %s, function %s, file %s, line %d\n", expr, func, file, line);
  } else {
    dprintf(2, "assertion failed: %s, file %s, line %d\n", expr, file, line);
  }
  abort();
}
