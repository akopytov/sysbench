/*
** Definitions for ARM64 CPUs.
** Copyright (C) 2005-2016 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_TARGET_ARM64_H
#define _LJ_TARGET_ARM64_H

/* -- Registers IDs ------------------------------------------------------- */

#define GPRDEF(_) \
  _(X0) _(X1) _(X2) _(X3) _(X4) _(X5) _(X6) _(X7) \
  _(X8) _(X9) _(X10) _(X11) _(X12) _(X13) _(X14) _(X15) \
  _(X16) _(X17) _(X18) _(X19) _(X20) _(X21) _(X22) _(X23) \
  _(X24) _(X25) _(X26) _(X27) _(X28) _(FP) _(LR) _(SP)
#define FPRDEF(_) \
  _(D0) _(D1) _(D2) _(D3) _(D4) _(D5) _(D6) _(D7) \
  _(D8) _(D9) _(D10) _(D11) _(D12) _(D13) _(D14) _(D15) \
  _(D16) _(D17) _(D18) _(D19) _(D20) _(D21) _(D22) _(D23) \
  _(D24) _(D25) _(D26) _(D27) _(D28) _(D29) _(D30) _(D31)
#define VRIDDEF(_)

#define RIDENUM(name)	RID_##name,

enum {
  GPRDEF(RIDENUM)		/* General-purpose registers (GPRs). */
  FPRDEF(RIDENUM)		/* Floating-point registers (FPRs). */
  RID_MAX,
  RID_TMP = RID_LR,
  RID_ZERO = RID_SP,

  /* Calling conventions. */
  RID_RET = RID_X0,
  RID_FPRET = RID_D0,

  /* These definitions must match with the *.dasc file(s): */
  RID_BASE = RID_X19,		/* Interpreter BASE. */
  RID_LPC = RID_X21,		/* Interpreter PC. */
  RID_GL = RID_X22,		/* Interpreter GL. */
  RID_LREG = RID_X23,		/* Interpreter L. */

  /* Register ranges [min, max) and number of registers. */
  RID_MIN_GPR = RID_X0,
  RID_MAX_GPR = RID_SP+1,
  RID_MIN_FPR = RID_MAX_GPR,
  RID_MAX_FPR = RID_D31+1,
  RID_NUM_GPR = RID_MAX_GPR - RID_MIN_GPR,
  RID_NUM_FPR = RID_MAX_FPR - RID_MIN_FPR
};

#define RID_NUM_KREF		RID_NUM_GPR
#define RID_MIN_KREF		RID_X0

/* -- Register sets ------------------------------------------------------- */

/* Make use of all registers, except for x18, fp, lr and sp. */
#define RSET_FIXED \
  (RID2RSET(RID_X18)|RID2RSET(RID_FP)|RID2RSET(RID_LR)|RID2RSET(RID_SP)|\
   RID2RSET(RID_GL))
#define RSET_GPR	(RSET_RANGE(RID_MIN_GPR, RID_MAX_GPR) - RSET_FIXED)
#define RSET_FPR	RSET_RANGE(RID_MIN_FPR, RID_MAX_FPR)
#define RSET_ALL	(RSET_GPR|RSET_FPR)
#define RSET_INIT	RSET_ALL

/* lr is an implicit scratch register. */
#define RSET_SCRATCH_GPR	(RSET_RANGE(RID_X0, RID_X17+1))
#define RSET_SCRATCH_FPR \
  (RSET_RANGE(RID_D0, RID_D7+1)|RSET_RANGE(RID_D16, RID_D31+1))
#define RSET_SCRATCH		(RSET_SCRATCH_GPR|RSET_SCRATCH_FPR)
#define REGARG_FIRSTGPR		RID_X0
#define REGARG_LASTGPR		RID_X7
#define REGARG_NUMGPR		8
#define REGARG_FIRSTFPR		RID_D0
#define REGARG_LASTFPR		RID_D7
#define REGARG_NUMFPR		8

/* -- Spill slots --------------------------------------------------------- */

