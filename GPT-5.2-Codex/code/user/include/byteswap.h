#pragma once

static inline unsigned short bswap_16(unsigned short x) {
  return (unsigned short)((x << 8) | (x >> 8));
}

static inline unsigned int bswap_32(unsigned int x) {
  return ((x & 0x000000ffU) << 24) |
         ((x & 0x0000ff00U) << 8) |
         ((x & 0x00ff0000U) >> 8) |
         ((x & 0xff000000U) >> 24);
}

static inline unsigned long long bswap_64(unsigned long long x) {
  return ((x & 0x00000000000000ffULL) << 56) |
         ((x & 0x000000000000ff00ULL) << 40) |
         ((x & 0x0000000000ff0000ULL) << 24) |
         ((x & 0x00000000ff000000ULL) << 8) |
         ((x & 0x000000ff00000000ULL) >> 8) |
         ((x & 0x0000ff0000000000ULL) >> 24) |
         ((x & 0x00ff000000000000ULL) >> 40) |
         ((x & 0xff00000000000000ULL) >> 56);
}
