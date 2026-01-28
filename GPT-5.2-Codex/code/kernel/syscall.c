#include <stdint.h>
#include <string.h>

#include "errno.h"
#include "file.h"
#include "log.h"
#include "memlayout.h"
#include "proc.h"
#include "riscv.h"
#include "sbi.h"
#include "syscall.h"
#include "timer.h"
#include "uart.h"
#include "vfs.h"
#include "vm.h"

#define AT_FDCWD (-100)

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x40
#define O_TRUNC 0x200
#define O_APPEND 0x400
#define O_DIRECTORY 0x10000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define SYS_IOCTL 29
#define SYS_DUP 23
#define SYS_DUP3 24
#define SYS_LSEEK 62
#define SYS_POLL 7

#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSETSW 0x5403
#define TCSETSF 0x5404
#define TIOCGWINSZ 0x5413
#define ICANON 0x0002
#define ECHO 0x0008

#define POLLIN 0x0001

#define MAX_POLL_FDS 16

#define MAX_ARG 16
#define ARG_MAX_LEN 128

struct linux_dirent64 {
  uint64_t d_ino;
  int64_t d_off;
  uint16_t d_reclen;
  uint8_t d_type;
  uint8_t d_name[0];
};

struct termios {
  uint32_t c_iflag;
  uint32_t c_oflag;
  uint32_t c_cflag;
  uint32_t c_lflag;
  uint8_t c_cc[32];
  uint32_t c_ispeed;
  uint32_t c_ospeed;
};

struct winsize {
  uint16_t ws_row;
  uint16_t ws_col;
  uint16_t ws_xpixel;
  uint16_t ws_ypixel;
};

struct pollfd {
  int fd;
  short events;
  short revents;
};

static uint32_t g_tty_lflag = ICANON | ECHO;

static int fdalloc(struct proc *p, struct file *f) {
  for (int i = 3; i < NOFILE; i++) {
    if (!p->ofile[i]) {
      p->ofile[i] = f;
      return i;
    }
  }
  return -EMFILE;
}

static int sys_write(uint64_t fd, uint64_t buf, uint64_t len) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  if (fd < NOFILE && p->ofile[fd]) {
    uint8_t tmp[256];
    uint64_t left = len;
    uint64_t off = 0;
    int total = 0;
    while (left > 0) {
      uint64_t chunk = left > sizeof(tmp) ? sizeof(tmp) : left;
      if (copyin(p->pagetable, tmp, buf + off, (size_t)chunk) != 0) {
        return -EFAULT;
      }
      int n = file_write(p->ofile[fd], tmp, (size_t)chunk);
      if (n < 0) {
        return n;
      }
      total += n;
      if (n < (int)chunk) {
        break;
      }
      left -= chunk;
      off += chunk;
    }
    return total;
  }
  if (fd == 1 || fd == 2) {
    uint8_t tmp[128];
    uint64_t left = len;
    uint64_t off = 0;
    while (left > 0) {
      uint64_t chunk = left > sizeof(tmp) ? sizeof(tmp) : left;
      if (copyin(p->pagetable, tmp, buf + off, (size_t)chunk) != 0) {
        return -EFAULT;
      }
      for (uint64_t i = 0; i < chunk; i++) {
        uart_putc((char)tmp[i]);
      }
      left -= chunk;
      off += chunk;
    }
    return (int)len;
  }
  if (fd >= NOFILE || !p->ofile[fd]) {
    return -EBADF;
  }
  return -EBADF;
}

