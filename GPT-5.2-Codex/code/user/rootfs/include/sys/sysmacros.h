#pragma once

#include <sys/types.h>

#define major(dev) ((unsigned int)(((dev) >> 8) & 0xfff))
#define minor(dev) ((unsigned int)(((dev) & 0xff) | (((dev) >> 12) & 0xfffff00)))
#define makedev(maj, min) \
  ((dev_t)((((maj) & 0xfff) << 8) | ((min) & 0xff) | (((min) & 0xfffff00) << 12)))
