#pragma once

#include <sys/types.h>

struct dirent {
  uint64_t d_ino;
  int64_t d_off;
  unsigned short d_reclen;
  unsigned char d_type;
  char d_name[256];
};

typedef struct DIR DIR;

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
void rewinddir(DIR *dirp);

#define DT_DIR 4
#define DT_REG 8
