#include <termios.h>
#include <sys/ioctl.h>
#include <stddef.h>

int tcgetattr(int fd, struct termios *term) {
  return ioctl(fd, TCGETS, term);
}

int tcsetattr(int fd, int opt, const struct termios *term) {
  (void)opt;
  return ioctl(fd, TCSETS, (void *)term);
}

int tcflush(int fd, int queue_selector) {
  (void)queue_selector;
  return ioctl(fd, TCSETS, NULL);
}
