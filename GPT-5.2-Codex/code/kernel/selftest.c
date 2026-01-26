#include <stdint.h>

#include "blk.h"
#include "errno.h"
#include "log.h"
#include "selftest.h"
#include "string.h"
#include "vfs.h"

#ifdef CONFIG_STAGE2_TEST

static int selftest_expect(int cond, const char *msg) {
  if (!cond) {
    log_error("selftest: %s", msg);
    return -1;
  }
  return 0;
}

static int selftest_blk_rw(void) {
  uint8_t orig[512];
  uint8_t pattern[512];
  uint8_t readback[512];

  if (blk_read(1, orig, 1) != 0) {
    log_error("selftest: blk read failed");
    return -1;
  }
  for (size_t i = 0; i < sizeof(pattern); i++) {
    pattern[i] = (uint8_t)(0xA5U ^ (uint8_t)i);
  }
  if (blk_write(1, pattern, 1) != 0) {
    log_error("selftest: blk write failed");
    return -1;
  }
  if (blk_read(1, readback, 1) != 0) {
    log_error("selftest: blk readback failed");
    return -1;
  }
  if (memcmp(pattern, readback, sizeof(pattern)) != 0) {
    log_error("selftest: blk mismatch");
    return -1;
  }
  if (blk_write(1, orig, 1) != 0) {
    log_error("selftest: blk restore failed");
    return -1;
  }
  return 0;
}

static int selftest_vfs_basic(void) {
  struct vfs_dirent ent;
  struct vnode *vn = NULL;
  char cwd[128];

  if (selftest_expect(vfs_readdir(vfs_root(), 0, &ent) == 0 &&
                          strcmp(ent.name, ".") == 0,
                      "vfs readdir dot") != 0) {
    return -1;
  }
  if (selftest_expect(vfs_readdir(vfs_root(), 1, &ent) == 0 &&
                          strcmp(ent.name, "..") == 0,
                      "vfs readdir dotdot") != 0) {
    return -1;
  }

  if (selftest_expect(vfs_mkdir("/TestDir/Sub", NULL) == 0, "vfs mkdir nested") != 0) {
    return -1;
  }
  if (selftest_expect(vfs_lookup("/TestDir", &vn) == 0 && vn && vn->type == VNODE_DIR,
                      "vfs lookup dir") != 0) {
    return -1;
  }
  vfs_set_cwd(vn);
  if (selftest_expect(vfs_getcwd(cwd, sizeof(cwd)) == 0 && strcmp(cwd, "/TestDir") == 0,
                      "vfs getcwd") != 0) {
    return -1;
  }
  vfs_set_cwd(vfs_root());

  if (selftest_expect(vfs_create("/AutoDir/Sub/auto.txt", &vn) == 0, "vfs create auto parents") != 0) {
    return -1;
  }
  if (selftest_expect(vfs_lookup("/AutoDir/Sub", &vn) == 0, "vfs lookup auto parents") != 0) {
    return -1;
  }
  if (selftest_expect(vfs_unlink("/AutoDir/Sub/auto.txt") == 0, "vfs unlink auto file") != 0) {
    return -1;
  }
  if (selftest_expect(vfs_unlink("/AutoDir/Sub") == 0, "vfs unlink auto dir") != 0) {
    return -1;
  }
  if (selftest_expect(vfs_unlink("/AutoDir") == 0, "vfs unlink auto parent") != 0) {
    return -1;
  }

  return 0;
}

