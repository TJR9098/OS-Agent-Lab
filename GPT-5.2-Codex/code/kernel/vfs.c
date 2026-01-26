#include <stdint.h>

#include "errno.h"
#include "log.h"
#include "string.h"
#include "vfs.h"

#define VNODE_POOL_SIZE 256
#define MAX_MOUNTS 8

static struct vnode g_vnode_pool[VNODE_POOL_SIZE];
static uint8_t g_vnode_used[VNODE_POOL_SIZE];

struct mount {
  struct vnode *mountpoint;
  struct vnode *root;
};

static struct mount g_mounts[MAX_MOUNTS];
static struct vnode *g_root;
static struct vnode *g_cwd;

struct vnode *vfs_alloc_vnode(void) {
  for (size_t i = 0; i < VNODE_POOL_SIZE; i++) {
    if (!g_vnode_used[i]) {
      g_vnode_used[i] = 1;
      memset(&g_vnode_pool[i], 0, sizeof(g_vnode_pool[i]));
      g_vnode_pool[i].refcnt = 1;
      return &g_vnode_pool[i];
    }
  }
  return NULL;
}

void vfs_free_vnode(struct vnode *vn) {
  if (!vn) {
    return;
  }
  for (size_t i = 0; i < VNODE_POOL_SIZE; i++) {
    if (&g_vnode_pool[i] == vn) {
      g_vnode_used[i] = 0;
      return;
    }
  }
}

int vfs_set_root(struct vnode *root) {
  g_root = root;
  if (!g_cwd) {
    g_cwd = root;
  }
  return 0;
}

int vfs_set_cwd(struct vnode *cwd) {
  g_cwd = cwd;
  return 0;
}

struct vnode *vfs_root(void) {
  return g_root;
}

struct vnode *vfs_cwd(void) {
  return g_cwd;
}

int vfs_mount(struct vnode *mountpoint, struct vnode *root) {
  if (!mountpoint || !root) {
    return -EINVAL;
  }
  for (size_t i = 0; i < MAX_MOUNTS; i++) {
    if (!g_mounts[i].mountpoint) {
      g_mounts[i].mountpoint = mountpoint;
      g_mounts[i].root = root;
      if (root->name[0] == '\0' && mountpoint->name[0] != '\0') {
        memcpy(root->name, mountpoint->name, strlen(mountpoint->name) + 1);
      }
      if (mountpoint->parent) {
        root->parent = mountpoint->parent;
      }
      return 0;
    }
  }
  return -ENOSPC;
}

struct vnode *vfs_follow_mount(struct vnode *vn) {
  if (!vn) {
    return NULL;
  }
  for (size_t i = 0; i < MAX_MOUNTS; i++) {
    if (g_mounts[i].mountpoint == vn) {
      return g_mounts[i].root;
    }
  }
  return vn;
}

static const char *path_next(const char *p, char *name, size_t name_len) {
  size_t n = 0;

  while (*p == '/') {
    p++;
  }
  if (*p == '\0') {
    return NULL;
  }

  while (*p && *p != '/') {
    if (n + 1 < name_len) {
      name[n++] = *p;
    }
    p++;
  }
  name[n] = '\0';
  return p;
}

static int vfs_resolve_parent_at(struct vnode *cwd,
                                 const char *path,
                                 struct vnode **out_parent,
                                 char *name,
                                 size_t name_len,
                                 int create_parents) {
  struct vnode *cur = NULL;
  const char *p = path;

  if (!path || !out_parent || !name) {
    return -EINVAL;
  }

  if (path[0] == '/') {
    cur = vfs_root();
  } else {
    cur = cwd ? cwd : vfs_cwd();
  }
  if (!cur) {
    return -ENOENT;
  }
  cur = vfs_follow_mount(cur);

  for (;;) {
    const char *next = path_next(p, name, name_len);
    if (!next) {
      return -EINVAL;
    }

    const char *q = next;
    while (*q == '/') {
      q++;
    }

    if (*q == '\0') {
      *out_parent = cur;
      return 0;
    }

    if (strcmp(name, ".") == 0) {
      p = q;
      continue;
    }
    if (strcmp(name, "..") == 0) {
      if (cur->parent) {
        cur = cur->parent;
      }
      p = q;
      continue;
    }

    if (cur->type != VNODE_DIR || !cur->ops || !cur->ops->lookup) {
      return -ENOTDIR;
    }

    struct vnode *next_vn = NULL;
    int ret = cur->ops->lookup(cur, name, &next_vn);
    if (ret != 0) {
      if (ret == -ENOENT && create_parents) {
        if (!cur->ops->mkdir) {
          return -ENOSYS;
        }
        ret = cur->ops->mkdir(cur, name, &next_vn);
      }
      if (ret != 0) {
        return ret;
      }
    }
    cur = vfs_follow_mount(next_vn);
    p = q;
  }
}

