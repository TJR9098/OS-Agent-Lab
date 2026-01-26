#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static int cat_fd(int fd) {
  char buf[256];
  for (;;) {
    int n = read(fd, buf, sizeof(buf));
    if (n < 0) {
      return n;
    }
    if (n == 0) {
      return 0;
    }
    int off = 0;
    while (off < n) {
      int w = write(1, buf + off, (size_t)(n - off));
      if (w < 0) {
        return w;
      }
      off += w;
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    int ret = cat_fd(0);
    if (ret < 0) {
      dprintf(2, "cat: stdin failed (%d)\n", ret);
      return 1;
    }
    return 0;
  }

  for (int i = 1; i < argc; i++) {
    int fd = openat(AT_FDCWD, argv[i], O_RDONLY);
    if (fd < 0) {
      dprintf(2, "cat: %s failed (%d)\n", argv[i], fd);
      return 1;
    }
    int ret = cat_fd(fd);
    close(fd);
    if (ret < 0) {
      dprintf(2, "cat: %s read failed (%d)\n", argv[i], ret);
      return 1;
    }
  }
  return 0;
}
