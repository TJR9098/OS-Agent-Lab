#include <libgen.h>
#include <string.h>

char *basename(char *path) {
  if (!path || !*path) {
    return ".";
  }
  size_t len = strlen(path);
  while (len > 0 && path[len - 1] == '/') {
    path[len - 1] = '\0';
    len--;
  }
  if (len == 0) {
    return "/";
  }
  char *p = strrchr(path, '/');
  return p ? (p + 1) : path;
}

char *dirname(char *path) {
  if (!path || !*path) {
    return ".";
  }
  size_t len = strlen(path);
  while (len > 0 && path[len - 1] == '/') {
    path[len - 1] = '\0';
    len--;
  }
  if (len == 0) {
    return "/";
  }
  char *p = strrchr(path, '/');
  if (!p) {
    return ".";
  }
  if (p == path) {
    path[1] = '\0';
    return path;
  }
  *p = '\0';
  return path;
}