static int sys_read(uint64_t fd, uint64_t buf, uint64_t len) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  if (fd < NOFILE && p->ofile[fd]) {
    uint8_t tmp[256];
    uint64_t left = len;
    uint64_t off = 0;
    int total = 0;
    while (left > 0) {
      uint64_t chunk = left > sizeof(tmp) ? sizeof(tmp) : left;
      int n = file_read(p->ofile[fd], tmp, (size_t)chunk);
      if (n < 0) {
        return n;
      }
      if (n == 0) {
        break;
      }
      if (copyout(p->pagetable, buf + off, tmp, (size_t)n) != 0) {
        return -EFAULT;
      }
      total += n;
      left -= n;
      off += n;
    }
    return total;
  }
  if (fd == 0) {
    uint8_t tmp[128];
    uint64_t n = 0;
    if (g_tty_lflag & ICANON) {
      while (n < len && n < sizeof(tmp)) {
        int c;
        while ((c = uart_getc()) < 0) {
        }
        if (c == '\r') {
          c = '\n';
        }
        if ((c == 0x7f || c == 0x08) && (g_tty_lflag & ECHO)) {
          if (n > 0) {
            n--;
            uart_putc('\b');
            uart_putc(' ');
            uart_putc('\b');
          }
          continue;
        }
        if (g_tty_lflag & ECHO) {
          uart_putc((char)c);
        }
        if (c == 0x7f || c == 0x08) {
          if (n > 0) {
            n--;
          }
          continue;
        }
        tmp[n++] = (uint8_t)c;
        if (c == '\n') {
          break;
        }
      }
    } else {
      if (len > 0) {
        int c;
        while ((c = uart_getc()) < 0) {
        }
        if (c == '\r') {
          c = '\n';
        }
        if (g_tty_lflag & ECHO) {
          uart_putc((char)c);
        }
        tmp[n++] = (uint8_t)c;
      }
    }
    if (copyout(p->pagetable, buf, tmp, (size_t)n) != 0) {
      return -EFAULT;
    }
    return (int)n;
  }
  if (fd >= NOFILE || !p->ofile[fd]) {
    return -EBADF;
  }
  return -EBADF;
}

static int sys_close(uint64_t fd) {
  struct proc *p = proc_current();
  if (!p || fd >= NOFILE || !p->ofile[fd]) {
    return -EBADF;
  }
  file_close(p->ofile[fd]);
  p->ofile[fd] = NULL;
  return 0;
}

static int sys_dup(uint64_t fd) {
  struct proc *p = proc_current();
  if (!p || fd >= NOFILE || !p->ofile[fd]) {
    return -EBADF;
  }
  struct file *f = file_dup(p->ofile[fd]);
  if (!f) {
    return -EBADF;
  }
  int newfd = fdalloc(p, f);
  if (newfd < 0) {
    file_close(f);
    return newfd;
  }
  return newfd;
}

static int sys_dup3(uint64_t oldfd, uint64_t newfd, uint64_t flags) {
  (void)flags;
  struct proc *p = proc_current();
  if (!p || oldfd >= NOFILE || newfd >= NOFILE) {
    return -EBADF;
  }
  if (!p->ofile[oldfd]) {
    return -EBADF;
  }
  if (oldfd == newfd) {
    return (int)newfd;
  }
  if (p->ofile[newfd]) {
    file_close(p->ofile[newfd]);
    p->ofile[newfd] = NULL;
  }
  struct file *f = file_dup(p->ofile[oldfd]);
  if (!f) {
    return -EBADF;
  }
  p->ofile[newfd] = f;
  return (int)newfd;
}

static int64_t sys_lseek(uint64_t fd, int64_t offset, uint64_t whence) {
  struct proc *p = proc_current();
  if (!p || fd >= NOFILE || !p->ofile[fd]) {
    return -EBADF;
  }
  struct file *f = p->ofile[fd];
  int64_t base = 0;
  if (whence == SEEK_SET) {
    base = 0;
  } else if (whence == SEEK_CUR) {
    base = (int64_t)f->off;
  } else if (whence == SEEK_END) {
    struct vfs_stat st;
    int ret = vfs_stat(f->vn, &st);
    if (ret != 0) {
      return ret;
    }
    base = st.st_size;
  } else {
    return -EINVAL;
  }
  int64_t next = base + offset;
  if (next < 0) {
    return -EINVAL;
  }
  f->off = (uint64_t)next;
  return next;
}

static int sys_ioctl(uint64_t fd, uint64_t req, uint64_t arg) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  if (fd > 2) {
    return -ENOTTY;
  }
  if (req == TCGETS) {
    struct termios term;
    memset(&term, 0, sizeof(term));
    term.c_lflag = g_tty_lflag;
    if (copyout(p->pagetable, arg, &term, sizeof(term)) != 0) {
      return -EFAULT;
    }
    return 0;
  }
  if (req == TCSETS || req == TCSETSW || req == TCSETSF) {
    struct termios term;
    if (copyin(p->pagetable, &term, arg, sizeof(term)) != 0) {
      return -EFAULT;
    }
    g_tty_lflag = term.c_lflag;
    return 0;
  }
  if (req == TIOCGWINSZ) {
    struct winsize ws;
    ws.ws_row = 24;
    ws.ws_col = 80;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;
    if (copyout(p->pagetable, arg, &ws, sizeof(ws)) != 0) {
      return -EFAULT;
    }
    return 0;
  }
  return -ENOTTY;
}

static int sys_openat(uint64_t dirfd, uint64_t path_ptr, uint64_t flags) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  if ((int64_t)dirfd != AT_FDCWD) {
    return -ENOSYS;
  }
  char path[256];
  if (copyinstr(p->pagetable, path, path_ptr, sizeof(path)) != 0) {
    return -EFAULT;
  }
  if ((flags & O_TRUNC) && ((flags & (O_WRONLY | O_RDWR)) == 0)) {
    return -EINVAL;
  }

  struct vnode *vn = NULL;
  int ret;
  if (flags & O_CREAT) {
    ret = vfs_create_at(p->cwd, path, &vn);
  } else {
    ret = vfs_lookup_at(p->cwd, path, &vn);
  }
  if (ret != 0) {
    return ret;
  }
  if (vn->type == VNODE_DIR) {
    if (flags & (O_WRONLY | O_RDWR | O_TRUNC)) {
      return -EISDIR;
    }
  } else if (flags & O_DIRECTORY) {
    return -ENOTDIR;
  }
  if ((flags & O_TRUNC) && vn->type == VNODE_FILE) {
    ret = vfs_truncate(vn, 0);
    if (ret != 0) {
      return ret;
    }
  }

  struct file *f = file_alloc();
  if (!f) {
    return -EMFILE;
  }
  f->vn = vn;
  f->readable = (flags & O_WRONLY) == 0;
  f->writable = (flags & O_WRONLY) || (flags & O_RDWR);
  f->off = 0;
  if (flags & O_APPEND) {
    struct vfs_stat st;
    ret = vfs_stat(vn, &st);
    if (ret != 0) {
      file_close(f);
      return ret;
    }
    f->off = (uint64_t)st.st_size;
  }
  int fd = fdalloc(p, f);
  if (fd < 0) {
    file_close(f);
    return fd;
  }
  return fd;
}

static int sys_getdents64(uint64_t fd, uint64_t dirp, uint64_t count) {
  struct proc *p = proc_current();
  if (!p || fd >= NOFILE || !p->ofile[fd]) {
    return -EBADF;
  }
  struct file *f = p->ofile[fd];
  if (!f->vn || f->vn->type != VNODE_DIR) {
    return -ENOTDIR;
  }

  uint64_t off = f->off;
  uint64_t written = 0;
  struct vfs_dirent ent;

  while (written + sizeof(struct linux_dirent64) + 1 < count) {
    int ret = vfs_readdir(f->vn, off, &ent);
    if (ret != 0) {
      break;
    }
    size_t name_len = strlen(ent.name);
    size_t reclen = sizeof(struct linux_dirent64) + name_len + 1;
    if (reclen & 7) {
      reclen = (reclen + 7) & ~7U;
    }
    if (written + reclen > count) {
      break;
    }

    uint8_t tmp[256];
    if (reclen > sizeof(tmp)) {
      return -EINVAL;
    }
    struct linux_dirent64 *d = (struct linux_dirent64 *)tmp;
    d->d_ino = ent.ino;
    d->d_off = (int64_t)(off + 1);
    d->d_reclen = (uint16_t)reclen;
    d->d_type = (ent.type == VNODE_DIR) ? 4 : 8;
    memcpy(d->d_name, ent.name, name_len + 1);
    if (copyout(p->pagetable, dirp + written, tmp, reclen) != 0) {
      return -EFAULT;
    }
    written += reclen;
    off++;
  }

  f->off = off;
  return (int)written;
}

