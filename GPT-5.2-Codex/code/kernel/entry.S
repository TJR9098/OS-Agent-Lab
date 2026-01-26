.section .text.boot
.globl _start
.globl kernel_boot

_start:
kernel_boot:
  la sp, boot_stack_top

  addi sp, sp, -24
  sd a0, 0(sp)
  sd a1, 8(sp)
  sd a2, 16(sp)

  call boot_pagetable_init

  srli t0, a0, 12
  li t1, 8
  slli t1, t1, 60
  or t0, t0, t1
  csrw satp, t0
  sfence.vma

  la t0, __bss_start
  la t1, __bss_end
1:
  bgeu t0, t1, 2f
  sb zero, 0(t0)
  addi t0, t0, 1
  j 1b
2:

  ld a0, 0(sp)
  ld a1, 8(sp)
  ld a2, 16(sp)
  addi sp, sp, 24

  la t2, kernel_stack_top_ptr
  ld sp, 0(t2)

  la t2, kernel_entry_ptr
  ld t3, 0(t2)
  jr t3

.section .boot
.align 3
.globl kernel_entry_ptr
kernel_entry_ptr:
  .quad kernel_main
.globl kernel_stack_top_ptr
kernel_stack_top_ptr:
  .quad kernel_stack_top

.equ BOOT_STACK_SIZE, 16384
.align 12
.globl boot_stack
boot_stack:
  .space BOOT_STACK_SIZE
.globl boot_stack_top
boot_stack_top:
