#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 32
#define MAX_INCS 8

static int copy_file(int outfd, const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    return fd;
  }
  char buf[512];
  for (;;) {
    int n = read(fd, buf, sizeof(buf));
    if (n < 0) {
      close(fd);
      return n;
    }
    if (n == 0) {
      break;
    }
    int off = 0;
    while (off < n) {
      int w = write(outfd, buf + off, (size_t)(n - off));
      if (w < 0) {
        close(fd);
        return w;
      }
      off += w;
    }
  }
  close(fd);
  return 0;
}

static void usage(void) {
  dprintf(2, "usage: tcc [-o output] [-I dir] [-L dir] [-l lib] <input.c>\n");
}

int main(int argc, char **argv) {
  const char *out = NULL;
  const char *input = NULL;
  const char *incs[MAX_INCS];
  int inc_count = 0;

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];
    if (strcmp(arg, "-o") == 0) {
      if (i + 1 >= argc) {
        usage();
        return 1;
      }
      out = argv[++i];
      continue;
    }
    if (strcmp(arg, "-I") == 0) {
      if (i + 1 >= argc) {
        usage();
        return 1;
      }
      if (inc_count < MAX_INCS) {
        incs[inc_count++] = argv[++i];
      } else {
        i++;
      }
      continue;
    }
    if (strncmp(arg, "-I", 2) == 0) {
      if (inc_count < MAX_INCS) {
        incs[inc_count++] = arg + 2;
      }
      continue;
    }
    if (strcmp(arg, "-L") == 0) {
      if (i + 1 < argc) {
        i++;
      }
      continue;
    }
    if (strncmp(arg, "-L", 2) == 0) {
      continue;
    }
    if (strcmp(arg, "-l") == 0) {
      if (i + 1 < argc) {
        i++;
      }
      continue;
    }
    if (strncmp(arg, "-l", 2) == 0) {
      continue;
    }
    if (arg[0] == '-') {
      dprintf(2, "tcc: unsupported option %s\n", arg);
      return 1;
    }
    if (!input) {
      input = arg;
    } else {
      dprintf(2, "tcc: only one input is supported\n");
      return 1;
    }
  }

  if (!input) {
    usage();
    return 1;
  }

  if (out) {
    int fd = open(out, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
      dprintf(2, "tcc: open %s failed (%d)\n", out, fd);
      return 1;
    }
    char shebang[256];
    const char *interp = "/bin/picoc";
    if (inc_count > 1) {
      dprintf(2, "tcc: warning: only the first -I is used for -o\n");
    }
    if (inc_count > 0) {
      int n = snprintf(shebang, sizeof(shebang), "#!%s -I%s\n", interp, incs[0]);
      if (n <= 0 || n >= (int)sizeof(shebang)) {
        close(fd);
        dprintf(2, "tcc: shebang too long\n");
        return 1;
      }
    } else {
      int n = snprintf(shebang, sizeof(shebang), "#!%s\n", interp);
      if (n <= 0 || n >= (int)sizeof(shebang)) {
        close(fd);
        return 1;
      }
    }
    if (write(fd, shebang, strlen(shebang)) < 0) {
      close(fd);
      return 1;
    }
    int ret = copy_file(fd, input);
    close(fd);
    if (ret < 0) {
      dprintf(2, "tcc: copy %s failed (%d)\n", input, ret);
      return 1;
    }
    return 0;
  }

  char *exec_argv[MAX_ARGS];
  int ac = 0;
  exec_argv[ac++] = "/bin/picoc";
  for (int i = 0; i < inc_count && ac + 2 < MAX_ARGS; i++) {
    exec_argv[ac++] = "-I";
    exec_argv[ac++] = (char *)incs[i];
  }
  exec_argv[ac++] = (char *)input;
  exec_argv[ac] = NULL;

  execve("/bin/picoc", exec_argv, NULL);
  dprintf(2, "tcc: exec /bin/picoc failed (errno=%d)\n", errno);
  return 1;
}
