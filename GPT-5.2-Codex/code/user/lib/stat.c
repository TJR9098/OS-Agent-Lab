#include <sys/stat.h>
#include <unistd.h>

int stat(const char *path, struct stat *st) {
  return fstatat(AT_FDCWD, path, st);
}
