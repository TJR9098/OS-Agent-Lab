.section .bss.stack, "aw", @nobits
.align 16

// Boot Stack (16KB)
.global boot_stack
.global boot_stack_top
boot_stack:
    .skip 16 * 1024  // 16KB
boot_stack_top:

// Kernel Stack (16KB)
.global kernel_stack
.global kernel_stack_top
kernel_stack:
    .skip 16 * 1024  // 16KB
kernel_stack_top:

.end
