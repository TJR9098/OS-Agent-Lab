.section .bss
.align 12

.equ BOOT_STACK_SIZE, 16384
.equ KERNEL_STACK_SIZE, 16384

.global boot_stack
boot_stack:
    .space BOOT_STACK_SIZE, 0

.global boot_stack_top
boot_stack_top:

.align 12

.global kernel_stack
kernel_stack:
    .space KERNEL_STACK_SIZE, 0

.global kernel_stack_top
kernel_stack_top:
