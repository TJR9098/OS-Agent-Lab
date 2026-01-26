#pragma once

#include <stdint.h>

#ifndef USER_SIZE_T
typedef uint64_t size_t;
#define USER_SIZE_T 1
#endif

#ifndef USER_PTRDIFF_T
typedef int64_t ptrdiff_t;
#define USER_PTRDIFF_T 1
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif
