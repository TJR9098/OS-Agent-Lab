#pragma once

#include <byteswap.h>

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN

#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BIG_ENDIAN __BIG_ENDIAN
#define BYTE_ORDER __BYTE_ORDER

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htobe16(x) bswap_16(x)
#define htole16(x) (x)
#define be16toh(x) bswap_16(x)
#define le16toh(x) (x)

#define htobe32(x) bswap_32(x)
#define htole32(x) (x)
#define be32toh(x) bswap_32(x)
#define le32toh(x) (x)

#define htobe64(x) bswap_64(x)
#define htole64(x) (x)
#define be64toh(x) bswap_64(x)
#define le64toh(x) (x)
#else
#define htobe16(x) (x)
#define htole16(x) bswap_16(x)
#define be16toh(x) (x)
#define le16toh(x) bswap_16(x)

#define htobe32(x) (x)
#define htole32(x) bswap_32(x)
#define be32toh(x) (x)
#define le32toh(x) bswap_32(x)

#define htobe64(x) (x)
#define htole64(x) bswap_64(x)
#define be64toh(x) (x)
#define le64toh(x) bswap_64(x)
#endif
