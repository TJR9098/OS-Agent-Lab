#include <stdint.h>
#include <string.h>

#include "errno.h"
#include "elf.h"
#include "file.h"
#include "log.h"
#include "memlayout.h"
#include "mm.h"
#include "proc.h"
#include "riscv.h"
#include "vfs.h"
#include "vm.h"

#define MAX_ARG 16
#define ARG_MAX_LEN 128
#define PGROUNDUP(a) (((a) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define SHEBANG_MAX 128
#define SHEBANG_MAX_DEPTH 1

static int parse_shebang(struct vnode *vn, char *interp, size_t interp_len,
                         char *arg, size_t arg_len) {
  if (!vn || !interp || interp_len == 0 || !arg || arg_len == 0) {
    return -EINVAL;
  }
  char buf[SHEBANG_MAX];
  int n = vfs_read(vn, buf, sizeof(buf) - 1, 0);
  if (n < 0) {
    return n;
  }
  if (n < 2) {
    return 0;
  }
  buf[n] = '\0';
  if (buf[0] != '#' || buf[1] != '!') {
    return 0;
  }
  char *p = buf + 2;
  while (*p == ' ' || *p == '\t') {
    p++;
  }
  char *start = p;
  while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
    p++;
  }
  size_t len = (size_t)(p - start);
  if (len == 0) {
    return -ENOEXEC;
  }
  if (len >= interp_len) {
    return -E2BIG;
  }
  memcpy(interp, start, len);
  interp[len] = '\0';

  while (*p == ' ' || *p == '\t') {
    p++;
  }
  if (*p && *p != '\n' && *p != '\r') {
    char *a = p;
    while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
      p++;
    }
    size_t alen = (size_t)(p - a);
    if (alen >= arg_len) {
      return -E2BIG;
    }
    memcpy(arg, a, alen);
    arg[alen] = '\0';
  } else {
    arg[0] = '\0';
  }
  return 1;
}

extern void swtch(struct context *old, struct context *new);
extern void userret(struct trapframe *tf, uint64_t kstack_top);

static struct proc g_procs[NPROC];
static struct context g_sched_ctx;
static struct proc *g_current;
static int g_next_pid = 1;
static uint8_t g_kstacks[NPROC][KSTACK_SIZE] __attribute__((aligned(PAGE_SIZE)));

static void proc_start(void) {
  struct proc *p = proc_current();
  if (!p) {
    return;
  }
  userret(p->tf, p->kstack + KSTACK_SIZE);
}

void proc_init(void) {
  for (int i = 0; i < NPROC; i++) {
    g_procs[i].state = PROC_UNUSED;
  }
}

struct proc *proc_current(void) {
  return g_current;
}

static struct proc *proc_find_unused(void) {
  for (int i = 0; i < NPROC; i++) {
    if (g_procs[i].state == PROC_UNUSED) {
      return &g_procs[i];
    }
  }
  return NULL;
}

struct proc *proc_alloc(void) {
  struct proc *p = proc_find_unused();
  if (!p) {
    return NULL;
  }
  memset(p, 0, sizeof(*p));
  p->state = PROC_USED;
  p->pid = g_next_pid++;
  void *tf = kalloc();
  if (!tf) {
    p->state = PROC_UNUSED;
    return NULL;
  }
  int idx = (int)(p - g_procs);
  p->kstack = (uint64_t)(uintptr_t)g_kstacks[idx];
  p->tf = (struct trapframe *)tf;

  p->pagetable = uvm_create();
  if (!p->pagetable) {
    kfree(tf);
    p->state = PROC_UNUSED;
    return NULL;
  }

  p->ctx.ra = (uint64_t)(uintptr_t)proc_start;
  p->ctx.sp = p->kstack + KSTACK_SIZE;
  p->cwd = vfs_root();
  return p;
}

