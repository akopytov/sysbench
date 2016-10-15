/*
** ARM64 instruction emitter.
** Copyright !!!TODO
*/

static Reg ra_allock(ASMState *as, intptr_t k, RegSet allow);

#define emit_canremat(ref)      ((ref) <= ASMREF_L)

#define glofs(as, k) \
  ((intptr_t)((uintptr_t)(k) - (uintptr_t)&J2GG(as->J)->g))

#define emit_getgl(as, r, field) \
  emit_lsptr(as, A64I_LDRx, (r), (void *)&J2G(as->J)->field)
#define emit_setgl(as, r, field) \
  emit_lsptr(as, A64I_STRx, (r), (void *)&J2G(as->J)->field)

static void emit_d(ASMState *as, A64Ins ai, Reg rd)
{
  *--as->mcp = ai | A64F_D(rd);
}

static void emit_n(ASMState *as, A64Ins ai, Reg rn)
{
  *--as->mcp = ai | A64F_N(rn);
}

static void emit_dn(ASMState *as, A64Ins ai, Reg rd, Reg rn)
{
  *--as->mcp = ai | A64F_D(rd) | A64F_N(rn);
}

static void emit_dm(ASMState *as, A64Ins ai, Reg rd, Reg rm)
{
  *--as->mcp = ai | A64F_D(rd) | A64F_M(rm);
}

static void emit_nm(ASMState *as, A64Ins ai, Reg rn, Reg rm)
{
  *--as->mcp = ai | A64F_N(rn) | A64F_M(rm);
}

static void emit_dnm(ASMState *as, A64Ins ai, Reg rd, Reg rn, Reg rm)
{
  *--as->mcp = ai | A64F_D(rd) | A64F_N(rn) | A64F_M(rm);
}

static void emit_dnma(ASMState *as, A64Ins ai, Reg rd, Reg rn, Reg rm, Reg ra)
{
  *--as->mcp = ai | A64F_D(rd) | A64F_N(rn) | A64F_M(rm) | A64F_A(ra);
}

/* Encode constant in K12 format for data processing instructions. */
static int emit_isk12(int64_t n, uint32_t *k12)
{
  uint64_t k = (n < 0) ? -n : n;
  uint32_t m = (n < 0) ? 0x40000000 : 0;
  if (k < 4096) {
    *k12 = A64I_BINOPk|m|(k<<10);
    return 1;
  } else if ((k & 0xfff000) == k) {
    *k12 = A64I_BINOPk|m|0x400000|(k>>2);
    return 1;
  }
  return 0;
}

int count_leading_zeroes(uint64_t value)
{
  return ((value == 0) ? 64 : __builtin_clzll(value));
}

/* Encode constant in K13 format for data processing instructions. */

/* The source code has been copied from arm vixl implementation
 * https://github.com/armvixl/vixl.git function IsImmLogical()
 * !!TODO: We can speed up the steps by precalculating all possible
 * encodings and then doing a binary search on that. It takes about
 * storage space of 5k entries.
 */
