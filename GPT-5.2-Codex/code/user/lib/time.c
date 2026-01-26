#include <sys/time.h>
#include <time.h>

int gettimeofday(struct timeval *tv, struct timezone *tz) {
  if (tz) {
    tz->tz_minuteswest = 0;
    tz->tz_dsttime = 0;
  }
  if (tv) {
    tv->tv_sec = 0;
    tv->tv_usec = 0;
  }
  return 0;
}

int settimeofday(const struct timeval *tv, const struct timezone *tz) {
  (void)tv;
  (void)tz;
  return 0;
}

time_t time(time_t *t) {
  time_t now = 0;
  if (t) {
    *t = now;
  }
  return now;
}

clock_t clock(void) {
  return 0;
}

static char g_timebuf[32] = "Thu Jan  1 00:00:00 1970\n";

char *asctime(const struct tm *tm) {
  (void)tm;
  return g_timebuf;
}

char *ctime(const time_t *t) {
  (void)t;
  return g_timebuf;
}

struct tm *gmtime(const time_t *t) {
  return localtime(t);
}

struct tm *gmtime_r(const time_t *t, struct tm *result) {
  return localtime_r(t, result);
}

static void fill_tm(struct tm *tm) {
  if (!tm) {
    return;
  }
  tm->tm_sec = 0;
  tm->tm_min = 0;
  tm->tm_hour = 0;
  tm->tm_mday = 1;
  tm->tm_mon = 0;
  tm->tm_year = 70;
  tm->tm_wday = 4;
  tm->tm_yday = 0;
  tm->tm_isdst = 0;
}

struct tm *localtime(const time_t *t) {
  static struct tm tm;
  (void)t;
  fill_tm(&tm);
  return &tm;
}

struct tm *localtime_r(const time_t *t, struct tm *result) {
  (void)t;
  fill_tm(result);
  return result;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
  (void)req;
  if (rem) {
    rem->tv_sec = 0;
    rem->tv_nsec = 0;
  }
  return 0;
}

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm) {
  (void)format;
  (void)tm;
  if (!s || max == 0) {
    return 0;
  }
  s[0] = '\0';
  return 0;
}

time_t mktime(struct tm *tm) {
  (void)tm;
  return 0;
}

time_t timegm(struct tm *tm) {
  return mktime(tm);
}

char *strptime(const char *s, const char *format, struct tm *tm) {
  (void)format;
  (void)tm;
  return (char *)s;
}
