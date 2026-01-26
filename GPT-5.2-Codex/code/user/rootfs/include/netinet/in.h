#pragma once

#include <stdint.h>

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr {
  in_addr_t s_addr;
};

struct in6_addr {
  uint8_t s6_addr[16];
};

struct sockaddr_in {
  unsigned short sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
  unsigned char sin_zero[8];
};

struct sockaddr_in6 {
  unsigned short sin6_family;
  in_port_t sin6_port;
  uint32_t sin6_flowinfo;
  struct in6_addr sin6_addr;
  uint32_t sin6_scope_id;
};

#define INADDR_ANY 0x00000000U
#define INADDR_BROADCAST 0xffffffffU
#define INADDR_LOOPBACK 0x7f000001U
#define INADDR_NONE 0xffffffffU