void proc_yield(void) {
  struct proc *p = proc_current();
  if (!p) {
    return;
  }
  if (p->state == PROC_RUNNING) {
    p->state = PROC_RUNNABLE;
  }
  swtch(&p->ctx, &g_sched_ctx);
}

void scheduler(void) {
  for (;;) {
    for (int i = 0; i < NPROC; i++) {
      struct proc *p = &g_procs[i];
      if (p->state != PROC_RUNNABLE) {
        continue;
      }
      g_current = p;
      p->state = PROC_RUNNING;
      uint64_t satp = (SATP_MODE_SV39 << 60) | ((uint64_t)(uintptr_t)p->pagetable >> 12);
      write_csr(satp, satp);
      sfence_vma();
      swtch(&g_sched_ctx, &p->ctx);
      g_current = NULL;
    }
  }
}

int proc_fork(struct trapframe *tf) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  struct proc *np = proc_alloc();
  if (!np) {
    return -ENOMEM;
  }

  if (uvm_copy(p->pagetable, np->pagetable, p->sz) != 0 ||
      uvm_copy_range(p->pagetable, np->pagetable, USER_STACK_BASE, USER_STACK_TOP) != 0) {
    uvm_free(np->pagetable);
    kfree((void *)(uintptr_t)np->tf);
    np->state = PROC_UNUSED;
    return -ENOMEM;
  }
  np->sz = p->sz;
  if (tf) {
    memcpy(np->tf, tf, sizeof(*tf));
  } else {
    memcpy(np->tf, p->tf, sizeof(*p->tf));
  }
  np->tf->a0 = 0;
  np->parent = p;

  for (int i = 0; i < NOFILE; i++) {
    if (p->ofile[i]) {
      np->ofile[i] = file_dup(p->ofile[i]);
    }
  }
  np->cwd = p->cwd;
  np->state = PROC_RUNNABLE;
  return np->pid;
}

int proc_wait(int pid, int *status) {
  struct proc *p = proc_current();
  if (!p) {
    return -EINVAL;
  }
  for (;;) {
    int found = 0;
    for (int i = 0; i < NPROC; i++) {
      struct proc *c = &g_procs[i];
      if (c->parent != p) {
        continue;
      }
      if (c->state == PROC_UNUSED) {
        continue;
      }
      found = 1;
      if (pid != -1 && c->pid != pid) {
        continue;
      }
      if (c->state == PROC_ZOMBIE) {
        int cpid = c->pid;
        if (status) {
          *status = c->exit_status;
        }
        uvm_free(c->pagetable);
        kfree((void *)(uintptr_t)c->tf);
        c->parent = NULL;
        c->state = PROC_UNUSED;
        return cpid;
      }
    }
    if (!found) {
      return -ECHILD;
    }
    proc_yield();
  }
}

void proc_exit(int status) {
  struct proc *p = proc_current();
  if (!p) {
    return;
  }
  for (int i = 0; i < NOFILE; i++) {
    if (p->ofile[i]) {
      file_close(p->ofile[i]);
      p->ofile[i] = NULL;
    }
  }
  p->exit_status = status;
  p->state = PROC_ZOMBIE;
  proc_yield();
}

