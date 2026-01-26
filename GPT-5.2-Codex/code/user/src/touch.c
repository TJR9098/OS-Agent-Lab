#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int parent_exists(const char *path) {
  const char *slash = strrchr(path, '/');
  char buf[256];
  const char *parent = NULL;

  if (!slash) {
    parent = ".";
  } else if (slash == path) {
    parent = "/";
  } else {
    size_t len = (size_t)(slash - path);
    if (len >= sizeof(buf)) {
      return -1;
    }
    memcpy(buf, path, len);
    buf[len] = '\0';
    parent = buf;
  }

  struct stat st;
  if (fstatat(AT_FDCWD, parent, &st) != 0) {
    return -1;
  }
  if (!S_ISDIR(st.st_mode)) {
    return -1;
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    dprintf(2, "touch: missing operand\n");
    return 1;
  }
  for (int i = 1; i < argc; i++) {
    struct stat st;
    if (fstatat(AT_FDCWD, argv[i], &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        dprintf(2, "touch: %s is a directory\n", argv[i]);
        return 1;
      }
      int fd = openat(AT_FDCWD, argv[i], O_WRONLY);
      if (fd < 0) {
        dprintf(2, "touch: %s failed (%d)\n", argv[i], fd);
        return 1;
      }
      close(fd);
      continue;
    }

    if (parent_exists(argv[i]) != 0) {
      dprintf(2, "touch: parent missing for %s\n", argv[i]);
      return 1;
    }

    int fd = openat(AT_FDCWD, argv[i], O_WRONLY | O_CREAT);
    if (fd < 0) {
      dprintf(2, "touch: %s failed (%d)\n", argv[i], fd);
      return 1;
    }
    close(fd);
  }
  return 0;
}