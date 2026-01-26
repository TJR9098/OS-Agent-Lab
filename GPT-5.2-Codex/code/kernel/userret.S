.section .text
.globl userret

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
.set TF_SEPC, 240
.set TF_SSTATUS, 248
.set TF_USP, 256

userret:
  csrw sscratch, a1

  ld t0, TF_SSTATUS(a0)
  ld t1, TF_SEPC(a0)
  csrw sstatus, t0
  csrw sepc, t1

  ld ra, TF_RA(a0)
  ld gp, TF_GP(a0)
  ld tp, TF_TP(a0)
  ld t0, TF_T0(a0)
  ld t1, TF_T1(a0)
  ld t2, TF_T2(a0)
  ld t3, TF_T3(a0)
  ld t4, TF_T4(a0)
  ld t5, TF_T5(a0)
  ld t6, TF_T6(a0)
  ld s0, TF_S0(a0)
  ld s1, TF_S1(a0)
  ld s2, TF_S2(a0)
  ld s3, TF_S3(a0)
  ld s4, TF_S4(a0)
  ld s5, TF_S5(a0)
  ld s6, TF_S6(a0)
  ld s7, TF_S7(a0)
  ld s8, TF_S8(a0)
  ld s9, TF_S9(a0)
  ld s10, TF_S10(a0)
  ld s11, TF_S11(a0)
  ld a1, TF_A1(a0)
  ld a2, TF_A2(a0)
  ld a3, TF_A3(a0)
  ld a4, TF_A4(a0)
  ld a5, TF_A5(a0)
  ld a6, TF_A6(a0)
  ld a7, TF_A7(a0)
  ld sp, TF_USP(a0)
  ld a0, TF_A0(a0)
  sret
