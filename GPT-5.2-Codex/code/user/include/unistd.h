#pragma once

#include <sys/types.h>
#include <stddef.h>
#include <sys/stat.h>

#define AT_FDCWD (-100)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

ssize_t read(int fd, void *buf, size_t len);
ssize_t write(int fd, const void *buf, size_t len);
int close(int fd);
int openat(int dirfd, const char *path, int flags, ...);
int open(const char *path, int flags, ...);
off_t lseek(int fd, off_t offset, int whence);
int dup(int fd);
int dup2(int oldfd, int newfd);
int isatty(int fd);
pid_t getpid(void);
pid_t getppid(void);
pid_t getpgrp(void);
uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);
int setuid(uid_t uid);
int seteuid(uid_t uid);
int setgid(gid_t gid);
int setegid(gid_t gid);
unsigned int sleep(unsigned int seconds);
int usleep(unsigned int usec);
unsigned int alarm(unsigned int seconds);
int getgroups(int size, gid_t list[]);
int getdents64(int fd, void *dirp, size_t count);
int mkdirat(int dirfd, const char *path);
int unlinkat(int dirfd, const char *path);
int fstat(int fd, struct stat *st);
int fstatat(int dirfd, const char *path, struct stat *st);
int unlink(const char *path);
int rmdir(const char *path);
int lstat(const char *path, struct stat *st);
int mkdir(const char *path, mode_t mode);
int mknod(const char *path, mode_t mode, dev_t dev);
int chown(const char *path, uid_t owner, gid_t group);
int chmod(const char *path, mode_t mode);
int ftruncate(int fd, off_t length);
int lchown(const char *path, uid_t owner, gid_t group);
mode_t umask(mode_t mask);
int access(const char *path, int mode);
int getpagesize(void);
long sysconf(int name);
#define _SC_CLK_TCK 2
int link(const char *oldpath, const char *newpath);
int symlink(const char *target, const char *linkpath);
int chdir(const char *path);
int fchdir(int fd);
int chroot(const char *path);
char *getcwd(char *buf, size_t size);
ssize_t readlink(const char *path, char *buf, size_t bufsiz);
int ttyname_r(int fd, char *buf, size_t buflen);
char *ttyname(int fd);
size_t confstr(int name, char *buf, size_t len);
char *ctermid(char *s);
int fchown(int fd, uid_t owner, gid_t group);
int fdatasync(int fd);
long fpathconf(int fd, int name);
int fsync(int fd);
int getdtablesize(void);
long gethostid(void);
char *getlogin(void);
int getlogin_r(char *buf, size_t bufsize);
char *getpass(const char *prompt);
int pause(void);
long pathconf(const char *path, int name);
int lockf(int fd, int cmd, off_t len);
int nice(int inc);
int setpgid(pid_t pid, pid_t pgid);
pid_t setpgrp(void);
int setregid(gid_t rgid, gid_t egid);
int setreuid(uid_t ruid, uid_t euid);
void sync(void);
pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgrp);
int truncate(const char *path, off_t length);
useconds_t ualarm(useconds_t usecs, useconds_t interval);

extern char **environ;

pid_t fork(void);
pid_t vfork(void);
int execve(const char *path, char *const argv[], char *const envp[]);
int execvp(const char *file, char *const argv[]);
int execv(const char *path, char *const argv[]);
pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait(int *status);
pid_t setsid(void);
int pipe(int pipefd[2]);

void _exit(int status);
void exit(int status);

void *sbrk(intptr_t inc);
int brk(void *addr);

extern int optind;
extern int optopt;
extern int opterr;
extern char *optarg;
int getopt(int argc, char *const argv[], const char *optstring);
