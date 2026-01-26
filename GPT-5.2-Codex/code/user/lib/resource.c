#include <sys/resource.h>

int getrusage(int who, struct rusage *usage) {
  (void)who;
  if (usage) {
    usage->ru_utime.tv_sec = 0;
    usage->ru_utime.tv_usec = 0;
    usage->ru_stime.tv_sec = 0;
    usage->ru_stime.tv_usec = 0;
  }
  return 0;
}

int getrlimit(int resource, struct rlimit *rlim) {
  (void)resource;
  if (rlim) {
    rlim->rlim_cur = RLIM_INFINITY;
    rlim->rlim_max = RLIM_INFINITY;
  }
  return 0;
}

int setrlimit(int resource, const struct rlimit *rlim) {
  (void)resource;
  (void)rlim;
  return 0;
}
