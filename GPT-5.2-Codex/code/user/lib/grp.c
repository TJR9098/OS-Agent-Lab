#include <grp.h>

static struct group g_grp = {
    .gr_name = "root",
    .gr_gid = 0,
};

struct group *getgrgid(int gid) {
  (void)gid;
  return &g_grp;
}

struct group *getgrnam(const char *name) {
  (void)name;
  return &g_grp;
}

int initgroups(const char *user, gid_t group) {
  (void)user;
  (void)group;
  return 0;
}

void endgrent(void) {
}
