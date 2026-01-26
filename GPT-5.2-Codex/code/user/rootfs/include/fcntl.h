#pragma once

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x40
#define O_EXCL 0x80
#define O_TRUNC 0x200
#define O_APPEND 0x400
#define O_NONBLOCK 0x800
#define O_NOCTTY 0x100
#define O_DIRECTORY 0x10000

#define F_DUPFD 0
#define F_DUPFD_CLOEXEC F_DUPFD
#define F_SETFL 4
#define F_GETFL 3
#define F_SETFD 2
#define FD_CLOEXEC 1

int fcntl(int fd, int cmd, ...);
