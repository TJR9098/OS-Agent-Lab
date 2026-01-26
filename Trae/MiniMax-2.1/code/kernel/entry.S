.section .text.boot
.align 4

.global kernel_boot
kernel_boot:
    csrrw a0, sscratch, a0
    mv s0, a0
    la sp, boot_stack_top
    
    call clear_bss
    
    la sp, kernel_stack_top
    csrw sscratch, sp
    mv a0, s0
    call kernel_main

clear_bss:
    la t0, __bss_start
    la t1, __bss_end
    bge t0, t1, clear_bss_done
clear_bss_loop:
    sd zero, 0(t0)
    addi t0, t0, 8
    blt t0, t1, clear_bss_loop
clear_bss_done:
    ret