static int sys_mkdirat(uint64_t dirfd, uint64_t path_ptr) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
#ifdef CONFIG_STAGE3_DEBUG_SYSCALL
  log_info("sys_mkdirat: enter");
#endif
  if ((int64_t)dirfd != AT_FDCWD) {
    return -ENOSYS;
  }
  char path[256];
  if (copyinstr(p->pagetable, path, path_ptr, sizeof(path)) != 0) {
    return -EFAULT;
  }
#ifdef CONFIG_STAGE3_DEBUG_SYSCALL
  log_info("sys_mkdirat: enter path=%s", path);
#endif
  int ret = vfs_mkdir_at(p->cwd, path, NULL);
#ifdef CONFIG_STAGE3_DEBUG_SYSCALL
  log_info("sys_mkdirat: path=%s ret=%d", path, ret);
#endif
  return ret;
}

static int sys_unlinkat(uint64_t dirfd, uint64_t path_ptr) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  if ((int64_t)dirfd != AT_FDCWD) {
    return -ENOSYS;
  }
  char path[256];
  if (copyinstr(p->pagetable, path, path_ptr, sizeof(path)) != 0) {
    return -EFAULT;
  }
  return vfs_unlink_at(p->cwd, path);
}

static int sys_fstat(uint64_t fd, uint64_t stat_ptr) {
  struct proc *p = proc_current();
  if (!p || fd >= NOFILE || !p->ofile[fd]) {
    return -EBADF;
  }
  struct vfs_stat st;
  int ret = file_stat(p->ofile[fd], &st);
  if (ret != 0) {
    return ret;
  }
  if (copyout(p->pagetable, stat_ptr, &st, sizeof(st)) != 0) {
    return -EFAULT;
  }
  return 0;
}

static int sys_ftruncate(uint64_t fd, int64_t length) {
  struct proc *p = proc_current();
  if (!p || fd >= NOFILE || !p->ofile[fd]) {
    return -EBADF;
  }
  if (length < 0) {
    return -EINVAL;
  }
  return file_truncate(p->ofile[fd], (uint64_t)length);
}

static int sys_fstatat(uint64_t dirfd, uint64_t path_ptr, uint64_t stat_ptr) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  if ((int64_t)dirfd != AT_FDCWD) {
    return -ENOSYS;
  }
  char path[256];
  if (copyinstr(p->pagetable, path, path_ptr, sizeof(path)) != 0) {
    return -EFAULT;
  }
  struct vnode *vn = NULL;
  int ret = vfs_lookup_at(p->cwd, path, &vn);
  if (ret != 0) {
    return ret;
  }
  struct vfs_stat st;
  ret = vfs_stat(vn, &st);
  if (ret != 0) {
    return ret;
  }
  if (copyout(p->pagetable, stat_ptr, &st, sizeof(st)) != 0) {
    return -EFAULT;
  }
  return 0;
}

static int poll_check(struct proc *p, struct pollfd *fds, uint64_t nfds) {
  int ready = 0;
  for (uint64_t i = 0; i < nfds; i++) {
    fds[i].revents = 0;
    if (fds[i].events & POLLIN) {
      int fd = fds[i].fd;
      if (fd == 0) {
        if (uart_rx_ready()) {
          fds[i].revents |= POLLIN;
        }
      } else if (fd >= 0 && fd < NOFILE) {
        if (p->ofile[fd]) {
          fds[i].revents |= POLLIN;
        } else {
          return -EBADF;
        }
      } else if (fd < 0) {
        return -EBADF;
      }
    }
    if (fds[i].revents) {
      ready++;
    }
  }
  return ready;
}

