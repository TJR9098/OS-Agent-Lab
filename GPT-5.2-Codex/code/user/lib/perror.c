#include <stdio.h>
#include <string.h>
#include <errno.h>

void perror(const char *s) {
  const char *msg = strerror(errno);
  if (s && *s) {
    dprintf(2, "%s: %s\n", s, msg);
  } else {
    dprintf(2, "%s\n", msg);
  }
}
