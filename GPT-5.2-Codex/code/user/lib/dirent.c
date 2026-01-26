#include <dirent.h>
#include <fcntl.h>
#include <linux/dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct DIR {
  int fd;
  int pos;
  int nread;
  char buf[512];
  struct dirent ent;
};

DIR *opendir(const char *name) {
  int fd = open(name, O_RDONLY | O_DIRECTORY);
  if (fd < 0) {
    return NULL;
  }
  DIR *d = (DIR *)malloc(sizeof(*d));
  if (!d) {
    close(fd);
    return NULL;
  }
  d->fd = fd;
  d->pos = 0;
  d->nread = 0;
  return d;
}

struct dirent *readdir(DIR *dirp) {
  if (!dirp) {
    return NULL;
  }
  for (;;) {
    if (dirp->pos >= dirp->nread) {
      int n = getdents64(dirp->fd, dirp->buf, sizeof(dirp->buf));
      if (n <= 0) {
        return NULL;
      }
      dirp->nread = n;
      dirp->pos = 0;
    }
    struct linux_dirent64 *d = (struct linux_dirent64 *)(dirp->buf + dirp->pos);
    if (d->d_reclen == 0) {
      dirp->pos = dirp->nread;
      continue;
    }
    dirp->pos += d->d_reclen;
    dirp->ent.d_ino = d->d_ino;
    dirp->ent.d_off = d->d_off;
    dirp->ent.d_reclen = d->d_reclen;
    dirp->ent.d_type = d->d_type;
    size_t len = strnlen(d->d_name, sizeof(dirp->ent.d_name) - 1);
    memcpy(dirp->ent.d_name, d->d_name, len);
    dirp->ent.d_name[len] = '\0';
    return &dirp->ent;
  }
}

int closedir(DIR *dirp) {
  if (!dirp) {
    return -1;
  }
  int fd = dirp->fd;
  free(dirp);
  return close(fd);
}

void rewinddir(DIR *dirp) {
  if (!dirp) {
    return;
  }
  lseek(dirp->fd, 0, SEEK_SET);
  dirp->pos = 0;
  dirp->nread = 0;
}
