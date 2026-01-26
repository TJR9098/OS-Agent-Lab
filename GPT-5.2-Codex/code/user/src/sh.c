#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 256
#define MAX_ARGS 16

static int read_line(char *buf, size_t size) {
  int n = (int)read(0, buf, size - 1);
  if (n <= 0) {
    return -1;
  }
  buf[n] = '\0';
  if (n > 0 && buf[n - 1] == '\n') {
    buf[n - 1] = '\0';
  }
  return n;
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

  if (strcmp(argv[0], "cd") == 0) {
    builtin_cd(argc, argv);
    return;
  }
  if (strcmp(argv[0], "pwd") == 0) {
    builtin_pwd();
    return;
  }
  if (strcmp(argv[0], "exit") == 0) {
    int code = 0;
    if (argc > 1) {
      code = atoi(argv[1]);
    }
    exit(code);
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

  pid_t pid = fork();
  if (pid < 0) {
    dprintf(2, "sh: fork failed (%d)\n", pid);
    return;
  }
  if (pid == 0) {
    execve(path, argv, NULL);
    dprintf(2, "sh: exec %s failed\n", path);
    exit(1);
  }
  wait(0);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
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
