#include <errno.h>
#include <poll.h>
#include <stdint.h>

#define SYS_POLL 7

static long syscall6(long num, long a0, long a1, long a2, long a3, long a4, long a5) {
  register long x10 __asm__("a0") = a0;
  register long x11 __asm__("a1") = a1;
  register long x12 __asm__("a2") = a2;
  register long x13 __asm__("a3") = a3;
  register long x14 __asm__("a4") = a4;
  register long x15 __asm__("a5") = a5;
  register long x17 __asm__("a7") = num;
  __asm__ volatile("ecall"
                   : "+r"(x10)
                   : "r"(x11), "r"(x12), "r"(x13), "r"(x14), "r"(x15), "r"(x17)
                   : "memory", "t0", "t1");
  return x10;
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  long ret = syscall6(SYS_POLL, (long)fds, (long)nfds, (long)timeout, 0, 0, 0);
  if (ret < 0) {
    errno = (int)(-ret);
    return -1;
  }
  return (int)ret;
}
