.section .text.init
.globl _start

_start:
  la sp, stack_top

  la t0, __bss_start
  la t1, __bss_end
1:
  bgeu t0, t1, 2f
  sd zero, 0(t0)
  addi t0, t0, 8
  j 1b
2:
  mv a0, a0
  mv a1, a1
  call firmware_main

3:
  wfi
  j 3b

.section .bss
.equ STACK_SIZE, 16384
.align 16
.globl stack
stack:
  .space STACK_SIZE
.globl stack_top
stack_top:

.equ TRAP_STACK_SIZE, 4096
.align 16
.globl trap_stack
trap_stack:
  .space TRAP_STACK_SIZE
.globl trap_stack_top
trap_stack_top:
