#pragma once

#include <sys/types.h>

struct group {
  char *gr_name;
  int gr_gid;
};

struct group *getgrgid(int gid);
struct group *getgrnam(const char *name);
int initgroups(const char *user, gid_t group);
void endgrent(void);
