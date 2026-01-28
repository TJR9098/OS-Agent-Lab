#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>

pid_t getpgrp(void) {
  return 0;
}

int setuid(uid_t uid) {
  (void)uid;
  return 0;
}

int seteuid(uid_t uid) {
  (void)uid;
  return 0;
}

int setgid(gid_t gid) {
  (void)gid;
  return 0;
}

int setegid(gid_t gid) {
  (void)gid;
  return 0;
}

unsigned int sleep(unsigned int seconds) {
  (void)seconds;
  return 0;
}

int usleep(unsigned int usec) {
  (void)usec;
  return 0;
}

unsigned int alarm(unsigned int seconds) {
  (void)seconds;
  return 0;
}

int getgroups(int size, gid_t list[]) {
  if (size > 0 && list) {
    list[0] = 0;
    return 1;
  }
  return 0;
}

int unlink(const char *path) {
  return unlinkat(AT_FDCWD, path);
}

int rmdir(const char *path) {
  return unlinkat(AT_FDCWD, path);
}

int lstat(const char *path, struct stat *st) {
  return fstatat(AT_FDCWD, path, st);
}

int mkdir(const char *path, mode_t mode) {
  (void)mode;
  return mkdirat(AT_FDCWD, path);
}

int mknod(const char *path, mode_t mode, dev_t dev) {
  (void)path;
  (void)mode;
  (void)dev;
  errno = ENOSYS;
  return -1;
}

int chown(const char *path, uid_t owner, gid_t group) {
  (void)path;
  (void)owner;
  (void)group;
  return 0;
}

int chmod(const char *path, mode_t mode) {
  (void)path;
  (void)mode;
  return 0;
}

int lchown(const char *path, uid_t owner, gid_t group) {
  (void)path;
  (void)owner;
  (void)group;
  return 0;
}

mode_t umask(mode_t mask) {
  (void)mask;
  return 0;
}

int access(const char *path, int mode) {
  (void)mode;
  struct stat st;
  if (stat(path, &st) < 0) {
    return -1;
  }
  return 0;
}

int getpagesize(void) {
  return 4096;
}

long sysconf(int name) {
  if (name == _SC_CLK_TCK) {
    return 100;
  }
  return -1;
}

int utimes(const char *path, const struct timeval times[2]) {
  (void)path;
  (void)times;
  return 0;
}

int link(const char *oldpath, const char *newpath) {
  (void)oldpath;
  (void)newpath;
  errno = ENOSYS;
  return -1;
}

int symlink(const char *target, const char *linkpath) {
  (void)target;
  (void)linkpath;
  errno = ENOSYS;
  return -1;
}

int fchdir(int fd) {
  (void)fd;
  errno = ENOSYS;
  return -1;
}

int chroot(const char *path) {
  (void)path;
  errno = ENOSYS;
  return -1;
}

size_t confstr(int name, char *buf, size_t len) {
  (void)name;
  if (buf && len > 0) {
    buf[0] = '\0';
  }
  return 0;
}

char *ctermid(char *s) {
  static char buf[] = "/dev/tty";
  if (!s) {
    return buf;
  }
  s[0] = '\0';
  return s;
}

int fchown(int fd, uid_t owner, gid_t group) {
  (void)fd;
  (void)owner;
  (void)group;
  errno = ENOSYS;
  return -1;
}

int fdatasync(int fd) {
  (void)fd;
  errno = ENOSYS;
  return -1;
}

long fpathconf(int fd, int name) {
  (void)fd;
  (void)name;
  errno = ENOSYS;
  return -1;
}

int fsync(int fd) {
  (void)fd;
  return 0;
}

int getdtablesize(void) {
  return 32;
}

long gethostid(void) {
  return 0;
}

char *getlogin(void) {
  return "root";
}

int getlogin_r(char *buf, size_t bufsize) {
  const char *name = "root";
  size_t len = strlen(name);
  if (!buf || bufsize == 0 || len + 1 > bufsize) {
    errno = ERANGE;
    return -1;
  }
  memcpy(buf, name, len + 1);
  return 0;
}

char *getpass(const char *prompt) {
  static char buf[1] = {0};
  (void)prompt;
  return buf;
}

int pause(void) {
  errno = EINTR;
  return -1;
}

long pathconf(const char *path, int name) {
  (void)path;
  (void)name;
  errno = ENOSYS;
  return -1;
}

int lockf(int fd, int cmd, off_t len) {
  (void)fd;
  (void)cmd;
  (void)len;
  return 0;
}

int nice(int inc) {
  (void)inc;
  return 0;
}

int setpgid(pid_t pid, pid_t pgid) {
  (void)pid;
  (void)pgid;
  return 0;
}

pid_t setpgrp(void) {
  return 0;
}

int setregid(gid_t rgid, gid_t egid) {
  (void)rgid;
  (void)egid;
  return 0;
}

int setreuid(uid_t ruid, uid_t euid) {
  (void)ruid;
  (void)euid;
  return 0;
}

void sync(void) {
}

pid_t tcgetpgrp(int fd) {
  (void)fd;
  errno = ENOSYS;
  return -1;
}

int tcsetpgrp(int fd, pid_t pgrp) {
  (void)fd;
  (void)pgrp;
  errno = ENOSYS;
  return -1;
}

int truncate(const char *path, off_t length) {
  (void)path;
  (void)length;
  errno = ENOSYS;
  return -1;
}

useconds_t ualarm(useconds_t usecs, useconds_t interval) {
  (void)interval;
  return usecs;
}

int execvp(const char *file, char *const argv[]) {
  return execve(file, argv, NULL);
}

int execv(const char *path, char *const argv[]) {
  return execve(path, argv, NULL);
}

pid_t vfork(void) {
  return fork();
}

pid_t setsid(void) {
  return 0;
}

ssize_t readlink(const char *path, char *buf, size_t bufsiz) {
  (void)path;
  (void)buf;
  (void)bufsiz;
  errno = ENOSYS;
  return -1;
}

int ttyname_r(int fd, char *buf, size_t buflen) {
  (void)fd;
  if (buf && buflen > 0) {
    buf[0] = '\0';
  }
  errno = ENOSYS;
  return -1;
}

char *ttyname(int fd) {
  static char buf[] = "/dev/tty";
  (void)fd;
  return buf;
}

int pipe(int pipefd[2]) {
  (void)pipefd;
  errno = ENOSYS;
  return -1;
}
