#include <stdint.h>

#include "bootinfo.h"
#include "blk.h"
#include "fat32.h"
#include "log.h"
#include "mm.h"
#include "proc.h"
#include "ramfs.h"
#include "riscv.h"
#include "selftest.h"
#include "string.h"
#include "timer.h"
#include "trap.h"
#include "vfs.h"
#include "uart.h"

void kernel_main(uint64_t hartid, uintptr_t dtb, struct bootinfo *bi) {
  uart_init();

  if (bi && bi->magic == BOOTINFO_MAGIC) {
    log_set_level(bi->log_level);
  }

  write_tp(hartid);

  log_info("kernel: boot hart=%lu dtb=%p bootinfo=%p", hartid, (void *)dtb, bi);

  if (bi && bi->magic == BOOTINFO_MAGIC) {
    log_info("mem: base=0x%lx size=0x%lx", bi->mem_base, bi->mem_size);
  }

  if (bi && bi->magic == BOOTINFO_MAGIC) {
    kinit(bi->mem_base, bi->mem_size);
    log_info("mem: kinit done");
  }

#ifdef CONFIG_STAGE3_DEBUG_BOOT
  log_info("boot: before trap_init");
#endif
  trap_init();
#ifdef CONFIG_STAGE3_DEBUG_BOOT
  log_info("boot: after trap_init");
#endif
  timer_init(hartid, bi);
#ifdef CONFIG_STAGE3_DEBUG_BOOT
  log_info("boot: after timer_init");
#endif
  trap_enable();
#ifdef CONFIG_STAGE3_DEBUG_BOOT
  log_info("boot: after trap_enable");
#endif

  struct vnode *root = NULL;
  {
    uint32_t root_index = 1;
    if (bi && bi->magic == BOOTINFO_MAGIC && bi->version >= 3) {
      root_index = bi->root_disk_index;
    }
    int blk_ok = blk_init((const void *)dtb, root_index);
    if (blk_ok == 0) {
#ifdef CONFIG_STAGE3_DEBUG_BLK
      uint8_t pattern[512];
      for (size_t i = 0; i < sizeof(pattern); i++) {
        pattern[i] = (uint8_t)(0x5aU ^ (uint8_t)i);
      }
      log_info("blk: write selftest start");
      if (blk_write(1, pattern, 1) == 0) {
        log_info("blk: write selftest pass");
      } else {
        log_error("blk: write selftest fail");
      }
#endif
#ifdef CONFIG_STAGE2_TEST
#ifdef CONFIG_STAGE2_TEST_BLK
      selftest_run_blk();
#endif
#endif
    }
#if !defined(CONFIG_STAGE2_TEST) || defined(CONFIG_STAGE2_TEST_FS)
    if (blk_ok == 0) {
      root = fat32_init();
      if (root) {
        vfs_set_root(root);
        vfs_set_cwd(root);

        struct vnode *ram_root = ramfs_init();
        if (ram_root) {
          struct vnode *mp = NULL;
          if (vfs_lookup("/ram", &mp) == 0) {
            vfs_mount(mp, ram_root);
          }
        }

#ifdef CONFIG_STAGE2_TEST
#ifdef CONFIG_STAGE2_TEST_FS
        selftest_run_fs();
#endif
#endif

        struct vfs_dirent ent;
        uint64_t idx = 0;
        log_info("vfs: root directory");
        while (vfs_readdir(root, idx, &ent) == 0) {
          log_info("vfs: %s%s", ent.name, (ent.type == VNODE_DIR) ? "/" : "");
          idx++;
        }
      }
    }
#endif
  }

#ifdef CONFIG_STAGE3_DEBUG_FS
  if (root) {
    log_info("vfs: mkdir /test start");
    int ret = vfs_mkdir("/test", NULL);
    log_info("vfs: mkdir /test ret=%d", ret);
  }
#endif

  proc_init();
  if (root) {
    struct proc *init = proc_alloc();
    if (init) {
      char *argv[] = {"sh", NULL};
      int ret = proc_exec_on(init, "/bin/sh", argv);
      if (ret == 0) {
        init->state = PROC_RUNNABLE;
        log_info("init: exec /bin/sh");
        scheduler();
      } else {
        log_error("init: exec /bin/sh failed (%d)", ret);
      }
    } else {
      log_error("init: alloc failed");
    }
  }

  for (;;) {
    __asm__ volatile("wfi");
  }
}
