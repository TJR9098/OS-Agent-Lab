#include <stdint.h>

#include "errno.h"
#include "string.h"
#include "vfs.h"

#define RAMFS_MAX_NODES 128
#define RAMFS_MAX_FILE_SIZE 4096

struct ramfs_node {
  int used;
  enum vnode_type type;
  char name[VFS_NAME_MAX + 1];
  struct ramfs_node *parent;
  struct ramfs_node *child;
  struct ramfs_node *sibling;
  size_t size;
  uint8_t data[RAMFS_MAX_FILE_SIZE];
};

static struct ramfs_node g_nodes[RAMFS_MAX_NODES];

static int name_eq_nocase(const char *a, const char *b) {
  while (*a && *b) {
    char ca = *a;
    char cb = *b;
    if (ca >= 'A' && ca <= 'Z') {
      ca = (char)(ca - 'A' + 'a');
    }
    if (cb >= 'A' && cb <= 'Z') {
      cb = (char)(cb - 'A' + 'a');
    }
    if (ca != cb) {
      return 0;
    }
    a++;
    b++;
  }
  return *a == *b;
}

static struct ramfs_node *ramfs_alloc_node(void) {
  for (size_t i = 0; i < RAMFS_MAX_NODES; i++) {
    if (!g_nodes[i].used) {
      memset(&g_nodes[i], 0, sizeof(g_nodes[i]));
      g_nodes[i].used = 1;
      return &g_nodes[i];
    }
  }
  return NULL;
}

static struct vnode *ramfs_wrap_node(struct ramfs_node *node, struct vnode *parent);

static int ramfs_lookup(struct vnode *dir, const char *name, struct vnode **out) {
  struct ramfs_node *d = (struct ramfs_node *)dir->data;
  struct ramfs_node *child = d->child;
  while (child) {
    if (name_eq_nocase(child->name, name)) {
      *out = ramfs_wrap_node(child, dir);
      return 0;
    }
    child = child->sibling;
  }
  return -ENOENT;
}

static int ramfs_create(struct vnode *dir, const char *name, struct vnode **out) {
  struct ramfs_node *d = (struct ramfs_node *)dir->data;
  struct ramfs_node *child = d->child;
  while (child) {
    if (name_eq_nocase(child->name, name)) {
      if (child->type == VNODE_DIR) {
        return -EISDIR;
      }
      *out = ramfs_wrap_node(child, dir);
      return 0;
    }
    child = child->sibling;
  }

  struct ramfs_node *node = ramfs_alloc_node();
  if (!node) {
    return -ENOMEM;
  }
  node->type = VNODE_FILE;
  memcpy(node->name, name, strlen(name) + 1);
  node->parent = d;
  node->sibling = d->child;
  d->child = node;
  *out = ramfs_wrap_node(node, dir);
  return 0;
}

static int ramfs_mkdir(struct vnode *dir, const char *name, struct vnode **out) {
  struct ramfs_node *d = (struct ramfs_node *)dir->data;
  struct ramfs_node *child = d->child;
  while (child) {
    if (name_eq_nocase(child->name, name)) {
      if (child->type == VNODE_DIR) {
        *out = ramfs_wrap_node(child, dir);
        return 0;
      }
      return -EEXIST;
    }
    child = child->sibling;
  }

  struct ramfs_node *node = ramfs_alloc_node();
  if (!node) {
    return -ENOMEM;
  }
  node->type = VNODE_DIR;
  memcpy(node->name, name, strlen(name) + 1);
  node->parent = d;
  node->sibling = d->child;
  d->child = node;
  *out = ramfs_wrap_node(node, dir);
  return 0;
}

static int ramfs_read(struct vnode *vn, void *buf, size_t len, uint64_t offset) {
  struct ramfs_node *node = (struct ramfs_node *)vn->data;
  if (node->type != VNODE_FILE) {
    return -EISDIR;
  }
  if (offset >= node->size) {
    return 0;
  }
  size_t to_copy = node->size - (size_t)offset;
  if (to_copy > len) {
    to_copy = len;
  }
  memcpy(buf, node->data + offset, to_copy);
  return (int)to_copy;
}