int vfs_lookup_at(struct vnode *cwd, const char *path, struct vnode **out) {
  struct vnode *cur;
  char name[VFS_NAME_MAX + 1];
  const char *p = path;

  if (!path || !out) {
    return -EINVAL;
  }

  if (path[0] == '/') {
    cur = vfs_root();
  } else {
    cur = cwd ? cwd : vfs_cwd();
  }
  if (!cur) {
    return -ENOENT;
  }
  cur = vfs_follow_mount(cur);

  for (;;) {
    p = path_next(p, name, sizeof(name));
    if (!p) {
      *out = cur;
      return 0;
    }

    if (strcmp(name, ".") == 0) {
      continue;
    }
    if (strcmp(name, "..") == 0) {
      if (cur->parent) {
        cur = cur->parent;
      }
      continue;
    }

    if (cur->type != VNODE_DIR || !cur->ops || !cur->ops->lookup) {
      return -ENOTDIR;
    }

    struct vnode *next_vn = NULL;
    int ret = cur->ops->lookup(cur, name, &next_vn);
    if (ret != 0) {
      return ret;
    }
    cur = vfs_follow_mount(next_vn);
  }
}

int vfs_create_at(struct vnode *cwd, const char *path, struct vnode **out) {
  struct vnode *vn = NULL;
  int ret = vfs_lookup_at(cwd, path, &vn);
  if (ret == 0) {
    if (vn->type == VNODE_DIR) {
      return -EISDIR;
    }
    if (out) {
      *out = vn;
    }
    return 0;
  }
  if (ret != -ENOENT) {
    return ret;
  }

  struct vnode *parent = NULL;
  char name[VFS_NAME_MAX + 1];
  ret = vfs_resolve_parent_at(cwd, path, &parent, name, sizeof(name), 1);
  if (ret != 0) {
    return ret;
  }
  if (name[0] == '\0') {
    return -EINVAL;
  }
  if (!parent->ops || !parent->ops->create) {
    return -ENOSYS;
  }
  return parent->ops->create(parent, name, out);
}

int vfs_mkdir_at(struct vnode *cwd, const char *path, struct vnode **out) {
  struct vnode *vn = NULL;
  int ret = vfs_lookup_at(cwd, path, &vn);
  if (ret == 0) {
    if (vn->type == VNODE_DIR) {
      if (out) {
        *out = vn;
      }
      return 0;
    }
    return -EEXIST;
  }
  if (ret != -ENOENT) {
    return ret;
  }

  struct vnode *parent = NULL;
  char name[VFS_NAME_MAX + 1];
  ret = vfs_resolve_parent_at(cwd, path, &parent, name, sizeof(name), 1);
  if (ret != 0) {
    return ret;
  }
  if (name[0] == '\0') {
    return -EINVAL;
  }
  if (!parent->ops || !parent->ops->mkdir) {
    return -ENOSYS;
  }
  return parent->ops->mkdir(parent, name, out);
}