/******************************************************************************/
/********************Copyright Notice for emit_isk13()*************************/
/******************************************************************************/
// Copyright 2015, ARM Limited
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of ARM Limited nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/******************************************************************************/
static uint32_t emit_isk13(A64Ins ai, uint64_t n)
{
  int is64 = ((ai & A64I_X) != 0x0);
  uint32_t res;
  int neg = 0;
  uint64_t a, n_plus_a, b, n_plus_a_minus_b, c, mask;
  int d, clz_a, clz_b, out_n, r, s;
  uint64_t multiplier, candidate;
  uint64_t multipliers[] = {
      0x0000000000000001UL,
      0x0000000100000001UL,
      0x0001000100010001UL,
      0x0101010101010101UL,
      0x1111111111111111UL,
      0x5555555555555555UL,
  };

  if (n & 1) {
    neg = 1;
    n = ~n;
  }
  if (!is64) {
    n <<= 32;
    n |= n >> 32;
  }
  a = (n & -n);  //Set to lowest set bit;
  n_plus_a = n + a;
  b = (n_plus_a & -n_plus_a);
  n_plus_a_minus_b = n_plus_a - b;
  c = (n_plus_a_minus_b & -n_plus_a_minus_b);
  if (c) { /* General case */
    int clz_c = count_leading_zeroes(c);
    clz_a = count_leading_zeroes(a);
    d = clz_a - clz_c;
    mask = (1ull << d) - 1;
    out_n = 0;
  }
  else {  /* degenerate case */
    if (!a)
      return (uint32_t)-1;
    else {
      clz_a = count_leading_zeroes(a);
      d = 64;
      mask = ~(0ull);
      out_n = 1;
    }
  }
  if ((d & (d-1)) != 0)  /* Not a power of 2 */
    return (uint32_t)-1;

  if (((b - a) & ~mask) != 0)
    return (uint32_t)-1;

  multiplier = multipliers[count_leading_zeroes(d) - 57];
  candidate = (b - a) * multiplier;
  if (n != candidate)
    return (uint32_t)-1;

  clz_b  = b ? count_leading_zeroes(b) : -1;
  s = clz_a - clz_b;
  if (neg) {
    s = d - s;
    r = (clz_b + 1) & (d - 1);
  }
  else
    r = (clz_a + 1) & (d - 1);
  s = ((-d << 1) | (s - 1)) & 0x3f;
  res = (out_n<<12) | (r << 6) | s;
  return (res<<10);
}

/* -- Emit loads/stores --------------------------------------------------- */

typedef enum {
  OFS_INVALID,
  OFS_UNSCALED,
  OFS_SCALED_0,
  OFS_SCALED_1,
  OFS_SCALED_2,
  OFS_SCALED_3,
} ofs_type;

static ofs_type check_offset(A64Ins ai, int32_t ofs)
{
  int scale;
  switch (ai)
  {
  case A64I_LDRBw: scale = 0; break;
  case (A64I_LDRBw ^ A64I_LS_S): scale = 0; break;
  case A64I_STRBw: scale = 0; break;
  case A64I_LDRHw: scale = 1; break;
  case (A64I_LDRHw ^ A64I_LS_S): scale = 1; break;
  case A64I_STRHw: scale = 1; break;
  case A64I_LDRw: scale = 2; break;
  case A64I_STRw: scale = 2; break;
  case A64I_LDRs: scale = 2; break;
  case A64I_STRs: scale = 2; break;
  case A64I_LDRx: scale = 3; break;
  case A64I_STRx: scale = 3; break;
  case A64I_LDRd: scale = 3; break;
  case A64I_STRd: scale = 3; break;
  default: lua_assert(!"invalid instruction in check_offset");
  }

  /* do we need to use unscaled op? */
  if (ofs < 0 || (ofs & ((1<<scale)-1)))
  {
    /* unaligned, so need to use u variant (eg ldur) */
    return (ofs >= -256 && ofs <= 255) ? OFS_UNSCALED : OFS_INVALID;
  } else {
    return (ofs >= 0 && ofs <= (4096<<scale)) ? OFS_SCALED_0 + scale : OFS_INVALID;
  }
}

static void emit_lso(ASMState *as, A64Ins ai, Reg rd, Reg rn, int32_t ofs)
{
  /* !!!TODO ARM emit_lso combines LDR/STR pairs into LDRD/STRD, something
     similar possible here? */

  lua_assert(rd <= 31);

  ofs_type ot = check_offset(ai, ofs);
  lua_assert(ot != OFS_INVALID);
  if (ot == OFS_UNSCALED) {
    ai ^= A64I_LS_U;
    *--as->mcp = ai | A64F_D(rd) | A64F_N(rn) | A64F_A_U(ofs & 0x1ff);
  } else {
    int32_t ofs_field;
    ofs_field = ofs >> (ot - OFS_SCALED_0);
    *--as->mcp = ai | A64F_D(rd) | A64F_N(rn) | A64F_A(ofs_field);
  }
}

