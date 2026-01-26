#pragma once

#include <stddef.h>
#include <stdint.h>

struct vnode;
struct vfs_stat;

struct file {
  int ref;
  int readable;
  int writable;
  uint64_t off;
  struct vnode *vn;
};

struct file *file_alloc(void);
struct file *file_dup(struct file *f);
void file_close(struct file *f);
int file_read(struct file *f, void *buf, size_t len);
int file_write(struct file *f, const void *buf, size_t len);
int file_truncate(struct file *f, uint64_t size);
int file_stat(struct file *f, struct vfs_stat *st);
