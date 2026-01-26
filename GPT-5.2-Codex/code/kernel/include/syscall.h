#pragma once

#include <stdint.h>

struct trapframe;

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

long syscall_dispatch(struct trapframe *tf);
