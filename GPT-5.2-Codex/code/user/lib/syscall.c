#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define SYS_DUP 23
#define SYS_DUP3 24
#define SYS_IOCTL 29
#define SYS_GETCWD 17
#define SYS_CHDIR 49
#define SYS_MKDIRAT 34
#define SYS_UNLINKAT 35
#define SYS_OPENAT 56
#define SYS_CLOSE 57
#define SYS_GETDENTS64 61
#define SYS_LSEEK 62
#define SYS_READ 63
#define SYS_WRITE 64
#define SYS_FSTATAT 79
#define SYS_FSTAT 80
#define SYS_EXIT 93
#define SYS_BRK 214
#define SYS_CLONE 220
#define SYS_EXECVE 221
#define SYS_WAIT4 260

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

static long wrap_ret(long ret) {
  if (ret < 0) {
    errno = (int)(-ret);
    return -1;
  }
  return ret;
}

ssize_t write(int fd, const void *buf, size_t len) {
  return (ssize_t)wrap_ret(syscall6(SYS_WRITE, fd, (long)buf, (long)len, 0, 0, 0));
}

ssize_t read(int fd, void *buf, size_t len) {
  return (ssize_t)wrap_ret(syscall6(SYS_READ, fd, (long)buf, (long)len, 0, 0, 0));
}

int dup(int fd) {
  return (int)wrap_ret(syscall6(SYS_DUP, fd, 0, 0, 0, 0, 0));
}

int dup2(int oldfd, int newfd) {
  return (int)wrap_ret(syscall6(SYS_DUP3, oldfd, newfd, 0, 0, 0, 0));
}

int ioctl(int fd, unsigned long req, void *arg) {
  return (int)wrap_ret(syscall6(SYS_IOCTL, fd, (long)req, (long)arg, 0, 0, 0));
}

int close(int fd) {
  return (int)wrap_ret(syscall6(SYS_CLOSE, fd, 0, 0, 0, 0, 0));
}

int openat(int dirfd, const char *path, int flags, ...) {
  return (int)wrap_ret(syscall6(SYS_OPENAT, dirfd, (long)path, flags, 0, 0, 0));
}

int open(const char *path, int flags, ...) {
  return openat(AT_FDCWD, path, flags);
}

off_t lseek(int fd, off_t offset, int whence) {
  return (off_t)wrap_ret(syscall6(SYS_LSEEK, fd, (long)offset, whence, 0, 0, 0));
}

int getdents64(int fd, void *dirp, size_t count) {
  return (int)wrap_ret(syscall6(SYS_GETDENTS64, fd, (long)dirp, (long)count, 0, 0, 0));
}

int mkdirat(int dirfd, const char *path) {
  return (int)wrap_ret(syscall6(SYS_MKDIRAT, dirfd, (long)path, 0, 0, 0, 0));
}

int unlinkat(int dirfd, const char *path) {
  return (int)wrap_ret(syscall6(SYS_UNLINKAT, dirfd, (long)path, 0, 0, 0, 0));
}

int fstat(int fd, struct stat *st) {
  return (int)wrap_ret(syscall6(SYS_FSTAT, fd, (long)st, 0, 0, 0, 0));
}

int fstatat(int dirfd, const char *path, struct stat *st) {
  return (int)wrap_ret(syscall6(SYS_FSTATAT, dirfd, (long)path, (long)st, 0, 0, 0));
}

int chdir(const char *path) {
  return (int)wrap_ret(syscall6(SYS_CHDIR, (long)path, 0, 0, 0, 0, 0));
}

char *getcwd(char *buf, size_t size) {
  long ret = syscall6(SYS_GETCWD, (long)buf, (long)size, 0, 0, 0, 0);
  if (ret < 0) {
    errno = (int)(-ret);
    return NULL;
  }
  return buf;
}

pid_t fork(void) {
  return (pid_t)wrap_ret(syscall6(SYS_CLONE, 0, 0, 0, 0, 0, 0));
}

int execve(const char *path, char *const argv[], char *const envp[]) {
  (void)envp;
  return (int)wrap_ret(syscall6(SYS_EXECVE, (long)path, (long)argv, 0, 0, 0, 0));
}

pid_t waitpid(pid_t pid, int *status, int options) {
  (void)options;
  return (pid_t)wrap_ret(syscall6(SYS_WAIT4, pid, (long)status, 0, 0, 0, 0));
}

pid_t wait(int *status) {
  return waitpid(-1, status, 0);
}

void _exit(int status) {
  syscall6(SYS_EXIT, status, 0, 0, 0, 0, 0);
  for (;;) {
  }
}

void exit(int status) {
  _exit(status);
}

static void *sys_brk(void *addr) {
  return (void *)syscall6(SYS_BRK, (long)addr, 0, 0, 0, 0, 0);
}

int brk(void *addr) {
  void *ret = sys_brk(addr);
  return (ret == addr) ? 0 : -1;
}

void *sbrk(intptr_t inc) {
  static void *cur;
  if (!cur) {
    cur = sys_brk(0);
  }
  void *old = cur;
  void *next = (void *)((char *)cur + inc);
  void *ret = sys_brk(next);
  if (ret != next) {
    return (void *)-1;
  }
  cur = next;
  return old;
}
