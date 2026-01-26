#include <sys/mman.h>
#include <stdlib.h>
#include <errno.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
  (void)addr;
  (void)prot;
  (void)fd;
  (void)offset;
  if ((flags & MAP_ANON) == 0) {
    errno = ENOSYS;
    return MAP_FAILED;
  }
  size_t total = length + sizeof(size_t);
  size_t *base = (size_t *)malloc(total);
  if (!base) {
    errno = ENOMEM;
    return MAP_FAILED;
  }
  *base = length;
  return (void *)(base + 1);
}

int munmap(void *addr, size_t length) {
  (void)length;
  if (!addr) {
    errno = EINVAL;
    return -1;
  }
  size_t *base = ((size_t *)addr) - 1;
  free(base);
  return 0;
}
