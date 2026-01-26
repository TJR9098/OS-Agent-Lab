#include <stdint.h>

#include "errno.h"
#include "file.h"
#include "string.h"
#include "vfs.h"

#define NFILE 64

static struct file g_file[NFILE];
static uint8_t g_file_used[NFILE];

struct file *file_alloc(void) {
  for (int i = 0; i < NFILE; i++) {
    if (!g_file_used[i]) {
      g_file_used[i] = 1;
      memset(&g_file[i], 0, sizeof(g_file[i]));
      g_file[i].ref = 1;
      return &g_file[i];
    }
  }
  return NULL;
}

struct file *file_dup(struct file *f) {
  if (!f) {
    return NULL;
  }
  f->ref++;
  return f;
}

void file_close(struct file *f) {
  if (!f) {
    return;
  }
  if (--f->ref == 0) {
    f->vn = NULL;
    f->off = 0;
    f->readable = 0;
    f->writable = 0;
    for (int i = 0; i < NFILE; i++) {
      if (&g_file[i] == f) {
        g_file_used[i] = 0;
        break;
      }
    }
  }
}

int file_read(struct file *f, void *buf, size_t len) {
  if (!f || !f->readable) {
    return -EBADF;
  }
  int n = vfs_read(f->vn, buf, len, f->off);
  if (n > 0) {
    f->off += (uint64_t)n;
  }
  return n;
}

int file_write(struct file *f, const void *buf, size_t len) {
  if (!f || !f->writable) {
    return -EBADF;
  }
  int n = vfs_write(f->vn, buf, len, f->off);
  if (n > 0) {
    f->off += (uint64_t)n;
  }
  return n;
}

int file_truncate(struct file *f, uint64_t size) {
  if (!f || !f->writable) {
    return -EBADF;
  }
  int ret = vfs_truncate(f->vn, size);
  if (ret == 0 && f->off > size) {
    f->off = size;
  }
  return ret;
}

int file_stat(struct file *f, struct vfs_stat *st) {
  if (!f) {
    return -EBADF;
  }
  return vfs_stat(f->vn, st);
}
