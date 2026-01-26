#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    dprintf(2, "mkdir: missing operand\n");
    return 1;
  }
  for (int i = 1; i < argc; i++) {
    int ret = mkdirat(AT_FDCWD, argv[i]);
    if (ret != 0) {
      dprintf(2, "mkdir: %s failed (%d)\n", argv[i], ret);
      return 1;
    }
  }
  return 0;
}