static void emit_lsptr(ASMState *as, A64Ins ai, Reg r, void *p)
{
  int64_t ofs = glofs(as, p);
  if (checki32(ofs) && check_offset(ai, ofs)) {
    emit_lso(as, ai, r, RID_GL, ofs);
  } else {
    int64_t i = i64ptr(p);
    Reg tmp = RID_TMP; /*!!!TODO allocate register? */
    emit_lso(as, ai, r, tmp, 0);
    *--as->mcp = A64I_MOVK_48x | A64F_D(tmp) | A64F_U16((i>>48) & 0xffff);
    *--as->mcp = A64I_MOVK_32x | A64F_D(tmp) | A64F_U16((i>>32) & 0xffff);
    *--as->mcp = A64I_MOVK_16x | A64F_D(tmp) | A64F_U16((i>>16) & 0xffff);
    *--as->mcp = A64I_MOVZx | A64F_D(tmp) | A64F_U16(i & 0xffff);
  }
}

/* Load a 32 bit constant into a GPR. */
static void emit_loadi(ASMState *as, Reg rd, int32_t i)
{
  /* !!!TODO handle wide move */
  if (i & 0xffff0000) {
    *--as->mcp = A64I_MOVK_16w | A64F_D(rd) | A64F_U16((i>>16) & 0xffff);
  }
  *--as->mcp = A64I_MOVZw | A64F_D(rd) | A64F_U16(i & 0xffff);
}

/* mov r, imm64 or shorter 32 bit extended load. */
static void emit_loadu64(ASMState *as, Reg rd, uint64_t u64)
{
  /* !!!TODO plenty of ways to optimise this! */
  if (u64 & 0xffff000000000000) {
    *--as->mcp = A64I_MOVK_48x | A64F_D(rd) | A64F_U16((u64>>48) & 0xffff);
  }
  if (u64 & 0xffff00000000) {
    *--as->mcp = A64I_MOVK_32x | A64F_D(rd) | A64F_U16((u64>>32) & 0xffff);
  }
  if (u64 & 0xffff0000) {
    *--as->mcp = A64I_MOVK_16x | A64F_D(rd) | A64F_U16((u64>>16) & 0xffff);
  }
  *--as->mcp = A64I_MOVZw | A64F_D(rd) | A64F_U16(u64 & 0xffff);
}

#define emit_loada(as, r, addr)   emit_loadu64(as, (r), (uintptr_t)(addr))

/* Generic load of register with base and (small) offset address. */
static void emit_loadofs(ASMState *as, IRIns *ir, Reg r, Reg base, int32_t ofs)
{
#if LJ_SOFTFP
  lua_assert(!irt_isnum(ir->t)); UNUSED(ir);
#else
  if (r >= RID_MAX_GPR)
    emit_lso(as, irt_isnum(ir->t) ? A64I_LDRd : A64I_LDRs, r & 31, base, ofs);
  else
#endif
    emit_lso(as, irt_is64(ir->t) ? A64I_LDRx : A64I_LDRw, r, base, ofs);
}

/* Generic store of register with base and (small) offset address. */
static void emit_storeofs(ASMState *as, IRIns *ir, Reg r, Reg base, int32_t ofs)
{
  if (r >= RID_MAX_GPR) {
    emit_lso(as, irt_isnum(ir->t) ? A64I_STRd : A64I_STRs, r & 31, base, ofs);
  } else {
    emit_lso(as, irt_is64(ir->t) ? A64I_STRx : A64I_STRw, r, base, ofs);
  }
}

/* Generic move between two regs. */
static void emit_movrr(ASMState *as, IRIns *ir, Reg dst, Reg src)
{
#if LJ_SOFTFP
  lua_assert(!irt_isnum(ir->t)); UNUSED(ir);
#else
  if (dst >= RID_MAX_GPR) {
    emit_dn(as, irt_isnum(ir->t) ? A64I_FMOV_D : A64I_FMOV_S,
     (dst & 31), (src & 31));
    return;
  }
#endif

// TODO: add swapping early registers for loads/stores?

  emit_dm(as, A64I_MOVx, dst, src);
}

