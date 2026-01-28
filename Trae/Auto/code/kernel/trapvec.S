.section .text
.align 4

.global kernel_trap_vector

# 定义trapframe大小
.equ TRAPFRAME_SIZE, 32*8 + 2*8 + 1*8  # 32个GPR + sepc + sstatus + usp

# 导出trapframe大小给C代码
.global TRAPFRAME_SIZE

kernel_trap_vector:
    # 1. 保存所有GPRs到trapframe
    addi sp, sp, -TRAPFRAME_SIZE
    
    # 保存x1-x31 (ra-sp-gp-tp-t0-t1-t2-s0-s1-a0-a1-a2-a3-a4-a5-a6-a7-s2-s3-s4-s5-s6-s7-s8-s9-s10-s11-t3-t4-t5-t6)
    sd ra, 0*8(sp)     # x1
    sd sp, 1*8(sp)     # x2 - 这里保存的是分配trapframe后的sp，后面会修正
    sd gp, 2*8(sp)     # x3
    sd tp, 3*8(sp)     # x4
    sd t0, 4*8(sp)     # x5
    sd t1, 5*8(sp)     # x6
    sd t2, 6*8(sp)     # x7
    sd s0, 7*8(sp)     # x8
    sd s1, 8*8(sp)     # x9
    sd a0, 9*8(sp)     # x10
    sd a1, 10*8(sp)    # x11
    sd a2, 11*8(sp)    # x12
    sd a3, 12*8(sp)    # x13
    sd a4, 13*8(sp)    # x14
    sd a5, 14*8(sp)    # x15
    sd a6, 15*8(sp)    # x16
    sd a7, 16*8(sp)    # x17
    sd s2, 17*8(sp)    # x18
    sd s3, 18*8(sp)    # x19
    sd s4, 19*8(sp)    # x20
    sd s5, 20*8(sp)    # x21
    sd s6, 21*8(sp)    # x22
    sd s7, 22*8(sp)    # x23
    sd s8, 23*8(sp)    # x24
    sd s9, 24*8(sp)    # x25
    sd s10, 25*8(sp)   # x26
    sd s11, 26*8(sp)   # x27
    sd t3, 27*8(sp)    # x28
    sd t4, 28*8(sp)    # x29
    sd t5, 29*8(sp)    # x30
    sd t6, 30*8(sp)    # x31
    
    # 2. 保存trap前的sp到trapframe
    addi t0, sp, TRAPFRAME_SIZE
    sd t0, 1*8(sp)     # 修正x2的值为trap前的sp
    
    # 3. 保存trap前的sp到usp字段
    sd t0, 33*8(sp)    # usp
    
    # 4. 保存特殊寄存器
    csrr t0, sepc
    csrr t1, sstatus
    sd t0, 31*8(sp)    # sepc
    sd t1, 32*8(sp)    # sstatus
    
    # 5. 调用trap处理函数
    mv a0, sp          # 传递trapframe指针
    jal ra, trap_handler
    
    # 6. 恢复特殊寄存器
    ld t0, 31*8(sp)    # sepc
    ld t1, 32*8(sp)    # sstatus
    csrw sepc, t0
    csrw sstatus, t1
    
    # 7. 恢复所有GPRs (x1-x31)
    ld ra, 0*8(sp)     # x1
    ld gp, 2*8(sp)     # x3
    ld tp, 3*8(sp)     # x4
    ld t0, 4*8(sp)     # x5
    ld t1, 5*8(sp)     # x6
    ld t2, 6*8(sp)     # x7
    ld s0, 7*8(sp)     # x8
    ld s1, 8*8(sp)     # x9
    ld a0, 9*8(sp)     # x10
    ld a1, 10*8(sp)    # x11
    ld a2, 11*8(sp)    # x12
    ld a3, 12*8(sp)    # x13
    ld a4, 13*8(sp)    # x14
    ld a5, 14*8(sp)    # x15
    ld a6, 15*8(sp)    # x16
    ld a7, 16*8(sp)    # x17
    ld s2, 17*8(sp)    # x18
    ld s3, 18*8(sp)    # x19
    ld s4, 19*8(sp)    # x20
    ld s5, 20*8(sp)    # x21
    ld s6, 21*8(sp)    # x22
    ld s7, 22*8(sp)    # x23
    ld s8, 23*8(sp)    # x24
    ld s9, 24*8(sp)    # x25
    ld s10, 25*8(sp)   # x26
    ld s11, 26*8(sp)   # x27
    ld t3, 27*8(sp)    # x28
    ld t4, 28*8(sp)    # x29
    ld t5, 29*8(sp)    # x30
    ld t6, 30*8(sp)    # x31
    
    # 8. 恢复trap前的sp
    ld sp, 1*8(sp)     # x2
    
    # 9. 返回
    sret

.end