/* Spill slots are 32 bit wide. An even/odd pair is used for FPRs.
**
** SPS_FIXED: Available fixed spill slots in interpreter frame.
** This definition must match with the vm_arm64.dasc file.
** Set SPS_FIXED to 0. Because there is no fixed spill slots defined in
** vm_arm64.dasc frame layout.
**
** SPS_FIRST: First spill slot for general use. Reserve min. two 32 bit slots.
**
** SPS_FIRST is set to none zero even when SPS_FIXED is 0. Because ra_hasspill()
** check against 0 to determine whether there is spill.
*/
#define SPS_FIXED       0
#define SPS_FIRST       2

#define SPOFS_TMP       0

#define sps_scale(slot)         (4 * (int32_t)(slot))
#define sps_align(slot)         (((slot) - SPS_FIXED + 3) & ~3)

/* -- Exit state ---------------------------------------------------------- */

/* This definition must match with the *.dasc file(s). */
typedef struct {
  lua_Number fpr[RID_NUM_FPR];  /* Floating-point registers. */
  intptr_t gpr[RID_NUM_GPR];     /* General-purpose registers. */
  int32_t spill[256];           /* Spill slots. */
} ExitState;

/* PC after instruction that caused an exit. Used to find the trace number. */
#define EXITSTATE_PCREG         RID_LR
/* Highest exit + 1 indicates stack check. */
#define EXITSTATE_CHECKEXIT     1

#define EXITSTUB_SPACING        4
#define EXITSTUBS_PER_GROUP     32


/* -- Instructions -------------------------------------------------------- */

/* ARM condition codes. */
typedef enum A64CC {
  CC_EQ, CC_NE, CC_CS, CC_CC, CC_MI, CC_PL, CC_VS, CC_VC,
  CC_HI, CC_LS, CC_GE, CC_LT, CC_GT, CC_LE, CC_AL,
  CC_HS = CC_CS, CC_LO = CC_CC
} A64CC;

/* Instruction fields. */
#define A64F_D(r)	(r)
#define A64F_N(r)       ((r) << 5)
#define A64F_A(r)       ((r) << 10)
#define A64F_A_U(r)     ((r) << 12)
#define A64F_M(r)       ((r) << 16)
#define A64F_IS(n)	((n) << 10)
#define A64F_IR(n)	((n) << 16)
#define A64F_U16(x)	((x) << 5)
#define A64F_S26(x)	(x)
#define A64F_S19(x)	((x) << 5)
#define A64F_COND(cc)   ((cc) << 12)  /* for CCMP */
#define A64F_NZCV(nzcv) ((nzcv) << 0) /* for CCMP */
#define A64F_EX(ex)	(A64I_EX | ((ex) << 13))
#define A64F_SH(sh, n)	(((sh) << 22) | ((n) << 10))
#define A64F_LSL16(n)	(((n) / 16) << 21)
#define A64F_BSH(sh)    ((sh) << 10)

