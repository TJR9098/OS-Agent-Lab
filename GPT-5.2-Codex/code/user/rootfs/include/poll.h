#pragma once

#include <sys/types.h>

struct pollfd {
  int fd;
  short events;
  short revents;
};

#define POLLIN 0x0001

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
