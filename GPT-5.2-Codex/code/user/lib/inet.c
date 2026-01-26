#include <arpa/inet.h>
#include <string.h>

uint32_t inet_addr(const char *cp) {
  (void)cp;
  return INADDR_NONE;
}

char *inet_ntoa(struct in_addr in) {
  static char buf[16];
  (void)in;
  strncpy(buf, "0.0.0.0", sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';
  return buf;
}

int inet_aton(const char *cp, struct in_addr *inp) {
  (void)cp;
  if (inp) {
    inp->s_addr = INADDR_NONE;
  }
  return 0;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
  (void)af;
  (void)src;
  (void)size;
  if (dst && size > 0) {
    dst[0] = '\0';
  }
  return NULL;
}

int inet_pton(int af, const char *src, void *dst) {
  (void)af;
  (void)src;
  (void)dst;
  return 0;
}
