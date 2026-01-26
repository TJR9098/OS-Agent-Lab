.section .text
.globl mtrap
.align 2

.set TF_RA, 0
.set TF_GP, 8
.set TF_TP, 16
.set TF_T0, 24
.set TF_T1, 32
.set TF_T2, 40
.set TF_T3, 48
.set TF_T4, 56
.set TF_T5, 64
.set TF_T6, 72
.set TF_S0, 80
.set TF_S1, 88
.set TF_S2, 96
.set TF_S3, 104
.set TF_S4, 112
.set TF_S5, 120
.set TF_S6, 128
.set TF_S7, 136
.set TF_S8, 144
.set TF_S9, 152
.set TF_S10, 160
.set TF_S11, 168
.set TF_A0, 176
.set TF_A1, 184
.set TF_A2, 192
.set TF_A3, 200
.set TF_A4, 208
.set TF_A5, 216
.set TF_A6, 224
.set TF_A7, 232
.set TF_MEPC, 240
.set TF_MSTATUS, 248
.set TF_MCAUSE, 256
.set TF_MTVAL, 264
.set TF_SIZE, 272

mtrap:
  csrrw sp, mscratch, sp
  addi sp, sp, -TF_SIZE
  sd ra, TF_RA(sp)
  sd gp, TF_GP(sp)
  sd tp, TF_TP(sp)
  sd t0, TF_T0(sp)
  sd t1, TF_T1(sp)
  sd t2, TF_T2(sp)
  sd t3, TF_T3(sp)
  sd t4, TF_T4(sp)
  sd t5, TF_T5(sp)
  sd t6, TF_T6(sp)
  sd s0, TF_S0(sp)
  sd s1, TF_S1(sp)
  sd s2, TF_S2(sp)
  sd s3, TF_S3(sp)
  sd s4, TF_S4(sp)
  sd s5, TF_S5(sp)
  sd s6, TF_S6(sp)
  sd s7, TF_S7(sp)
  sd s8, TF_S8(sp)
  sd s9, TF_S9(sp)
  sd s10, TF_S10(sp)
  sd s11, TF_S11(sp)
  sd a0, TF_A0(sp)
  sd a1, TF_A1(sp)
  sd a2, TF_A2(sp)
  sd a3, TF_A3(sp)
  sd a4, TF_A4(sp)
  sd a5, TF_A5(sp)
  sd a6, TF_A6(sp)
  sd a7, TF_A7(sp)

  csrr t0, mepc
  sd t0, TF_MEPC(sp)
  csrr t0, mstatus
  sd t0, TF_MSTATUS(sp)
  csrr t0, mcause
  sd t0, TF_MCAUSE(sp)
  csrr t0, mtval
  sd t0, TF_MTVAL(sp)

  mv a0, sp
  call mtrap_handler

  ld t0, TF_MEPC(sp)
  csrw mepc, t0
  ld t0, TF_MSTATUS(sp)
  csrw mstatus, t0

  ld ra, TF_RA(sp)
  ld gp, TF_GP(sp)
  ld tp, TF_TP(sp)
  ld t0, TF_T0(sp)
  ld t1, TF_T1(sp)
  ld t2, TF_T2(sp)
  ld t3, TF_T3(sp)
  ld t4, TF_T4(sp)
  ld t5, TF_T5(sp)
  ld t6, TF_T6(sp)
  ld s0, TF_S0(sp)
  ld s1, TF_S1(sp)
  ld s2, TF_S2(sp)
  ld s3, TF_S3(sp)
  ld s4, TF_S4(sp)
  ld s5, TF_S5(sp)
  ld s6, TF_S6(sp)
  ld s7, TF_S7(sp)
  ld s8, TF_S8(sp)
  ld s9, TF_S9(sp)
  ld s10, TF_S10(sp)
  ld s11, TF_S11(sp)
  ld a0, TF_A0(sp)
  ld a1, TF_A1(sp)
  ld a2, TF_A2(sp)
  ld a3, TF_A3(sp)
  ld a4, TF_A4(sp)
  ld a5, TF_A5(sp)
  ld a6, TF_A6(sp)
  ld a7, TF_A7(sp)
  addi sp, sp, TF_SIZE
  csrrw sp, mscratch, sp
  mret
