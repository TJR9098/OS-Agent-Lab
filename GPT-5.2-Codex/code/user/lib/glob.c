#include <glob.h>

int glob(const char *pattern, int flags, int (*errfunc)(const char *, int), glob_t *pglob) {
  (void)pattern;
  (void)flags;
  (void)errfunc;
  if (pglob) {
    pglob->gl_pathc = 0;
    pglob->gl_pathv = 0;
    pglob->gl_offs = 0;
  }
  return GLOB_NOMATCH;
}

void globfree(glob_t *pglob) {
  (void)pglob;
}
