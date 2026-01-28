#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *base_name(const char *path) {
  size_t len = strlen(path);
  while (len > 0 && path[len - 1] == '/') {
    len--;
  }
  const char *end = path + len;
  const char *p = end;
  while (p > path && p[-1] != '/') {
    p--;
  }
  return p;
}

static int join_path(char *out, size_t outsz, const char *dir, const char *name) {
  if (!out || !dir || !name) {
    return -1;
  }
  size_t dlen = strlen(dir);
  int need_slash = (dlen > 0 && dir[dlen - 1] != '/');
  if (snprintf(out, outsz, "%s%s%s", dir, need_slash ? "/" : "", name) >= (int)outsz) {
    return -1;
  }
  return 0;
}

static int confirm_overwrite(const char *path) {
  printf("cp: overwrite '%s'? [y/N] ", path);
  fflush(stdout);
  char buf[8];
  if (!fgets(buf, sizeof(buf), stdin)) {
    return 0;
  }
  return (buf[0] == 'y' || buf[0] == 'Y');
}

static int copy_file(const char *src, const char *dst) {
  struct stat st;
  if (stat(dst, &st) == 0) {
    if (S_ISDIR(st.st_mode)) {
      dprintf(2, "cp: %s is a directory\n", dst);
      return -1;
    }
    if (!confirm_overwrite(dst)) {
      return 0;
    }
  }

  int in = open(src, O_RDONLY);
  if (in < 0) {
    dprintf(2, "cp: open %s failed (%d)\n", src, in);
    return -1;
  }
  int out = open(dst, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (out < 0) {
    dprintf(2, "cp: open %s failed (%d)\n", dst, out);
    close(in);
    return -1;
  }

  char buf[512];
  for (;;) {
    int n = read(in, buf, sizeof(buf));
    if (n < 0) {
      dprintf(2, "cp: read %s failed (%d)\n", src, n);
      close(in);
      close(out);
      return -1;
    }
    if (n == 0) {
      break;
    }
    int off = 0;
    while (off < n) {
      int w = write(out, buf + off, (size_t)(n - off));
      if (w < 0) {
        dprintf(2, "cp: write %s failed (%d)\n", dst, w);
        close(in);
        close(out);
        return -1;
      }
      off += w;
    }
  }

  close(in);
  close(out);
  return 0;
}

static int copy_path(const char *src, const char *dst, int dst_is_dir);

static int copy_dir(const char *src, const char *dst) {
  struct stat st;
  if (stat(dst, &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      dprintf(2, "cp: %s is not a directory\n", dst);
      return -1;
    }
  } else {
    if (mkdir(dst, 0755) < 0) {
      dprintf(2, "cp: mkdir %s failed (%d)\n", dst, errno);
      return -1;
    }
  }

  DIR *dir = opendir(src);
  if (!dir) {
    dprintf(2, "cp: opendir %s failed (%d)\n", src, errno);
    return -1;
  }
  struct dirent *ent;
  int ret = 0;
  while ((ent = readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }
    char src_path[512];
    char dst_path[512];
    if (join_path(src_path, sizeof(src_path), src, ent->d_name) != 0 ||
        join_path(dst_path, sizeof(dst_path), dst, ent->d_name) != 0) {
      dprintf(2, "cp: path too long\n");
      ret = -1;
      break;
    }
    if (copy_path(src_path, dst_path, 0) != 0) {
      ret = -1;
      break;
    }
  }
  closedir(dir);
  return ret;
}

static int copy_path(const char *src, const char *dst, int dst_is_dir) {
  struct stat st;
  if (stat(src, &st) < 0) {
    dprintf(2, "cp: stat %s failed (%d)\n", src, errno);
    return -1;
  }

  char dst_path[512];
  const char *final_dst = dst;
  if (dst_is_dir) {
    const char *name = base_name(src);
    if (join_path(dst_path, sizeof(dst_path), dst, name) != 0) {
      dprintf(2, "cp: path too long\n");
      return -1;
    }
    final_dst = dst_path;
  }

  if (S_ISDIR(st.st_mode)) {
    return copy_dir(src, final_dst);
  }
  return copy_file(src, final_dst);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    dprintf(2, "usage: cp SRC... DST\n");
    return 1;
  }

  const char *dst = argv[argc - 1];
  struct stat st;
  int dst_exists = (stat(dst, &st) == 0);
  int dst_is_dir = dst_exists && S_ISDIR(st.st_mode);

  if (argc > 3 && !dst_is_dir) {
    dprintf(2, "cp: target %s is not a directory\n", dst);
    return 1;
  }

  for (int i = 1; i < argc - 1; i++) {
    const char *src = argv[i];
    int ret = copy_path(src, dst, dst_is_dir);
    if (ret != 0) {
      return 1;
    }
  }
  return 0;
}
