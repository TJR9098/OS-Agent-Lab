#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    dprintf(2, "rm: missing operand\n");
    return 1;
  }
  int status = 0;
  for (int i = 1; i < argc; i++) {
    if (unlinkat(AT_FDCWD, argv[i]) < 0) {
      dprintf(2, "rm: %s failed (%d)\n", argv[i], errno);
      status = 1;
    }
  }
  return status;
}

