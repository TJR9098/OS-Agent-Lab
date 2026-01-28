#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define MAX_LINE 256
#define MAX_ARGS 16
#define MAX_MATCHES 64

struct match {
  char name[256];
  unsigned char type;
};

static struct termios g_saved_term;
static int g_term_valid = 0;

static void print_prompt(void);

static void tty_set_mode(int raw) {
  struct termios term;
  if (tcgetattr(0, &term) != 0) {
    return;
  }
  if (!g_term_valid) {
    g_saved_term = term;
    g_term_valid = 1;
  }
  if (raw) {
    term.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
  } else {
    term.c_lflag |= (tcflag_t)(ICANON | ECHO);
  }
  tcsetattr(0, TCSANOW, &term);
}

static void tty_restore(void) {
  if (!g_term_valid) {
    return;
  }
  struct termios term = g_saved_term;
  term.c_lflag |= (tcflag_t)(ICANON | ECHO);
  tcsetattr(0, TCSANOW, &term);
}

static void write_str(const char *s) {
  if (!s) {
    return;
  }
  write(1, s, strlen(s));
}

static int starts_with_ci(const char *s, const char *prefix) {
  while (*prefix && *s) {
    int a = tolower((unsigned char)*s++);
    int b = tolower((unsigned char)*prefix++);
    if (a != b) {
      return 0;
    }
  }
  return *prefix == '\0';
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

static void sort_matches(struct match *matches, int count) {
  for (int i = 0; i < count; i++) {
    for (int j = i + 1; j < count; j++) {
      if (ci_cmp(matches[i].name, matches[j].name) > 0) {
        struct match tmp = matches[i];
        matches[i] = matches[j];
        matches[j] = tmp;
      }
    }
  }
}

static int collect_matches(const char *dirpath, const char *prefix, struct match *out, int max) {
  DIR *dir = opendir(dirpath);
  if (!dir) {
    return 0;
  }
  int count = 0;
  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }
    if (!starts_with_ci(ent->d_name, prefix)) {
      continue;
    }
    if (count >= max) {
      break;
    }
    strncpy(out[count].name, ent->d_name, sizeof(out[count].name) - 1);
    out[count].name[sizeof(out[count].name) - 1] = '\0';
    out[count].type = ent->d_type;
    count++;
  }
  closedir(dir);
  sort_matches(out, count);
  return count;
}

static size_t common_prefix_len(const struct match *matches, int count, const char *prefix) {
  size_t base = strlen(prefix);
  if (count <= 0) {
    return base;
  }
  size_t limit = strlen(matches[0].name);
  for (int i = 1; i < count; i++) {
    size_t j = 0;
    size_t max = strlen(matches[i].name);
    if (max < limit) {
      limit = max;
    }
    for (; j < limit; j++) {
      int a = tolower((unsigned char)matches[0].name[j]);
      int b = tolower((unsigned char)matches[i].name[j]);
      if (a != b) {
        break;
      }
    }
    limit = j;
  }
  if (limit < base) {
    return base;
  }
  return limit;
}

static int read_line(char *buf, size_t size) {
  size_t len = 0;
  buf[0] = '\0';
  for (;;) {
    char c = 0;
    int n = (int)read(0, &c, 1);
    if (n <= 0) {
      return -1;
    }
    if (c == '\r') {
      c = '\n';
    }
    if (c == '\n') {
      write_str("\n");
      buf[len] = '\0';
      return (int)len;
    }
    if (c == 0x7f || c == 0x08) {
      if (len > 0) {
        len--;
        buf[len] = '\0';
        write_str("\b \b");
      }
      continue;
    }
    if (c == '\t') {
      buf[len] = '\0';
      size_t start = len;
      while (start > 0 && !isspace((unsigned char)buf[start - 1])) {
        start--;
      }

      char token[256];
      size_t token_len = len - start;
      if (token_len >= sizeof(token)) {
        token_len = sizeof(token) - 1;
      }
      memcpy(token, buf + start, token_len);
      token[token_len] = '\0';

      const char *scan_dir = ".";
      const char *prefix = token;
      int command_pos = (start == 0);
      char dirbuf[256];

      if (command_pos && strchr(token, '/') == NULL) {
        scan_dir = "/bin";
      } else {
        char *slash = strrchr(token, '/');
        if (slash) {
          size_t dlen = (size_t)(slash - token);
          if (dlen == 0) {
            scan_dir = "/";
          } else {
            if (dlen >= sizeof(dirbuf)) {
              dlen = sizeof(dirbuf) - 1;
            }
            memcpy(dirbuf, token, dlen);
            dirbuf[dlen] = '\0';
            scan_dir = dirbuf;
          }
          prefix = slash + 1;
        }
      }

      struct match *matches = (struct match *)malloc(sizeof(*matches) * (size_t)MAX_MATCHES);
      if (!matches) {
        write_str("\a");
        continue;
      }
      int count = collect_matches(scan_dir, prefix, matches, MAX_MATCHES);
      if (count == 0) {
        free(matches);
        write_str("\a");
        continue;
      }

      size_t prefix_len = strlen(prefix);
      size_t common_len = common_prefix_len(matches, count, prefix);
      if (common_len > prefix_len) {
        const char *full = matches[0].name;
        size_t add = common_len - prefix_len;
        if (len + add >= size) {
          add = size - len - 1;
        }
        if (add > 0) {
          memcpy(buf + len, full + prefix_len, add);
          len += add;
          buf[len] = '\0';
          write(1, full + prefix_len, add);
        }
      }

      if (count == 1) {
        int is_dir = (matches[0].type == DT_DIR);
        if (is_dir && (len + 1) < size && (len == 0 || buf[len - 1] != '/')) {
          buf[len++] = '/';
          buf[len] = '\0';
          write_str("/");
        }
        free(matches);
        continue;
      }

      if (common_len == prefix_len) {
        write_str("\n");
        for (int i = 0; i < count; i++) {
          if (i > 0) {
            write_str(" ");
          }
          write_str(matches[i].name);
          if (matches[i].type == DT_DIR) {
            write_str("/");
          }
        }
        write_str("\n");
        print_prompt();
        if (len > 0) {
          write(1, buf, len);
        }
      }
      free(matches);
      continue;
    }
    if (len + 1 < size) {
      buf[len++] = c;
      buf[len] = '\0';
      write(1, &c, 1);
    }
  }
}