static int proc_exec_common_depth(struct proc *p, const char *path, char *const argv[], int depth) {
  if (!p) {
    return -EINVAL;
  }
#ifdef CONFIG_STAGE3_DEBUG_EXEC
  log_info("exec: path=%s start", path);
#endif
  struct vnode *vn = NULL;
  int ret = vfs_lookup_at(p->cwd, path, &vn);
  if (ret != 0) {
#ifdef CONFIG_STAGE3_DEBUG_EXEC
    log_info("exec: path=%s lookup ret=%d", path, ret);
#endif
    return ret;
  }

  char interp[64];
  char interp_arg[64];
  ret = parse_shebang(vn, interp, sizeof(interp), interp_arg, sizeof(interp_arg));
  if (ret < 0) {
    return ret;
  }
  if (ret > 0) {
    if (depth >= SHEBANG_MAX_DEPTH) {
      return -ELOOP;
    }
    char *new_argv[MAX_ARG + 1];
    int argc = 0;
    new_argv[argc++] = interp;
    if (interp_arg[0] != '\0' && argc < MAX_ARG) {
      new_argv[argc++] = interp_arg;
    }
    if (argc < MAX_ARG) {
      new_argv[argc++] = (char *)path;
    }
    if (argv) {
      for (int i = 1; argv[i] && argc < MAX_ARG; i++) {
        new_argv[argc++] = argv[i];
      }
    }
    new_argv[argc] = NULL;
    return proc_exec_common_depth(p, interp, new_argv, depth + 1);
  }

  pagetable_t newpt = uvm_create();
  if (!newpt) {
    return -ENOMEM;
  }

  struct elf_info info;
  ret = elf_load(newpt, vn, &info);
#ifdef CONFIG_STAGE3_DEBUG_EXEC
  log_info("exec: path=%s elf_load ret=%d", path, ret);
#endif
  if (ret != 0) {
    uvm_free(newpt);
    return ret;
  }

  uint64_t stack_base = USER_STACK_TOP - USER_STACK_SIZE;
  for (uint64_t va = stack_base; va < USER_STACK_TOP; va += PAGE_SIZE) {
    void *mem = kalloc();
    if (!mem) {
      uvm_free(newpt);
      return -ENOMEM;
    }
    if (uvm_map(newpt, va, (uint64_t)(uintptr_t)mem, PAGE_SIZE, PTE_R | PTE_W | PTE_U) != 0) {
      uvm_free(newpt);
      return -ENOMEM;
    }
  }

  uint64_t sp = USER_STACK_TOP;
  uint64_t uargv[MAX_ARG + 1];
  int argc = 0;
  if (argv) {
    for (; argv[argc] && argc < MAX_ARG; argc++) {
      size_t len = strlen(argv[argc]) + 1;
      if (len > ARG_MAX_LEN) {
        uvm_free(newpt);
        return -E2BIG;
      }
      sp -= len;
      sp &= ~0xFULL;
      if (sp < stack_base) {
        uvm_free(newpt);
        return -E2BIG;
      }
      if (copyout(newpt, sp, argv[argc], len) != 0) {
        uvm_free(newpt);
        return -EFAULT;
      }
      uargv[argc] = sp;
    }
  }
  uargv[argc] = 0;
  sp -= (uint64_t)(argc + 1) * sizeof(uint64_t);
  sp &= ~0xFULL;
  if (sp < stack_base) {
    uvm_free(newpt);
    return -E2BIG;
  }
  if (copyout(newpt, sp, uargv, (size_t)(argc + 1) * sizeof(uint64_t)) != 0) {
    uvm_free(newpt);
    return -EFAULT;
  }

  pagetable_t oldpt = p->pagetable;
  p->pagetable = newpt;
  p->sz = PGROUNDUP(info.sz);
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->sepc = info.entry;
  p->tf->sstatus = (read_csr(sstatus) & ~SSTATUS_SPP) | SSTATUS_SPIE;
  p->tf->usp = sp;
  p->tf->a0 = (uint64_t)argc;
  p->tf->a1 = sp;
  p->tf->a2 = 0;
  if (p == proc_current()) {
    uint64_t satp = (SATP_MODE_SV39 << 60) | ((uint64_t)(uintptr_t)newpt >> 12);
    write_csr(satp, satp);
    sfence_vma();
  }
  uvm_free(oldpt);
  return 0;
}

static int proc_exec_common(struct proc *p, const char *path, char *const argv[]) {
  return proc_exec_common_depth(p, path, argv, 0);
}

int proc_exec(const char *path, char *const argv[]) {
  return proc_exec_common(proc_current(), path, argv);
}

int proc_exec_on(struct proc *p, const char *path, char *const argv[]) {
  return proc_exec_common(p, path, argv);
}