typedef enum A64Ins {
  A64I_S = 0x20000000,
  A64I_X = 0x80000000,
  A64I_EX = 0x00200000,
  A64I_MOVKw = 0x72800000,
  A64I_MOVKx = A64I_X|A64I_MOVKw,
  A64I_MOVK_16w = 0x72a00000,
  A64I_MOVK_16x = 0xf2a00000,
  A64I_MOVK_32x = 0xf2c00000,
  A64I_MOVK_48x = 0xf2e00000,
  A64I_MOVZw = 0x52800000,
  A64I_MOVZx = 0xd2800000,
  A64I_MOVNw = 0x12800000,
  A64I_MOVNx = 0x92800000,
  A64I_MOVw = 0x2a0003e0,
  A64I_MOVx = 0xaa0003e0,
  A64I_LDRBw = 0x39400000,
  A64I_LDRHw = 0x79400000,
  A64I_LDRw = 0xb9400000,
  A64I_LDRx = 0xf9400000,
  A64I_LDRLw = 0x18000000,
  A64I_LDRLx = 0x58000000,
  A64I_NOP = 0xd503201f,
  A64I_ADDw = 0x0b000000,
  A64I_ADDx = 0x8b000000,
  A64I_ADDSw = A64I_ADDw | A64I_S,
  A64I_ADDSx = A64I_ADDx | A64I_S,
  A64I_B = 0x14000000,
  A64I_Bcond = 0x54000000,
  A64I_BL = 0x94000000,
  A64I_BR = 0xd61f0000,
  A64I_BLR = 0xd63f0000,
  A64I_ANDw = 0x0a000000,
  A64I_ANDx = 0x8a000000,
  A64I_ANDIw = 0x12000000,
  A64I_ANDIx = A64I_X|A64I_ANDIw,
  A64I_ANDSw = 0x6a000000,
  A64I_ANDSx = A64I_X|A64I_ANDSw,
  A64I_ANDSIw= 0x72000000,
  A64I_ANDSIx= A64I_X|A64I_ANDSIw,
  A64I_ORRw = 0x2a000000, /* orr w0,w0,w0 */
  A64I_ORRx = 0xaa000000, /* orr x0,x0,x0 */
  A64I_EORw = 0x4a000000,
  A64I_EORx = 0xca000000,
  A64I_BICw = 0x0a200000,
  A64I_BICx = A64I_X|A64I_BICw,
  A64I_BICSw = 0x6a200000,
  A64I_BICSx = A64I_X|A64I_BICSw,
  A64I_CCMPw = 0x7a400000, /* ccmp w0,w0,#0,eq */
  A64I_CCMPx = 0xfa400000, /* ccmp x0,x0,#0,eq */
  A64I_STRBw = 0x39000000, /* strb w0,[x0] */
  A64I_STRHw = 0x79000000, /* strh w0,[x0] */
  A64I_STRw = 0xb9000000, /* str w0,[x0] */
  A64I_STRx = 0xf9000000, /* str x0,[x0] */
  A64I_SUBx = 0xcb000000,
  A64I_SUBw = 0x4b000000,
  A64I_SUBSx = A64I_SUBx | A64I_S,
  A64I_SUBSw = A64I_SUBw | A64I_S,
  A64I_CSELw = 0x1a800000,
  A64I_CSELx = 0x9a800000,

  A64I_SXTBw = 0x13001c00,
  A64I_SXTHw = 0x13003c00,
  A64I_SXTW = 0x93407c00,
  A64I_UXTBw = 0x53001c00,
  A64I_UXTHw = 0x53003c00,
  A64I_SBFMw = 0x13000000,
  A64I_SBFMx = 0x93400000,
  A64I_UBFMw = 0x53000000,
  A64I_UBFMx = 0xd3400000,
  A64I_EXTRw = 0x13800000,
  A64I_EXTRx = 0x93c00000,

  A64I_ASRw = 0x13007c00,
  A64I_ASRx = 0x9340fc00,
  A64I_LSLx = 0xd3400000, /* lsl w0, w0, #0 */
  A64I_LSRx = 0xd340fc00, /* lsr w0, w0, #0 */
  A64I_SHRw = 0x1ac02000, /* lsl/lsr/asr/ror w0, w0, w0 */
  A64I_SHRx = 0x9ac02000,
  A64I_MVNw = 0x2a2003e0,
  A64I_MVNx = 0xaa2003e0,
  A64I_REVw = 0x5ac00800,
  A64I_REVx = 0xdac00c00,

  A64I_MULw = 0x1b007c00,
  A64I_MULx = 0x9b007c00,
  A64I_SMULL = 0x9b207c00,

  /* FP */
  A64I_FADDd = 0x1e602800,
  A64I_FSUBd = 0x1e603800,
  A64I_FMADDd = 0x1f400000,
  A64I_FMSUBd = 0x1f408000,
  A64I_FNMADDd = 0x1f600000,
  A64I_FNMSUBd = 0x1f608000,
  A64I_FMULd = 0x1e600800,
  A64I_FDIVd = 0x1e601800,
  A64I_FNEGd = 0x1e614000,
  A64I_FABS = 0x1e60c000,
  A64I_FSQRTd = 0x1e61c000,
  A64I_STRd = 0xfd000000, /* str d0,[x0] */
  A64I_LDRd = 0xfd400000, /* ldr d0,[x0] */
  A64I_STRs = 0xbd000000, /* str s0,[x0] */
  A64I_LDRs = 0xbd400000, /* ldr s0,[x0] */
  A64I_FCMPd = 0x1e602000,
  A64I_FCMPZd = 0x1e602008,
  A64I_FCSELd = 0x1e600c00,
  A64I_FRINTMd = 0x1e654000,
  A64I_FRINTPd = 0x1e64c000,
  A64I_FRINTZd = 0x1e65c000,

  A64I_FCVT_F32_F64 = 0x1e624000,
  A64I_FCVT_F64_F32 = 0x1e22c000,
  A64I_FCVT_F32_S32 = 0x1e220000,
  A64I_FCVT_F64_S32 = 0x1e620000,
  A64I_FCVT_F32_U32 = 0x1e230000,
  A64I_FCVT_F64_U32 = 0x1e630000,
  A64I_FCVT_F32_S64 = 0x9e220000,
  A64I_FCVT_F64_S64 = 0x9e620000,
  A64I_FCVT_F32_U64 = 0x9e230000,
  A64I_FCVT_F64_U64 = 0x9e630000,
  A64I_FCVT_S32_F64 = 0x1e780000,
  A64I_FCVT_S32_F32 = 0x1e380000,
  A64I_FCVT_U32_F64 = 0x1e790000,
  A64I_FCVT_U32_F32 = 0x1e390000,
  A64I_FCVT_S64_F64 = 0x9e780000,
  A64I_FCVT_S64_F32 = 0x9e380000,
  A64I_FCVT_U64_F64 = 0x9e790000,
  A64I_FCVT_U64_F32 = 0x9e390000,

  A64I_FMOV_S = 0x1e204000,
  A64I_FMOV_D = 0x1e604000,
  A64I_FMOV_R_S = 0x1e260000,
  A64I_FMOV_S_R = 0x1e270000,
  A64I_FMOV_R_D = 0x9e660000,
  A64I_FMOV_D_R = 0x9e670000,


  /* assembler aliases */
  A64I_CMPw = A64I_SUBSw | A64F_D (RID_ZERO),
  A64I_CMPx = A64I_SUBSx | A64F_D (RID_ZERO),
  A64I_CMNx = A64I_ADDSx | A64F_D (RID_ZERO),
  A64I_CMNw = A64I_ADDSw | A64F_D (RID_ZERO),
  A64I_NEGw = A64I_SUBw | A64F_N (RID_ZERO),
  A64I_NEGx = A64I_SUBx | A64F_N (RID_ZERO),
  A64I_TSTIw = A64I_ANDSIw | A64F_D (0x1f), /* tst wn, #imm*/
  A64I_TSTIx = A64I_ANDSIx | A64F_D (0x1f), /* tst xn, #imm*/
  A64I_TSTw  = A64I_ANDSw | A64F_D (0x1f),  /* tst wn, wm {,#shift}*/
  A64I_TSTx  = A64I_ANDSx | A64F_D (0x1f),  /* tst xn, xm {,#shift}*/

  /* fields */
  A64I_BINOPk = 0x1a000000, /* A64I_ADDx^A64I_BINOPk => ADD x0,x0,0 */
  A64I_BITOPk = 0x18000000, /* A64I_ANDx^A64I_BITOPk => AND x0,x0,0 */
  A64I_LS_U = 0x01000000, /* A64I_LDRx^A64I_LS_U => LDUR x0,[x0] */
  A64I_LS_S = 0x00800000, /* A64I_LDRBw^A64I_LS_S => LDRSB w0,[x0] */
} A64Ins;

typedef enum A64Shift {
  A64SH_LSL, A64SH_LSR, A64SH_ASR, A64SH_ROR
} A64Shift;

typedef enum A64Extend {
  A64EX_UXTB, A64EX_UXTH, A64EX_UXTW, A64EX_UXTX,
  A64EX_SXTB, A64EX_SXTH, A64EX_SXTW, A64EX_SXTX,
} A64Extend;

#endif
