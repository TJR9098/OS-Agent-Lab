#include <sys/times.h>

long times(struct tms *buf) {
  if (buf) {
    buf->tms_utime = 0;
    buf->tms_stime = 0;
    buf->tms_cutime = 0;
    buf->tms_cstime = 0;
  }
  return 0;
}