static int sys_poll(uint64_t fds_ptr, uint64_t nfds, int64_t timeout_ms) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  if (nfds > MAX_POLL_FDS) {
    return -EINVAL;
  }
  struct pollfd fds[MAX_POLL_FDS];
  size_t bytes = (size_t)nfds * sizeof(struct pollfd);
  if (bytes > 0 && copyin(p->pagetable, fds, fds_ptr, bytes) != 0) {
    return -EFAULT;
  }

  int ready = 0;
  if (timeout_ms == 0) {
    int ret = poll_check(p, fds, nfds);
    if (ret < 0) {
      return ret;
    }
    ready = ret;
  } else {
    uint64_t timebase = timer_timebase_hz();
    if (timebase == 0) {
      timebase = 10000000ULL;
    }
    uint64_t start = read_csr(time);
    uint64_t deadline = 0;
    if (timeout_ms > 0) {
      uint64_t ticks = ((uint64_t)timeout_ms * timebase) / 1000;
      deadline = start + ticks;
    }
    for (;;) {
      int ret = poll_check(p, fds, nfds);
      if (ret < 0) {
        return ret;
      }
      ready = ret;
      if (ready > 0) {
        break;
      }
      if (timeout_ms > 0) {
        uint64_t now = read_csr(time);
        if (now >= deadline) {
          break;
        }
      } else if (timeout_ms == 0) {
        break;
      }
    }
  }

  if (bytes > 0 && copyout(p->pagetable, fds_ptr, fds, bytes) != 0) {
    return -EFAULT;
  }
  return ready;
}

static int sys_getpid(void) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  return p->pid;
}

static int sys_getppid(void) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  if (!p->parent) {
    return 0;
  }
  return p->parent->pid;
}

static int sys_getuid(void) {
  return 0;
}

static int sys_geteuid(void) {
  return 0;
}

static int sys_getgid(void) {
  return 0;
}

static int sys_getegid(void) {
  return 0;
}

static int sys_chdir(uint64_t path_ptr) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  char path[256];
  if (copyinstr(p->pagetable, path, path_ptr, sizeof(path)) != 0) {
    return -EFAULT;
  }
  struct vnode *vn = NULL;
  int ret = vfs_lookup_at(p->cwd, path, &vn);
  if (ret != 0) {
    return ret;
  }
  if (vn->type != VNODE_DIR) {
    return -ENOTDIR;
  }
  p->cwd = vn;
  return 0;
}

static int sys_getcwd(uint64_t buf_ptr, uint64_t size) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  char tmp[256];
  if (size > sizeof(tmp)) {
    size = sizeof(tmp);
  }
  int ret = vfs_getcwd_at(p->cwd, tmp, (size_t)size);
  if (ret != 0) {
    return ret;
  }
  size_t n = strlen(tmp) + 1;
  if (n > size) {
    return -ERANGE;
  }
  if (copyout(p->pagetable, buf_ptr, tmp, n) != 0) {
    return -EFAULT;
  }
  return (int)buf_ptr;
}

static int sys_clone(struct trapframe *tf) {
  return proc_fork(tf);
}

static int sys_execve(struct trapframe *tf, uint64_t path_ptr, uint64_t argv_ptr) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  char path[256];
  if (copyinstr(p->pagetable, path, path_ptr, sizeof(path)) != 0) {
    return -EFAULT;
  }
  char *argv[MAX_ARG + 1];
  char argbuf[MAX_ARG][ARG_MAX_LEN];
  int argc = 0;
  if (argv_ptr != 0) {
    for (; argc < MAX_ARG; argc++) {
      uint64_t uarg = 0;
      if (copyin(p->pagetable, &uarg, argv_ptr + (uint64_t)argc * sizeof(uint64_t), sizeof(uint64_t)) != 0) {
        return -EFAULT;
      }
      if (uarg == 0) {
        break;
      }
      if (copyinstr(p->pagetable, argbuf[argc], uarg, sizeof(argbuf[argc])) != 0) {
        return -EFAULT;
      }
      argv[argc] = argbuf[argc];
    }
  }
  argv[argc] = NULL;
  int ret = proc_exec(path, argv);
  if (ret == 0 && tf) {
    *tf = *p->tf;
    return (int)tf->a0;
  }
  return ret;
}

static int sys_wait4(uint64_t pid, uint64_t status_ptr) {
  int status = 0;
  int ret = proc_wait((int)pid, (status_ptr != 0) ? &status : NULL);
  if (ret < 0) {
    return ret;
  }
  if (status_ptr != 0) {
    struct proc *p = proc_current();
    if (!p) {
      return -EINVAL;
    }
    if (copyout(p->pagetable, status_ptr, &status, sizeof(status)) != 0) {
      return -EFAULT;
    }
  }
  return ret;
}

