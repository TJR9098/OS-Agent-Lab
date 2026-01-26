#include <pwd.h>

static struct passwd g_pw = {
    .pw_name = "root",
    .pw_dir = "/root",
    .pw_shell = "/bin/sh",
    .pw_uid = 0,
    .pw_gid = 0,
};

struct passwd *getpwuid(int uid) {
  (void)uid;
  return &g_pw;
}

struct passwd *getpwnam(const char *name) {
  (void)name;
  return &g_pw;
}
