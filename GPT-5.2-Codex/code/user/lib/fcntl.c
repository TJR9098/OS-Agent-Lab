#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

int fcntl(int fd, int cmd, ...) {
  va_list ap;
  va_start(ap, cmd);
  int arg = 0;
  if (cmd == F_DUPFD || cmd == F_DUPFD_CLOEXEC || cmd == F_SETFL || cmd == F_SETFD) {
    arg = va_arg(ap, int);
  }
  va_end(ap);

  switch (cmd) {
    case F_DUPFD:
#if F_DUPFD_CLOEXEC != F_DUPFD
    case F_DUPFD_CLOEXEC:
#endif
    {
      if (arg < 0) {
        errno = EINVAL;
        return -1;
      }
      int tmp[32];
      int count = 0;
      for (;;) {
        int newfd = dup(fd);
        if (newfd < 0) {
          for (int i = 0; i < count; i++) {
            close(tmp[i]);
          }
          return -1;
        }
        if (newfd >= arg) {
          for (int i = 0; i < count; i++) {
            close(tmp[i]);
          }
          return newfd;
        }
        if (count >= (int)(sizeof(tmp) / sizeof(tmp[0]))) {
          close(newfd);
          for (int i = 0; i < count; i++) {
            close(tmp[i]);
          }
          errno = EMFILE;
          return -1;
        }
        tmp[count++] = newfd;
      }
    }
    case F_GETFL:
      return 0;
    case F_SETFL:
      (void)arg;
      return 0;
    case F_SETFD:
      (void)arg;
      return 0;
    default:
      errno = ENOSYS;
      return -1;
  }
}
