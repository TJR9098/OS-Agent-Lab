#include <fnmatch.h>

static int match_here(const char *pattern, const char *str) {
  while (*pattern) {
    if (*pattern == '*') {
      while (*pattern == '*') {
        pattern++;
      }
      if (*pattern == '\0') {
        return 1;
      }
      for (; *str; str++) {
        if (match_here(pattern, str)) {
          return 1;
        }
      }
      return 0;
    }
    if (*pattern == '?') {
      if (*str == '\0') {
        return 0;
      }
      pattern++;
      str++;
      continue;
    }
    if (*pattern != *str) {
      return 0;
    }
    pattern++;
    str++;
  }
  return *str == '\0';
}

int fnmatch(const char *pattern, const char *string, int flags) {
  (void)flags;
  if (!pattern || !string) {
    return FNM_NOMATCH;
  }
  return match_here(pattern, string) ? 0 : FNM_NOMATCH;
}