int vfs_readdir(struct vnode *dir, uint64_t offset, struct vfs_dirent *out) {
  if (!dir || !out) {
    return -EINVAL;
  }
  if (dir->type != VNODE_DIR || !dir->ops || !dir->ops->readdir) {
    return -ENOTDIR;
  }

  if (offset == 0) {
    memset(out, 0, sizeof(*out));
    out->type = VNODE_DIR;
    out->ino = 0;
    memcpy(out->name, ".", 2);
    return 0;
  }
  if (offset == 1) {
    memset(out, 0, sizeof(*out));
    out->type = VNODE_DIR;
    out->ino = 0;
    memcpy(out->name, "..", 3);
    return 0;
  }
  return dir->ops->readdir(dir, offset - 2, out);
}

int vfs_read(struct vnode *vn, void *buf, size_t len, uint64_t offset) {
  if (!vn || !buf) {
    return -EINVAL;
  }
  if (!vn->ops || !vn->ops->read) {
    return -ENOSYS;
  }
  return vn->ops->read(vn, buf, len, offset);
}

int vfs_write(struct vnode *vn, const void *buf, size_t len, uint64_t offset) {
  if (!vn || !buf) {
    return -EINVAL;
  }
  if (!vn->ops || !vn->ops->write) {
    return -ENOSYS;
  }
  return vn->ops->write(vn, buf, len, offset);
}

int vfs_truncate(struct vnode *vn, uint64_t size) {
  if (!vn) {
    return -EINVAL;
  }
  if (!vn->ops || !vn->ops->truncate) {
    return -ENOSYS;
  }
  return vn->ops->truncate(vn, size);
}

int vfs_unlink_at(struct vnode *cwd, const char *path) {
  struct vnode *parent = NULL;
  char name[VFS_NAME_MAX + 1];
  int ret = vfs_resolve_parent_at(cwd, path, &parent, name, sizeof(name), 0);
  if (ret != 0) {
    return ret;
  }
  if (name[0] == '\0') {
    return -EINVAL;
  }
  if (!parent->ops || !parent->ops->unlink) {
    return -ENOSYS;
  }
  return parent->ops->unlink(parent, name);
}

int vfs_stat(struct vnode *vn, struct vfs_stat *st) {
  if (!vn || !st) {
    return -EINVAL;
  }
  if (!vn->ops || !vn->ops->stat) {
    return -ENOSYS;
  }
  return vn->ops->stat(vn, st);
}

int vfs_getcwd_at(struct vnode *cwd, char *buf, size_t len) {
  struct vnode *cur = cwd ? cwd : vfs_cwd();
  char stack[16][VFS_NAME_MAX + 1];
  size_t depth = 0;
  size_t i;
  size_t pos = 0;

  if (!buf || len == 0) {
    return -EINVAL;
  }
  if (!cur || !vfs_root()) {
    return -ENOENT;
  }
  if (cur == vfs_root()) {
    if (len < 2) {
      return -ERANGE;
    }
    buf[0] = '/';
    buf[1] = '\0';
    return 0;
  }

  while (cur && cur != vfs_root() && depth < 16) {
    memcpy(stack[depth], cur->name, VFS_NAME_MAX + 1);
    depth++;
    cur = cur->parent;
  }

  if (depth == 0) {
    if (len < 2) {
      return -ERANGE;
    }
    buf[0] = '/';
    buf[1] = '\0';
    return 0;
  }

  buf[0] = '/';
  pos = 1;

  for (i = depth; i > 0; i--) {
    size_t n = strlen(stack[i - 1]);
    if (pos + n + 1 >= len) {
      return -ERANGE;
    }
    memcpy(buf + pos, stack[i - 1], n);
    pos += n;
    if (i > 1) {
      buf[pos++] = '/';
    }
  }
  buf[pos] = '\0';
  return 0;
}

int vfs_lookup(const char *path, struct vnode **out) {
  return vfs_lookup_at(NULL, path, out);
}

int vfs_create(const char *path, struct vnode **out) {
  return vfs_create_at(NULL, path, out);
}

int vfs_mkdir(const char *path, struct vnode **out) {
  return vfs_mkdir_at(NULL, path, out);
}

int vfs_unlink(const char *path) {
  return vfs_unlink_at(NULL, path);
}

int vfs_getcwd(char *buf, size_t len) {
  return vfs_getcwd_at(NULL, buf, len);
}
