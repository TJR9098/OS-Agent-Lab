.section .bss
.equ KERNEL_STACK_SIZE, 16384
.align 16
.globl kernel_stack
kernel_stack:
  .space KERNEL_STACK_SIZE
.globl kernel_stack_top
kernel_stack_top: