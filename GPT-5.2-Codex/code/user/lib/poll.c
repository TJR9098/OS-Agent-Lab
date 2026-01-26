#include <poll.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  (void)timeout;
  int ready = 0;
  for (nfds_t i = 0; i < nfds; i++) {
    fds[i].revents = 0;
    if (fds[i].events & POLLIN) {
      fds[i].revents = POLLIN;
      ready++;
    }
  }
  return ready;
}
