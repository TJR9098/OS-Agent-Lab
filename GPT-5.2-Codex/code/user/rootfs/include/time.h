#pragma once

#include <sys/types.h>

typedef int64_t time_t;
typedef int64_t clock_t;

#define CLOCKS_PER_SEC 100

#ifndef TIMESPEC_DEFINED
#define TIMESPEC_DEFINED 1
struct timespec {
  int64_t tv_sec;
  int64_t tv_nsec;
};
#endif

struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
};

time_t time(time_t *t);
clock_t clock(void);
char *asctime(const struct tm *tm);
char *ctime(const time_t *t);
struct tm *gmtime(const time_t *t);
struct tm *gmtime_r(const time_t *t, struct tm *result);
struct tm *localtime(const time_t *t);
struct tm *localtime_r(const time_t *t, struct tm *result);
int nanosleep(const struct timespec *req, struct timespec *rem);
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
time_t mktime(struct tm *tm);
time_t timegm(struct tm *tm);
char *strptime(const char *s, const char *format, struct tm *tm);