static int ramfs_write(struct vnode *vn, const void *buf, size_t len, uint64_t offset) {
  struct ramfs_node *node = (struct ramfs_node *)vn->data;
  if (node->type != VNODE_FILE) {
    return -EISDIR;
  }
  if (offset >= RAMFS_MAX_FILE_SIZE) {
    return 0;
  }
  size_t to_copy = len;
  if (offset + to_copy > RAMFS_MAX_FILE_SIZE) {
    to_copy = RAMFS_MAX_FILE_SIZE - (size_t)offset;
  }
  memcpy(node->data + offset, buf, to_copy);
  if (offset + to_copy > node->size) {
    node->size = offset + to_copy;
  }
  return (int)to_copy;
}

static int ramfs_readdir(struct vnode *dir, uint64_t offset, struct vfs_dirent *out) {
  struct ramfs_node *d = (struct ramfs_node *)dir->data;
  struct ramfs_node *child = d->child;
  uint64_t idx = 0;

  while (child) {
    if (idx == offset) {
      memset(out, 0, sizeof(*out));
      memcpy(out->name, child->name, strlen(child->name) + 1);
      out->type = (uint8_t)child->type;
      out->ino = (uint64_t)(uintptr_t)child;
      return 0;
    }
    idx++;
    child = child->sibling;
  }
  return -ENOENT;
}

static int ramfs_unlink(struct vnode *dir, const char *name) {
  struct ramfs_node *d = (struct ramfs_node *)dir->data;
  struct ramfs_node *prev = NULL;
  struct ramfs_node *child = d->child;

  while (child) {
    if (name_eq_nocase(child->name, name)) {
      if (child->type == VNODE_DIR && child->child) {
        return -ENOTEMPTY;
      }
      if (prev) {
        prev->sibling = child->sibling;
      } else {
        d->child = child->sibling;
      }
      child->used = 0;
      return 0;
    }
    prev = child;
    child = child->sibling;
  }
  return -ENOENT;
}

static int ramfs_truncate(struct vnode *vn, uint64_t size) {
  struct ramfs_node *node = (struct ramfs_node *)vn->data;
  if (node->type != VNODE_FILE) {
    return -EISDIR;
  }
  if (size > RAMFS_MAX_FILE_SIZE) {
    size = RAMFS_MAX_FILE_SIZE;
  }
  if (size < node->size) {
    memset(node->data + size, 0, node->size - size);
  }
  node->size = (size_t)size;
  return 0;
}

static int ramfs_stat(struct vnode *vn, struct vfs_stat *st) {
  struct ramfs_node *node = (struct ramfs_node *)vn->data;
  memset(st, 0, sizeof(*st));
  st->st_size = (int64_t)node->size;
  st->st_ino = (uint64_t)(uintptr_t)node;
  st->st_nlink = 1;
  st->st_mode = (node->type == VNODE_DIR) ? 0040755 : 0100644;
  return 0;
}

static const struct vnode_ops g_ramfs_ops = {
    .lookup = ramfs_lookup,
    .create = ramfs_create,
    .mkdir = ramfs_mkdir,
    .read = ramfs_read,
    .write = ramfs_write,
    .truncate = ramfs_truncate,
    .readdir = ramfs_readdir,
    .unlink = ramfs_unlink,
    .stat = ramfs_stat,
};

static struct vnode *ramfs_wrap_node(struct ramfs_node *node, struct vnode *parent) {
  struct vnode *vn = vfs_alloc_vnode();
  if (!vn) {
    return NULL;
  }
  vn->type = node->type;
  vn->ops = &g_ramfs_ops;
  vn->data = node;
  vn->parent = parent ? parent : vn;
  memcpy(vn->name, node->name, strlen(node->name) + 1);
  return vn;
}

struct vnode *ramfs_init(void) {
  struct ramfs_node *root = ramfs_alloc_node();
  if (!root) {
    return NULL;
  }
  root->type = VNODE_DIR;
  root->name[0] = '\0';
  root->parent = root;
  return ramfs_wrap_node(root, NULL);
}
