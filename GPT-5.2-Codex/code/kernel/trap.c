#include <stdint.h>

#include "errno.h"
#include "log.h"
#include "proc.h"
#include "riscv.h"
#include "syscall.h"
#include "timer.h"
#include "trap.h"

extern void kernel_trap_vector(void);

static int trap_is_interrupt(uint64_t scause) {
  return (scause >> 63) != 0;
}

static uint64_t trap_code(uint64_t scause) {
  return scause & 0xfffU;
}

void trap_init(void) {
  write_csr(stvec, (uint64_t)&kernel_trap_vector & ~0x3ULL);
}

void trap_enable(void) {
  set_csr(sie, SIE_STIE);
  set_csr(sstatus, SSTATUS_SIE);
}

void kernel_trap_handler(struct trapframe *tf) {
  uint64_t scause = read_csr(scause);
  uint64_t stval = read_csr(stval);
  struct proc *p = proc_current();

  if (trap_is_interrupt(scause)) {
    uint64_t code = trap_code(scause);
    if (code == 5) {
      uint64_t hartid = read_tp();
      timer_handle(hartid);
      if (p && p->state == PROC_RUNNING) {
        proc_yield();
      }
      return;
    }
  }

  if (scause == 8) {
    tf->sepc += 4;
#ifdef CONFIG_STAGE3_DEBUG_BRK
    long ret = syscall_dispatch(tf);
    if (tf->a7 == SYS_BRK) {
      log_info("syscall brk: ret=0x%lx", (uint64_t)ret);
    }
    tf->a0 = ret;
#else
    tf->a0 = syscall_dispatch(tf);
#endif
    return;
  }

  if (p && p->state == PROC_RUNNING) {
#ifdef CONFIG_STAGE3_DEBUG_TRAP
    log_error("trap: pid=%d scause=0x%lx sepc=0x%lx stval=0x%lx a0=0x%lx a1=0x%lx a5=0x%lx",
              p->pid,
              scause,
              tf->sepc,
              stval,
              tf->a0,
              tf->a1,
              tf->a5);
#else
    log_error("trap: pid=%d scause=0x%lx sepc=0x%lx stval=0x%lx", p->pid, scause, tf->sepc, stval);
#endif
    proc_exit(-1);
    return;
  }

  log_error("trap: scause=0x%lx sepc=0x%lx stval=0x%lx", scause, tf->sepc, stval);
  for (;;) {
    __asm__ volatile("wfi");
  }
}
