#include <mntent.h>
#include <string.h>

FILE *setmntent(const char *filename, const char *type) {
  return fopen(filename, type);
}

struct mntent *getmntent(FILE *stream) {
  (void)stream;
  return NULL;
}

int endmntent(FILE *stream) {
  if (!stream) {
    return 1;
  }
  return fclose(stream);
}

char *hasmntopt(const struct mntent *mnt, const char *opt) {
  if (!mnt || !mnt->mnt_opts || !opt) {
    return NULL;
  }
  return strstr(mnt->mnt_opts, opt);
}