static int split_args(char *buf, char *argv[], int max) {
  int argc = 0;
  char *p = buf;
  while (*p) {
    while (*p && isspace((unsigned char)*p)) {
      *p++ = '\0';
    }
    if (!*p) {
      break;
    }
    if (argc + 1 >= max) {
      break;
    }
    argv[argc++] = p;
    while (*p && !isspace((unsigned char)*p)) {
      p++;
    }
  }
  argv[argc] = 0;
  return argc;
}

static void print_prompt(void) {
  char cwd[256];
  char path[256];
  const char *out = "?";
  if (getcwd(cwd, sizeof(cwd))) {
    if (strncmp(cwd, "/root", 5) == 0 && (cwd[5] == '\0' || cwd[5] == '/')) {
      path[0] = '~';
      strncpy(path + 1, cwd + 5, sizeof(path) - 2);
      path[sizeof(path) - 1] = '\0';
      out = path;
    } else {
      strncpy(path, cwd, sizeof(path) - 1);
      path[sizeof(path) - 1] = '\0';
      out = path;
    }
  }
  printf("root@qemu:%s$ ", out);
}

static void builtin_pwd(void) {
  char buf[256];
  if (!getcwd(buf, sizeof(buf))) {
    dprintf(2, "pwd: failed\n");
    return;
  }
  printf("%s\n", buf);
}

static void builtin_cd(int argc, char **argv) {
  const char *path = (argc < 2) ? "/" : argv[1];
  int ret = chdir(path);
  if (ret < 0) {
    dprintf(2, "cd: %s failed (%d)\n", path, ret);
  }
}

static void run_command(int argc, char **argv) {
  if (argc == 0) {
    return;
  }

  const char *redir_path = NULL;
  int redir_append = 0;
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0) {
      if (i + 1 >= argc) {
        dprintf(2, "sh: missing redirection path\n");
        return;
      }
      redir_path = argv[i + 1];
      redir_append = (argv[i][1] == '>');
      for (int j = i; j + 2 <= argc; j++) {
        argv[j] = argv[j + 2];
      }
      argc -= 2;
      argv[argc] = NULL;
      break;
    }
  }

  if (strcmp(argv[0], "cd") == 0) {
    builtin_cd(argc, argv);
    return;
  }
  if (strcmp(argv[0], "pwd") == 0) {
    if (redir_path) {
      int flags = O_CREAT | O_WRONLY | (redir_append ? O_APPEND : O_TRUNC);
      int fd = openat(AT_FDCWD, redir_path, flags);
      if (fd < 0) {
        dprintf(2, "sh: redirect %s failed (%d)\n", redir_path, fd);
        return;
      }
      int saved = dup(1);
      dup2(fd, 1);
      close(fd);
      builtin_pwd();
      if (saved >= 0) {
        dup2(saved, 1);
        close(saved);
      }
    } else {
      builtin_pwd();
    }
    return;
  }
  if (strcmp(argv[0], "exit") == 0) {
    int code = 0;
    if (argc > 1) {
      code = atoi(argv[1]);
    }
    tty_restore();
    exit(code);
  }

  const char *typed = argv[0];
  if (strcmp(argv[0], "vim") == 0) {
    argv[0] = "vi";
  } else if (strcmp(argv[0], "shutdown") == 0) {
    argv[0] = "poweroff";
  }

  char path[128];
  if (strchr(argv[0], '/')) {
    strncpy(path, argv[0], sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
  } else {
    const char *prefix = "/bin/";
    size_t plen = strlen(prefix);
    size_t clen = strlen(argv[0]);
    if (plen + clen + 1 > sizeof(path)) {
      dprintf(2, "sh: command too long\n");
      return;
    }
    memcpy(path, prefix, plen);
    memcpy(path + plen, argv[0], clen + 1);
  }

  tty_restore();
  pid_t pid = fork();
  if (pid < 0) {
    dprintf(2, "sh: fork failed (%d)\n", pid);
    tty_set_mode(1);
    return;
  }
  if (pid == 0) {
    if (redir_path) {
      int flags = O_CREAT | O_WRONLY | (redir_append ? O_APPEND : O_TRUNC);
      int fd = openat(AT_FDCWD, redir_path, flags);
      if (fd < 0) {
        dprintf(2, "sh: redirect %s failed (%d)\n", redir_path, fd);
        exit(1);
      }
      dup2(fd, 1);
      close(fd);
    }
    execve(path, argv, NULL);
    if (errno == ENOENT || errno == ENOSYS) {
      dprintf(2, "%s not implemented\n", typed);
    } else {
      dprintf(2, "sh: exec %s failed (%d)\n", path, errno);
    }
    exit(1);
  }
  wait(0);
  tty_set_mode(1);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  tty_set_mode(1);
  for (;;) {
    print_prompt();
    char line[MAX_LINE];
    int n = read_line(line, sizeof(line));
    if (n <= 0) {
      continue;
    }
    char *args[MAX_ARGS];
    int ac = split_args(line, args, MAX_ARGS);
    run_command(ac, args);
  }
  return 0;
}