/* Emit an arithmetic/logic operation with a constant operand. */
static void emit_opk(ASMState *as, A64Ins ai, Reg dest, Reg src,
                     int32_t i, RegSet allow)
{
  uint32_t k;
  if (emit_isk12(i, &k))
    emit_dn(as, ai^k, dest, src);
  else
    emit_dnm(as, ai, dest, src, ra_allock(as, i, allow));
}

static void emit_ccmpr(ASMState *as, A64Ins ai, A64CC cond, int32_t nzcv, Reg
rn, Reg rm)
{
  *--as->mcp = ai | A64F_N(rn) | A64F_M(rm) | A64F_NZCV(nzcv) | A64F_COND(cond);
}

static void emit_ccmpk(ASMState *as, A64Ins ai, A64CC cond, int32_t nzcv, Reg
rn, int32_t k, RegSet allow)
{
  if (k >=0 && k <= 31)
    *--as->mcp =
      ai | A64F_N (rn) | A64F_M (k) | A64F_NZCV (nzcv) | A64F_COND (cond);
  else
  {
    emit_ccmpr(as, ai, cond, nzcv, rn, ra_allock(as, k, allow));
  }
}

static Reg ra_scratch(ASMState *as, RegSet allow);

/* Load 64 bit IR constant into register. */
static void emit_loadk64(ASMState *as, Reg r, IRIns *ir)
{
  // TODO: This can probably be optimized.
  Reg r64 = r;
  uint64_t k = ir_k64(ir)->u64;
  if (rset_test(RSET_FPR, r)) {
    r64 = ra_scratch(as, RSET_GPR);
    emit_dn(as, A64I_FMOV_D_R, (r & 31), r64);
  }
  emit_loadu64(as, r64, k);
}

/* -- Emit control-flow instructions -------------------------------------- */

/* Label for internal jumps. */
typedef MCode *MCLabel;

/* Return label pointing to current PC. */
#define emit_label(as)		((as)->mcp)

static void emit_branch(ASMState *as, A64Ins ai, MCode *target)
{
  MCode *p = as->mcp;
  ptrdiff_t delta = target - (p - 1);
  lua_assert(((delta + 0x02000000) >> 26) == 0);
  *--p = ai | ((uint32_t)delta & 0x03ffffffu);
  as->mcp = p;
}

static void emit_cond_branch(ASMState *as, A64CC cond, MCode *target)
{
  MCode *p = as->mcp;
  ptrdiff_t delta = target - (p - 1);
  lua_assert(((delta + 0x40000) >> 19) == 0);
  *--p = A64I_Bcond | (((uint32_t)delta & 0x7ffff)<<5) | cond;
  as->mcp = p;
}

/* Add offset to pointer. */
static void emit_addptr(ASMState *as, Reg r, int32_t ofs)
{
  if (ofs)
  {
    A64Ins op = ofs < 0 ? A64I_SUBx : A64I_ADDx;
    if (ofs < 0)
      ofs = -ofs;
    emit_opk(as, op, r, r, ofs, rset_exclude(RSET_GPR, r));
  }
}

#define emit_jmp(as, target) emit_branch(as, A64I_B, (target))

#define emit_setvmstate(as, i)                UNUSED(i)
#define emit_spsub(as, ofs)                   emit_addptr(as, RID_SP, -(ofs))

static void emit_call(ASMState *as, void *target)
{
  static const int32_t imm_bits = 26;
  ptrdiff_t delta = (char *)target - (char *)(as->mcp - 1);
  lua_assert((delta & 3) == 0);
  if ((((delta >> 2) + (1 << (imm_bits - 1))) >> imm_bits) == 0) {
    *--as->mcp = A64I_BL | ((delta >> 2) & ((1 << imm_bits) - 1));
  } else {
    /* Use LR for indirect calls. */
    emit_n(as, A64I_BLR, RID_LR);
    emit_loada(as, RID_LR, target);
  }
}
