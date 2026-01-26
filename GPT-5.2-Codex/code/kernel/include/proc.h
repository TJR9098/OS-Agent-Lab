#pragma once

#include <stdint.h>

#include "trap.h"
#include "vm.h"

#define NPROC 16
#define NOFILE 16

enum proc_state {
  PROC_UNUSED = 0,
  PROC_USED,
  PROC_RUNNABLE,
  PROC_RUNNING,
  PROC_SLEEPING,
  PROC_ZOMBIE,
};

struct vnode;
struct file;

struct context {
  uint64_t ra;
  uint64_t sp;
  uint64_t s0;
  uint64_t s1;
  uint64_t s2;
  uint64_t s3;
  uint64_t s4;
  uint64_t s5;
  uint64_t s6;
  uint64_t s7;
  uint64_t s8;
  uint64_t s9;
  uint64_t s10;
  uint64_t s11;
};

struct proc {
  int pid;
  enum proc_state state;
  struct trapframe *tf;
  struct context ctx;
  pagetable_t pagetable;
  uint64_t sz;
  uint64_t kstack;
  struct vnode *cwd;
  struct file *ofile[NOFILE];
  struct proc *parent;
  int exit_status;
};

void proc_init(void);
struct proc *proc_current(void);
struct proc *proc_alloc(void);
void proc_yield(void);
void scheduler(void);
int proc_fork(struct trapframe *tf);
int proc_exec(const char *path, char *const argv[]);
int proc_exec_on(struct proc *p, const char *path, char *const argv[]);
int proc_wait(int pid, int *status);
void proc_exit(int status);
