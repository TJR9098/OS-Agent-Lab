#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct entry {
  char *name;
  unsigned char type;
};

static const char *basename_simple(const char *path) {
  const char *p = strrchr(path, '/');
  if (!p) {
    return path;
  }
  if (p[1] == '\0') {
    return p;
  }
  return p + 1;
}

static int ci_cmp(const char *a, const char *b) {
  while (*a && *b) {
    int ca = tolower((unsigned char)*a);
    int cb = tolower((unsigned char)*b);
    if (ca != cb) {
      return ca - cb;
    }
    a++;
    b++;
  }
  return (unsigned char)*a - (unsigned char)*b;
}

static void sort_entries(struct entry *ents, size_t count) {
  for (size_t i = 0; i < count; i++) {
    for (size_t j = i + 1; j < count; j++) {
      if (ci_cmp(ents[i].name, ents[j].name) > 0) {
        struct entry tmp = ents[i];
        ents[i] = ents[j];
        ents[j] = tmp;
      }
    }
  }
}

static void free_entries(struct entry *ents, size_t count) {
  for (size_t i = 0; i < count; i++) {
    free(ents[i].name);
  }
  free(ents);
}

static void print_entry(const struct entry *ent, int *first) {
  if (!ent || !ent->name) {
    return;
  }
  if (!*first) {
    printf(" ");
  }
  printf("%s%s", ent->name, (ent->type == DT_DIR) ? "/" : "");
  *first = 0;
}

static int list_dir(const char *path) {
  int fd = openat(AT_FDCWD, path, O_RDONLY | O_DIRECTORY);
  if (fd < 0) {
    return fd;
  }

  struct entry *ents = NULL;
  size_t count = 0;
  size_t cap = 0;
  struct entry dot = {0};
  struct entry dotdot = {0};

  char buf[512];
  for (;;) {
    int n = getdents64(fd, buf, sizeof(buf));
    if (n <= 0) {
      break;
    }
    int pos = 0;
    while (pos < n) {
      struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + pos);
      if (d->d_reclen == 0) {
        break;
      }
      const char *name = d->d_name;
      if (strcmp(name, ".") == 0) {
        dot.name = strdup(name);
        dot.type = d->d_type;
      } else if (strcmp(name, "..") == 0) {
        dotdot.name = strdup(name);
        dotdot.type = d->d_type;
      } else {
        if (count == cap) {
          size_t new_cap = cap ? cap * 2 : 16;
          struct entry *next = realloc(ents, new_cap * sizeof(*ents));
          if (!next) {
            close(fd);
            free_entries(ents, count);
            return -1;
          }
          ents = next;
          cap = new_cap;
        }
        ents[count].name = strdup(name);
        ents[count].type = d->d_type;
        count++;
      }
      pos += d->d_reclen;
    }
  }
  close(fd);

  int first = 1;
  if (dot.name) {
    print_entry(&dot, &first);
    free(dot.name);
  }
  if (dotdot.name) {
    print_entry(&dotdot, &first);
    free(dotdot.name);
  }

  sort_entries(ents, count);
  for (size_t i = 0; i < count; i++) {
    print_entry(&ents[i], &first);
  }
  printf("\n");
  free_entries(ents, count);
  return 0;
}

static int list_path(const char *path) {
  int ret = list_dir(path);
  if (ret == 0) {
    return 0;
  }
  int fd = openat(AT_FDCWD, path, O_RDONLY);
  if (fd < 0) {
    dprintf(2, "ls: %s failed (%d)\n", path, fd);
    return -1;
  }
  close(fd);
  const char *name = basename_simple(path);
  printf("%s\n", name);
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    return list_path(".");
  }
  for (int i = 1; i < argc; i++) {
    int ret = list_path(argv[i]);
    if (ret != 0) {
      return 1;
    }
  }
  return 0;
}
