#include <string.h>
#include <unistd.h>

int optind = 1;
int optopt = 0;
int opterr = 1;
char *optarg = NULL;

int getopt(int argc, char *const argv[], const char *optstring) {
  static int optpos = 0;
  if (optind >= argc || !argv[optind]) {
    return -1;
  }
  const char *arg = argv[optind];
  if (arg[0] != '-' || arg[1] == '\0') {
    return -1;
  }
  if (strcmp(arg, "--") == 0) {
    optind++;
    return -1;
  }
  if (optpos == 0) {
    optpos = 1;
  }
  char c = arg[optpos];
  const char *opt = strchr(optstring, c);
  if (!opt) {
    optopt = (unsigned char)c;
    if (arg[++optpos] == '\0') {
      optind++;
      optpos = 0;
    }
    return '?';
  }
  if (opt[1] == ':') {
    if (arg[optpos + 1] != '\0') {
      optarg = (char *)&arg[optpos + 1];
      optind++;
      optpos = 0;
    } else if (optind + 1 < argc) {
      optarg = argv[optind + 1];
      optind += 2;
      optpos = 0;
    } else {
      optopt = (unsigned char)c;
      optind++;
      optpos = 0;
      return '?';
    }
  } else {
    optarg = NULL;
    if (arg[++optpos] == '\0') {
      optind++;
      optpos = 0;
    }
  }
  return c;
}
