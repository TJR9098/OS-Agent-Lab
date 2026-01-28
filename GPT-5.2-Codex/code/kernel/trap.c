#include <stdint.h>

#include "errno.h"
#include "log.h"
#include "proc.h"
#include "riscv.h"
#include "syscall.h"
#include "timer.h"
#include "trap.h"
#include "vm.h"

extern void kernel_trap_vector(void);

static int trap_is_interrupt(uint64_t scause) {
  return (scause >> 63) != 0;
}

static uint64_t trap_code(uint64_t scause) {
  return scause & 0xfffU;
}

#ifdef CONFIG_STAGE4_DEBUG_VM_WALK
#define SATP_PPN_MASK ((1UL << 44) - 1)
#define PTE2PA(pte) (((pte) >> 10) << 12)

static void debug_walk_current(uint64_t va) {
  uint64_t satp_val = read_csr(satp);
  uint64_t root_pa = (satp_val & SATP_PPN_MASK) << 12;
  pte_t *l0 = (pte_t *)(uintptr_t)root_pa;
  uint64_t i2 = VPN2(va);
  pte_t pte2 = l0[i2];
  log_info("vmwalk: satp=0x%lx root=0x%lx va=0x%lx vpn2=%lu pte2=0x%lx",
           satp_val,
           root_pa,
           va,
           i2,
           pte2);
  if ((pte2 & PTE_V) == 0) {
    return;
  }
  if (pte2 & (PTE_R | PTE_W | PTE_X)) {
    log_info("vmwalk: l0 leaf pa=0x%lx flags=0x%lx", PTE2PA(pte2), pte2 & 0x3ff);
    return;
  }
  pte_t *l1 = (pte_t *)(uintptr_t)PTE2PA(pte2);
  uint64_t i1 = VPN1(va);
  pte_t pte1 = l1[i1];
  log_info("vmwalk: vpn1=%lu pte1=0x%lx", i1, pte1);
  if ((pte1 & PTE_V) == 0) {
    return;
  }
  if (pte1 & (PTE_R | PTE_W | PTE_X)) {
    log_info("vmwalk: l1 leaf pa=0x%lx flags=0x%lx", PTE2PA(pte1), pte1 & 0x3ff);
    return;
  }
  pte_t *l2 = (pte_t *)(uintptr_t)PTE2PA(pte1);
  uint64_t i0 = VPN0(va);
  pte_t pte0 = l2[i0];
  log_info("vmwalk: vpn0=%lu pte0=0x%lx", i0, pte0);
  if (pte0 & PTE_V) {
    log_info("vmwalk: l2 leaf pa=0x%lx flags=0x%lx", PTE2PA(pte0), pte0 & 0x3ff);
  }
}
#endif

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
    log_error("trap: pid=%d scause=0x%lx sepc=0x%lx stval=0x%lx ra=0x%lx usp=0x%lx a0=0x%lx a1=0x%lx a5=0x%lx",
              p->pid,
              scause,
              tf->sepc,
              stval,
              tf->ra,
              tf->usp,
              tf->a0,
              tf->a1,
              tf->a5);
#else
    log_error("trap: pid=%d scause=0x%lx sepc=0x%lx stval=0x%lx", p->pid, scause, tf->sepc, stval);
#endif
#ifdef CONFIG_STAGE4_DEBUG_VM_WALK
    if (scause == 0xc || scause == 0xd || scause == 0xf) {
      debug_walk_current(tf->sepc);
      debug_walk_current((uint64_t)&kernel_trap_vector);
    }
#endif
    proc_exit(-1);
    return;
  }

  log_error("trap: scause=0x%lx sepc=0x%lx stval=0x%lx", scause, tf->sepc, stval);
  for (;;) {
    __asm__ volatile("wfi");
  }
}