static int sys_exit(uint64_t status) {
  proc_exit((int)status);
  return 0;
}

static long sys_reboot(uint64_t magic1, uint64_t magic2, uint64_t cmd, uint64_t arg) {
  (void)magic1;
  (void)magic2;
  (void)cmd;
  (void)arg;
  log_info("reboot: shutdown");
  sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN, SBI_SRST_REASON_NONE);
  for (;;) {
    __asm__ volatile("wfi");
  }
  __builtin_unreachable();
}

static long sys_brk(uint64_t addr) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  long ret = 0;
  if (addr == 0) {
    ret = (long)p->sz;
  } else if (addr < USER_BASE || addr > USER_STACK_BASE) {
    ret = (long)p->sz;
  } else {
    if (addr > p->sz) {
      if (uvm_alloc(p->pagetable, p->sz, addr, PTE_R | PTE_W | PTE_U) != 0) {
        ret = (long)p->sz;
      } else {
        p->sz = addr;
        ret = (long)p->sz;
      }
    } else if (addr < p->sz) {
      uvm_dealloc(p->pagetable, p->sz, addr);
      p->sz = addr;
      ret = (long)p->sz;
    } else {
      ret = (long)p->sz;
    }
  }
#ifdef CONFIG_STAGE3_DEBUG_BRK
  log_info("sys_brk: pid=%d addr=0x%lx sz=0x%lx ret=0x%lx", p->pid, addr, p->sz, ret);
#endif
  return ret;
}

long syscall_dispatch(struct trapframe *tf) {
  switch (tf->a7) {
    case SYS_WRITE:
      return sys_write(tf->a0, tf->a1, tf->a2);
    case SYS_READ:
      return sys_read(tf->a0, tf->a1, tf->a2);
    case SYS_OPENAT:
      return sys_openat(tf->a0, tf->a1, tf->a2);
    case SYS_CLOSE:
      return sys_close(tf->a0);
    case SYS_DUP:
      return sys_dup(tf->a0);
    case SYS_DUP3:
      return sys_dup3(tf->a0, tf->a1, tf->a2);
    case SYS_GETDENTS64:
      return sys_getdents64(tf->a0, tf->a1, tf->a2);
    case SYS_MKDIRAT:
      return sys_mkdirat(tf->a0, tf->a1);
    case SYS_UNLINKAT:
      return sys_unlinkat(tf->a0, tf->a1);
    case SYS_FSTAT:
      return sys_fstat(tf->a0, tf->a1);
    case SYS_FTRUNCATE:
      return sys_ftruncate(tf->a0, (int64_t)tf->a1);
    case SYS_FSTATAT:
      return sys_fstatat(tf->a0, tf->a1, tf->a2);
    case SYS_GETPID:
      return sys_getpid();
    case SYS_GETPPID:
      return sys_getppid();
    case SYS_GETUID:
      return sys_getuid();
    case SYS_GETEUID:
      return sys_geteuid();
    case SYS_GETGID:
      return sys_getgid();
    case SYS_GETEGID:
      return sys_getegid();
    case SYS_CHDIR:
      return sys_chdir(tf->a0);
    case SYS_GETCWD:
      return sys_getcwd(tf->a0, tf->a1);
    case SYS_IOCTL:
      return sys_ioctl(tf->a0, tf->a1, tf->a2);
    case SYS_LSEEK:
      return sys_lseek(tf->a0, (int64_t)tf->a1, tf->a2);
    case SYS_POLL:
      return sys_poll(tf->a0, tf->a1, (int64_t)tf->a2);
    case SYS_CLONE:
      return sys_clone(tf);
    case SYS_EXECVE:
      return sys_execve(tf, tf->a0, tf->a1);
    case SYS_WAIT4:
      return sys_wait4(tf->a0, tf->a1);
    case SYS_EXIT:
      return sys_exit(tf->a0);
    case SYS_REBOOT:
      return sys_reboot(tf->a0, tf->a1, tf->a2, tf->a3);
    case SYS_BRK:
      return sys_brk(tf->a0);
    default:
      log_error("syscall: unknown %lu", tf->a7);
      return -ENOSYS;
  }
}
