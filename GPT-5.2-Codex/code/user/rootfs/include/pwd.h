#pragma once

struct passwd {
  char *pw_name;
  char *pw_dir;
  char *pw_shell;
  int pw_uid;
  int pw_gid;
};

struct passwd *getpwuid(int uid);
struct passwd *getpwnam(const char *name);
