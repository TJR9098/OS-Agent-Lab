#pragma once

#include <stdint.h>
#include <stddef.h>

#define VFS_NAME_MAX 255

enum vnode_type {
  VNODE_FILE = 1,
  VNODE_DIR = 2,
};

struct vnode;

struct vfs_dirent {
  char name[VFS_NAME_MAX + 1];
  uint8_t type;
  uint64_t ino;
};

struct vfs_timespec {
  int64_t tv_sec;
  int64_t tv_nsec;
};

struct vfs_stat {
  uint64_t st_dev;
  uint64_t st_ino;
  uint32_t st_mode;
  uint32_t st_nlink;
  uint32_t st_uid;
  uint32_t st_gid;
  uint64_t st_rdev;
  uint64_t __pad;
  int64_t st_size;
  int64_t st_blksize;
  int32_t __pad2;
  int64_t st_blocks;
  struct vfs_timespec st_atim;
  struct vfs_timespec st_mtim;
  struct vfs_timespec st_ctim;
  uint64_t __unused[2];
};

struct vnode_ops {
  int (*lookup)(struct vnode *dir, const char *name, struct vnode **out);
  int (*create)(struct vnode *dir, const char *name, struct vnode **out);
  int (*mkdir)(struct vnode *dir, const char *name, struct vnode **out);
  int (*read)(struct vnode *vn, void *buf, size_t len, uint64_t offset);
  int (*write)(struct vnode *vn, const void *buf, size_t len, uint64_t offset);
  int (*truncate)(struct vnode *vn, uint64_t size);
  int (*readdir)(struct vnode *dir, uint64_t offset, struct vfs_dirent *out);
  int (*unlink)(struct vnode *dir, const char *name);
  int (*stat)(struct vnode *vn, struct vfs_stat *st);
};

struct vnode {
  enum vnode_type type;
  const struct vnode_ops *ops;
  void *data;
  struct vnode *parent;
  char name[VFS_NAME_MAX + 1];
  uint32_t refcnt;
};

struct vnode *vfs_alloc_vnode(void);
void vfs_free_vnode(struct vnode *vn);

int vfs_set_root(struct vnode *root);
int vfs_set_cwd(struct vnode *cwd);
struct vnode *vfs_root(void);
struct vnode *vfs_cwd(void);

int vfs_mount(struct vnode *mountpoint, struct vnode *root);
struct vnode *vfs_follow_mount(struct vnode *vn);

int vfs_lookup_at(struct vnode *cwd, const char *path, struct vnode **out);
int vfs_create_at(struct vnode *cwd, const char *path, struct vnode **out);
int vfs_mkdir_at(struct vnode *cwd, const char *path, struct vnode **out);
int vfs_unlink_at(struct vnode *cwd, const char *path);
int vfs_getcwd_at(struct vnode *cwd, char *buf, size_t len);

int vfs_lookup(const char *path, struct vnode **out);
int vfs_create(const char *path, struct vnode **out);
int vfs_mkdir(const char *path, struct vnode **out);
int vfs_readdir(struct vnode *dir, uint64_t offset, struct vfs_dirent *out);
int vfs_read(struct vnode *vn, void *buf, size_t len, uint64_t offset);
int vfs_write(struct vnode *vn, const void *buf, size_t len, uint64_t offset);
int vfs_truncate(struct vnode *vn, uint64_t size);
int vfs_unlink(const char *path);
int vfs_stat(struct vnode *vn, struct vfs_stat *st);
int vfs_getcwd(char *buf, size_t len);
