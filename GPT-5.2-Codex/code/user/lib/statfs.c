#include <sys/statfs.h>
#include <errno.h>

int statfs(const char *path, struct statfs *buf) {
  (void)path;
  (void)buf;
  errno = ENOSYS;
  return -1;
}

int fstatfs(int fd, struct statfs *buf) {
  (void)fd;
  (void)buf;
  errno = ENOSYS;
  return -1;
}
