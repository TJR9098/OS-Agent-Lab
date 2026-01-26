#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int ensure_dir(const char *path) {
  struct stat st;
  if (fstatat(AT_FDCWD, path, &st) == 0) {
    if (S_ISDIR(st.st_mode)) {
      return 0;
    }
    return -1;
  }
  int ret = mkdirat(AT_FDCWD, path);
  return ret;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  const char *dir = "/test";
  const char *path = "/test/hello_world.c";
  const char *content = "#include <stdio.h>\nint main(){ printf(\"Hello, world!\\n\"); return 0; }\n";

  if (ensure_dir(dir) != 0) {
    dprintf(2, "mkhello: create %s failed\n", dir);
    return 1;
  }

  int fd = openat(AT_FDCWD, path, O_WRONLY | O_CREAT | O_TRUNC);
  if (fd < 0) {
    dprintf(2, "mkhello: open %s failed (%d)\n", path, fd);
    return 1;
  }
  size_t len = strlen(content);
  if (write(fd, content, len) != (int)len) {
    dprintf(2, "mkhello: write failed\n");
    close(fd);
    return 1;
  }
  close(fd);
  printf("mkhello: wrote %s\n", path);
  return 0;
}