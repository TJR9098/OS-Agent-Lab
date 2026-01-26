#pragma once

#include <stdint.h>
#include <endian.h>
#include <netinet/in.h>
#include <sys/socket.h>

static inline uint16_t htons(uint16_t x) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return bswap_16(x);
#else
  return x;
#endif
}

static inline uint16_t ntohs(uint16_t x) {
  return htons(x);
}

static inline uint32_t htonl(uint32_t x) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return bswap_32(x);
#else
  return x;
#endif
}

static inline uint32_t ntohl(uint32_t x) {
  return htonl(x);
}

uint32_t inet_addr(const char *cp);
char *inet_ntoa(struct in_addr in);
int inet_aton(const char *cp, struct in_addr *inp);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
int inet_pton(int af, const char *src, void *dst);
