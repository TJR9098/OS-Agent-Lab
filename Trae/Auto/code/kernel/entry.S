.section .text.boot

.global kernel_boot

kernel_boot:
    # 1. 设置boot栈指针
    la sp, boot_stack_top
    
    # 2. 保存hartid到t0寄存器
    mv t0, a0
    
    # 3. 清零BSS段
    la t1, __bss_start
    la t2, __bss_end
    
clear_bss:
    bge t1, t2, bss_done
    sd zero, (t1)
    addi t1, t1, 8
    j clear_bss

bss_done:
    # 4. 恢复hartid到a0
    mv a0, t0
    
    # 5. 切换到kernel栈
    la sp, kernel_stack_top
    
    # 6. 跳转到kernel_main
    jal ra, kernel_main
    
    # 如果kernel_main返回，进入死循环
    j .

.end