static int selftest_fat32_ops(void) {
  struct vnode *vn = NULL;
  struct vnode *vn2 = NULL;
  struct vfs_stat st;
  const char *path = "/TestDir/Sub/LongFileName123.txt";
  const char *path_case = "/testdir/sub/longfilename123.TXT";
  const char *data = "fat32-write-test";
  char buf[64];
  int ret;

  ret = vfs_create(path, &vn);
  if (selftest_expect(ret == 0, "fat32 create lfn") != 0) {
    return -1;
  }
  ret = vfs_lookup(path_case, &vn2);
  if (selftest_expect(ret == 0 && vn2 != NULL, "fat32 lookup nocase") != 0) {
    return -1;
  }
  ret = vfs_write(vn, data, strlen(data), 0);
  if (selftest_expect(ret == (int)strlen(data), "fat32 write") != 0) {
    return -1;
  }
  memset(buf, 0, sizeof(buf));
  ret = vfs_read(vn2, buf, sizeof(buf), 0);
  if (ret != (int)strlen(data) || memcmp(buf, data, strlen(data)) != 0) {
    log_error("selftest: fat32 readback ret=%d buf=\"%s\"", ret, buf);
    return -1;
  }
  ret = vfs_truncate(vn, 0);
  if (selftest_expect(ret == 0, "fat32 truncate") != 0) {
    return -1;
  }
  if (selftest_expect(vfs_stat(vn, &st) == 0 && st.st_size == 0, "fat32 stat size") != 0) {
    return -1;
  }
  ret = vfs_unlink("/TestDir/Sub");
  if (selftest_expect(ret == -ENOTEMPTY, "fat32 unlink non-empty dir") != 0) {
    return -1;
  }
  ret = vfs_unlink(path);
  if (selftest_expect(ret == 0, "fat32 unlink file") != 0) {
    return -1;
  }
  ret = vfs_unlink("/TestDir/Sub");
  if (selftest_expect(ret == 0, "fat32 unlink empty dir") != 0) {
    return -1;
  }
  ret = vfs_unlink("/TestDir");
  if (selftest_expect(ret == 0, "fat32 unlink parent dir") != 0) {
    return -1;
  }

  return 0;
}

static int selftest_ramfs_ops(void) {
  struct vnode *vn = NULL;
  struct vnode *vn2 = NULL;
  const char *path = "/ram/RTest/Note.txt";
  const char *path_case = "/ram/rtest/note.TXT";
  const char *data = "ramfs-data";
  char buf[32];
  int ret;

  if (selftest_expect(vfs_mkdir("/ram/RTest", NULL) == 0, "ramfs mkdir") != 0) {
    return -1;
  }
  ret = vfs_create(path, &vn);
  if (selftest_expect(ret == 0, "ramfs create") != 0) {
    return -1;
  }
  ret = vfs_lookup(path_case, &vn2);
  if (selftest_expect(ret == 0 && vn2, "ramfs lookup nocase") != 0) {
    return -1;
  }
  ret = vfs_write(vn, data, strlen(data), 0);
  if (selftest_expect(ret == (int)strlen(data), "ramfs write") != 0) {
    return -1;
  }
  memset(buf, 0, sizeof(buf));
  ret = vfs_read(vn2, buf, sizeof(buf), 0);
  if (ret != (int)strlen(data) || memcmp(buf, data, strlen(data)) != 0) {
    log_error("selftest: ramfs readback ret=%d buf=\"%s\"", ret, buf);
    return -1;
  }
  ret = vfs_unlink("/ram/RTest");
  if (selftest_expect(ret == -ENOTEMPTY, "ramfs unlink non-empty dir") != 0) {
    return -1;
  }
  ret = vfs_unlink(path);
  if (selftest_expect(ret == 0, "ramfs unlink file") != 0) {
    return -1;
  }
  ret = vfs_unlink("/ram/RTest");
  if (selftest_expect(ret == 0, "ramfs unlink dir") != 0) {
    return -1;
  }
  return 0;
}

void selftest_run_blk(void) {
  log_info("selftest: blk rw start");
  if (selftest_blk_rw() == 0) {
    log_info("selftest: blk rw pass");
  } else {
    log_error("selftest: blk rw fail");
  }
}

void selftest_run_fs(void) {
  log_info("selftest: vfs basic start");
  if (selftest_vfs_basic() == 0) {
    log_info("selftest: vfs basic pass");
  } else {
    log_error("selftest: vfs basic fail");
  }

  log_info("selftest: fat32 ops start");
  if (selftest_fat32_ops() == 0) {
    log_info("selftest: fat32 ops pass");
  } else {
    log_error("selftest: fat32 ops fail");
  }

  log_info("selftest: ramfs ops start");
  if (selftest_ramfs_ops() == 0) {
    log_info("selftest: ramfs ops pass");
  } else {
    log_error("selftest: ramfs ops fail");
  }
}

#else

void selftest_run_blk(void) {
}

void selftest_run_fs(void) {
}

#endif
