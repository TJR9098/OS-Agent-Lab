#pragma once

#include <sys/types.h>

struct tms {
  long tms_utime;
  long tms_stime;
  long tms_cutime;
  long tms_cstime;
};

long times(struct tms *buf);
