/*
** ARM IR assembler (SSA IR -> machine code).
** Copyright (C) 2005-2016 Mike Pall. See Copyright Notice in luajit.h
*/

/* -- Register allocator extensions --------------------------------------- */

/* Allocate a register with a hint. */
static Reg ra_hintalloc(ASMState *as, IRRef ref, Reg hint, RegSet allow)
{
  Reg r = IR(ref)->r;
  if (ra_noreg(r)) {
    if (!ra_hashint(r) && !iscrossref(as, ref))
      ra_sethint(IR(ref)->r, hint);  /* Propagate register hint. */
    r = ra_allocref(as, ref, allow);
  }
  ra_noweak(as, r);
  return r;
}

/* Allocate two source registers for three-operand instructions. */
static Reg ra_alloc2(ASMState *as, IRIns *ir, RegSet allow)
{
  IRIns *irl = IR(ir->op1), *irr = IR(ir->op2);
  Reg left = irl->r, right = irr->r;
  if (ra_hasreg(left)) {
    ra_noweak(as, left);
    if (ra_noreg(right))
      right = ra_allocref(as, ir->op2, rset_exclude(allow, left));
    else
      ra_noweak(as, right);
  } else if (ra_hasreg(right)) {
    ra_noweak(as, right);
    left = ra_allocref(as, ir->op1, rset_exclude(allow, right));
  } else if (ra_hashint(right)) {
    right = ra_allocref(as, ir->op2, allow);
    left = ra_alloc1(as, ir->op1, rset_exclude(allow, right));
  } else {
    left = ra_allocref(as, ir->op1, allow);
    right = ra_alloc1(as, ir->op2, rset_exclude(allow, left));
  }
  return left | (right << 8);
}

/* -- Guard handling ------------------------------------------------------ */

/* Generate an exit stub group at the bottom of the reserved MCode memory. */
static MCode *asm_exitstub_gen(ASMState *as, ExitNo group)
{
  MCode *mxp = as->mcbot;
  int i;
  intptr_t dispatch;
  if (mxp + 5*4+4*EXITSTUBS_PER_GROUP >= as->mctop)
    asm_mclimit(as);
  dispatch = i64ptr(J2GG(as->J)->dispatch);
  /* str lr, [sp, #TMPDofs];
     bl ->vm_exit_handler;
     .long DISPATCH_address (lo)
     .long DISPATCH_address (hi)
     .long group. */
  *mxp++ = A64I_STRx|A64F_D(RID_LR)|A64F_N(RID_SP);
  *mxp = A64I_BL|(((MCode *)(void *)lj_vm_exit_handler-mxp)&0x03ffffffu);
  mxp++;
  *mxp++ = (MCode)(dispatch & 0xffffffff);          /* DISPATCH address (lo) */
  *mxp++ = (MCode)((dispatch >> 32) & 0xffffffff);  /* DISPATCH address (hi) */
  *mxp++ = group*EXITSTUBS_PER_GROUP;
  for (i = 0; i < EXITSTUBS_PER_GROUP; i++)
    *mxp++ = A64I_B|((-5-i)&0x03ffffffu);
  lj_mcode_sync(as->mcbot, mxp);
  lj_mcode_commitbot(as->J, mxp);
  as->mcbot = mxp;
  as->mclim = as->mcbot + MCLIM_REDZONE;
  return mxp - EXITSTUBS_PER_GROUP;
}

/* Setup all needed exit stubs. */
static void asm_exitstub_setup(ASMState *as, ExitNo nexits)
{
  ExitNo i;
  if (nexits >= EXITSTUBS_PER_GROUP*LJ_MAX_EXITSTUBGR)
    lj_trace_err(as->J, LJ_TRERR_SNAPOV);
  for (i = 0; i < (nexits+EXITSTUBS_PER_GROUP-1)/EXITSTUBS_PER_GROUP; i++)
    if (as->J->exitstubgroup[i] == NULL)
      as->J->exitstubgroup[i] = asm_exitstub_gen(as, i);
}

/* Emit conditional branch to exit for guard. */
static void asm_guardcc(ASMState *as, A64CC cc)
{
  MCode *target = exitstub_addr(as->J, as->snapno);
  MCode *p = as->mcp;
  if (LJ_UNLIKELY(p == as->invmcp)) {
    as->loopinv = 1;
    *p = A64I_BL | ((target-p) & 0x03ffffffu);
    emit_cond_branch(as, cc^1, p-1); /* branch to self, patched in asm_loop_fixup */
    return;
  }
  /* ARM64 doesn't have conditional BL, so we emit an unconditional BL
     and branch around it with the opposite condition */
  emit_branch(as, A64I_BL, target);
  emit_cond_branch(as, cc^1, p);
}

/* -- Operand fusion ------------------------------------------------------ */

/* Limit linear search to this distance. Avoids O(n^2) behavior. */
#define CONFLICT_SEARCH_LIM	31

static int asm_isk32(ASMState *as, IRRef ref, int32_t *k)
{
  if (irref_isk(ref)) {
    IRIns *ir = IR(ref);
    if (ir->o == IR_KNULL || !irt_is64(ir->t)) {
      *k = ir->i;
      return 1;
    } else if (checki32((int64_t)ir_k64(ir)->u64)) {
      *k = (int32_t)ir_k64(ir)->u64;
      return 1;
    }
  }
  return 0;
}

/* Check if there's no conflicting instruction between curins and ref. */
static int noconflict(ASMState *as, IRRef ref, IROp conflict)
{
  IRIns *ir = as->ir;
  IRRef i = as->curins;
  if (i > ref + CONFLICT_SEARCH_LIM)
    return 0;  /* Give up, ref is too far away. */
  while (--i > ref)
    if (ir[i].o == conflict)
      return 0;  /* Conflict found. */
  return 1;  /* Ok, no conflict. */
}

/* Fuse the array base of colocated arrays. */
static int32_t asm_fuseabase(ASMState *as, IRRef ref)
{
  IRIns *ir = IR(ref);
  if (ir->o == IR_TNEW && ir->op1 <= LJ_MAX_COLOSIZE &&
      !neverfuse(as) && noconflict(as, ref, IR_NEWREF))
    return (int32_t)sizeof(GCtab);
  return 0;
}

/* Fuse array/hash/upvalue reference into register+offset operand. */
static Reg asm_fuseahuref(ASMState *as, IRRef ref, int32_t *ofsp, RegSet allow,
			  A64Ins ins)
{
  IRIns *ir = IR(ref);
  if (ra_noreg(ir->r)) {
    if (ir->o == IR_AREF) {
      if (mayfuse(as, ref)) {
        if (irref_isk(ir->op2)) {
          IRRef tab = IR(ir->op1)->op1;
          int32_t ofs = asm_fuseabase(as, tab);
          IRRef refa = ofs ? tab : ir->op1;
          ofs += 8*IR(ir->op2)->i;
          if (check_offset(ins, ofs + (ofs > 0 ? 4 : 0)) != OFS_INVALID) {
            *ofsp = ofs;
            return ra_alloc1(as, refa, allow);
          }
        }
      }
    } else if (ir->o == IR_HREFK) {
      if (mayfuse(as, ref)) {
        int32_t ofs = (int32_t)(IR(ir->op2)->op2 * sizeof(Node));
        if (check_offset(ins, ofs + (ofs > 0 ? 4 : 0)) != OFS_INVALID) {
          *ofsp = ofs;
          return ra_alloc1(as, ir->op1, allow);
        }
      }
    } else if (ir->o == IR_UREFC) {
      if (irref_isk(ir->op1)) {
	GCfunc *fn = ir_kfunc(IR(ir->op1));
	GCupval *uv = &gcref(fn->l.uvptr[(ir->op2 >> 8)])->uv;
	int64_t ofs = glofs(as, &uv->tv);
	if (checki32(ofs) && check_offset(ins, ofs)) {
	  *ofsp = (int32_t)ofs;
	  return RID_GL;
	}
      }
    }
  }
  *ofsp = 0;
  return ra_alloc1(as, ref, allow);
}

/* Fuse m operand into arithmetic/logic instructions. */
static uint32_t asm_fuseopm(ASMState *as, A64Ins ai, IRRef ref, RegSet allow)
{
  IRIns *ir = IR(ref);
  if (ra_hasreg(ir->r)) {
    ra_noweak(as, ir->r);
    return A64F_M(ir->r);
  } else if (irref_isk(ref)) {
    if((ai & 0x1f000000) == 0x0a000000) { // logical instruction ?
#if 0
      /* TODO: Implement emit_isk13 */
      uint32_t k = emit_isk13(ai, ir->i);
      if (k != -1)
        return k^A64I_BITOPk;
#endif
    } else {
      int64_t k;
      uint32_t m;
      if (ir->o == IR_KINT64)
	k = ir_kint64(ir)->u64;
      else if (ir->o == IR_KGC)
	k = (intptr_t)ir_kgc(ir);
      else if (ir->o == IR_KPTR || ir->o == IR_KKPTR)
	k = (intptr_t)ir_kptr(ir);
      else
	k = ir->i;
      if (emit_isk12(k, &m))
	return m;
    }
  } else if (mayfuse(as, ref)) {
    if (ir->o >= IR_BSHL && ir->o <= IR_BSAR) {
      Reg m = ra_alloc1(as, ir->op1, allow);
      A64Shift sh = ir->o == IR_BSHL ? A64SH_LSL :
		    ir->o == IR_BSHR ? A64SH_LSR : A64SH_ASR;
      if (irref_isk(ir->op2)) {
	return A64F_M(m) | A64F_SH(sh,
                            (IR(ir->op2)->i & (irt_is64(ir->t) ? 63 : 31)));
      }
    } else if (ir->o == IR_ADD && ir->op1 == ir->op2) {
     Reg m = ra_alloc1(as, ir->op1, allow);
     return A64F_M(m) | A64F_SH(A64SH_LSL, 1);
    }
  }
  return A64F_M(ra_allocref(as, ref, allow));
}

/* Fuse shifts into loads/stores. Only bother with BSHL 2 => lsl #2. */
static IRRef asm_fuselsl2(ASMState *as, IRRef ref)
{
    lua_unimpl();
    return 0;
}

/* Fuse XLOAD/XSTORE reference into load/store operand. */
static void asm_fusexref(ASMState *as, A64Ins ai, Reg rd, IRRef ref,
			 RegSet allow, int32_t ofs)
{
  IRIns *ir = IR(ref);
  Reg base;
  if (ra_noreg(ir->r) && canfuse(as, ir)) {
    if (ir->o == IR_ADD) {
      int32_t ofs2;
      if (asm_isk32(as, ir->op2, &ofs2) &&
	  check_offset(ai, (ofs2 = ofs + ofs2))) {
	ofs = ofs2;
	ref = ir->op1;
      }
    } else if (ir->o == IR_STRREF) {
      lua_assert(ofs == 0);
      int32_t ofs2;
      ofs = (int32_t)sizeof(GCstr);
      /*
       * TODO: This piece of code is an issue, because ofs+ofs2 could go into
       * overflow. And that should be handled gracefully, by allocating a
       * register for a 64-bit constant. That means that these offsets should
       * be of int64_t type as well. This also raises many other questions
       * like should emit_lso receive int64_t as offset as well, etc.
       */
      if (asm_isk32(as, ir->op2, &ofs2)) {
	ofs += ofs2;
	ref = ir->op1;
      } else if (asm_isk32(as, ir->op1, &ofs2)) {
	ofs += ofs2;
	ref = ir->op2;
      } else {
	/* NYI: Fuse ADD with constant. */
	Reg rn = ra_alloc1(as, ir->op1, allow);
	uint32_t m = asm_fuseopm(as, 0, ir->op2, rset_exclude(allow, rn));
	emit_lso(as, ai, rd, rd, ofs);
	emit_dn(as, A64I_ADDx^m, rd, rn);
	return;
      }
      if (!check_offset(ai, ofs)) {
	Reg rn = ra_alloc1(as, ref, allow);
	Reg rm = ra_allock(as, ofs, rset_exclude(allow, rn));
	/* Transform from unsigned immediate offset to register offset. */
	emit_dnm(as, (ai ^ 0x01204800), rd, rn, rm);
	return;
      }
    }
  }
  base = ra_alloc1(as, ref, allow);
  emit_lso(as, ai, (rd & 31), base, ofs);
}

/* Fuse to multiply-add/sub instruction. */
static int asm_fusemadd(ASMState *as, IRIns *ir, A64Ins ai, A64Ins air)
{
  IRRef lref = ir->op1, rref = ir->op2;
  IRIns *irm;
  if (lref != rref &&
      ((mayfuse(as, lref) && (irm = IR(lref), irm->o == IR_MUL) &&
        ra_noreg(irm->r)) ||
       (mayfuse(as, rref) && (irm = IR(rref), irm->o == IR_MUL) &&
        (rref = lref, ai = air, ra_noreg(irm->r))))) {
    Reg dest = ra_dest(as, ir, RSET_FPR);
    Reg add = ra_hintalloc(as, rref, dest, RSET_FPR);
    Reg right, left = ra_alloc2(as, irm,
                        rset_exclude(rset_exclude(RSET_FPR, dest), add));
    right = (left >> 8); left &= 255;
    emit_dnma(as, ai, (dest & 31), (left & 31), (right & 31), (add & 31));
    return 1;
  }
  return 0;
}

/* -- Calls --------------------------------------------------------------- */

/* Generate a call to a C function. */
static void asm_gencall(ASMState *as, const CCallInfo *ci, IRRef *args)
{
  uint32_t n, nargs = CCI_XNARGS(ci);
  int32_t ofs = 0;
  Reg gpr, fpr = REGARG_FIRSTFPR;

  if ((void *)ci->func) {
    emit_call(as, (void *)ci->func);
  }

  for (gpr = REGARG_FIRSTGPR; gpr <= REGARG_LASTGPR; gpr++)
    as->cost[gpr] = REGCOST(~0u, ASMREF_L);
  gpr = REGARG_FIRSTGPR;

  for (n = 0; n < nargs; n++) { /* Setup args. */
    IRRef ref = args[n];
    IRIns *ir = IR(ref);
    if (ref) {
      if (irt_isfp(ir->t)) {
        if (fpr <= REGARG_LASTFPR) {
          lua_assert(rset_test(as->freeset, fpr)); /* Must have been evicted. */
          ra_leftov(as, fpr, ref);
          fpr++;
        } else {
          Reg r = ra_alloc1(as, ref, RSET_FPR);
          emit_spstore(as, ir, r, ofs);
          ofs += 8;
        }
      } else {
        if (gpr <= REGARG_LASTGPR) {
          lua_assert(rset_test(as->freeset, gpr)); /* Must have been evicted. */
          ra_leftov(as, gpr, ref);
          gpr++;
        } else {
          Reg r = ra_alloc1(as, ref, RSET_GPR);
          emit_spstore(as, ir, r, ofs);
          ofs += 8;
        }
      }
    }
  }
}

/* Setup result reg/sp for call. Evict scratch regs. */
static void asm_setupresult(ASMState *as, IRIns *ir, const CCallInfo *ci)
{
  RegSet drop = RSET_SCRATCH;
  if (ra_hasreg(ir->r))
    rset_clear(drop, ir->r); /* Dest reg handled below. */
  ra_evictset(as, drop); /* Evictions must be performed first. */
  if (ra_used(ir)) {
    lua_assert(!irt_ispri(ir->t));
    if (irt_isfp(ir->t)) {
      if (ci->flags & CCI_CASTU64) {
        Reg dest = ra_dest(as, ir, RSET_FPR) & 31;
        if (irt_isnum(ir->t))
          emit_dn(as, A64I_FMOV_D_R, dest, RID_RET);
        else
          emit_dn(as, A64I_FMOV_S_R, dest, RID_RET);
      } else {
        ra_destreg(as, ir, RID_FPRET);
      }
    } else {
      ra_destreg(as, ir, RID_RET);
    }
  }
  UNUSED(ci);
}

static void asm_callx(ASMState *as, IRIns *ir)
{
  IRRef args[CCI_NARGS_MAX*2];
  CCallInfo ci;
  IRRef func;
  IRIns *irf;
  ci.flags = asm_callx_flags(as, ir);
  asm_collectargs(as, ir, &ci, args);
  asm_setupresult(as, ir, &ci);
  func = ir->op2; irf = IR(func);
  if (irf->o == IR_CARG) { func = irf->op1; irf = IR(func); }
  if (irref_isk(func)) {  /* Call to constant address. */
    ci.func = (ASMFunction)(ir_k64(irf)->u64);
  } else {
    /* Use LR for indirect calls. */
    emit_n(as, A64I_BLR, RID_LR);
    emit_loadu64(as, RID_LR, func);
    ci.func = (ASMFunction)(void *)0;
  }
  asm_gencall(as, &ci, args);
}

/* -- Returns ------------------------------------------------------------- */

/* Return to lower frame. Guard that it goes to the right spot. */
static void asm_retf(ASMState *as, IRIns *ir)
{
  Reg base = ra_alloc1(as, REF_BASE, RSET_GPR);
  Reg rpc = ra_scratch(as, rset_exclude(RSET_GPR, base));
  void *pc = ir_kptr(IR(ir->op2));
  int32_t delta = 1+LJ_FR2+bc_a(*((const BCIns *)pc - 1));
  as->topslot -= (BCReg)delta;
  if ((int32_t)as->topslot < 0) as->topslot = 0;
  irt_setmark(IR(REF_BASE)->t);  /* Children must not coalesce with BASE reg. */
  /* Need to force a spill on REF_BASE now to update the stack slot. */
  emit_lso(as, A64I_STRx, base, RID_SP, ra_spill(as, IR(REF_BASE)));
  emit_setgl(as, base, jit_base);
  emit_addptr(as, base, -8*delta);
  asm_guardcc(as, CC_NE);
  emit_nm(as, A64I_CMPx, RID_TMP, rpc);
  emit_loadu64(as, rpc, u64ptr(pc));
  emit_lso(as, A64I_LDRx, RID_TMP, base, -8);
}

/* -- Type conversions ---------------------------------------------------- */

static void asm_tointg(ASMState *as, IRIns *ir, Reg left)
{
  Reg tmp = ra_scratch(as, rset_exclude(RSET_FPR, left));
  Reg dest = ra_dest(as, ir, RSET_GPR);
  asm_guardcc(as, CC_NE);
  emit_nm(as, A64I_FCMPd, (tmp & 31), (left & 31));
  emit_dn(as, A64I_FCVT_F64_S32, (tmp & 31), dest);
  emit_dn(as, A64I_FCVT_S32_F64, dest, (left & 31));
}

static void asm_tobit(ASMState *as, IRIns *ir)
{
  RegSet allow = RSET_FPR;
  Reg left = ra_alloc1(as, ir->op1, allow);
  Reg right = ra_alloc1(as, ir->op2, rset_clear(allow, left));
  Reg tmp = ra_scratch(as, rset_clear(allow, right));
  Reg dest = ra_dest(as, ir, RSET_GPR);
  emit_dn(as, A64I_FMOV_R_S, dest, (tmp & 31));
  emit_dnm(as, A64I_FADDd, (tmp & 31), (left & 31), (right & 31));
}

static void asm_conv(ASMState *as, IRIns *ir)
{
  IRType st = (IRType)(ir->op2 & IRCONV_SRCMASK);
  int st64 = (st == IRT_I64 || st == IRT_U64 || st == IRT_P64);
  int stfp = (st == IRT_NUM || st == IRT_FLOAT);
  IRRef lref = ir->op1;
  lua_assert(irt_type(ir->t) != st);
  if (irt_isfp(ir->t)) {
    Reg dest = ra_dest(as, ir, RSET_FPR);
    if (stfp) {  /* FP to FP conversion. */
      emit_dn(as, st == IRT_NUM ? A64I_FCVT_F32_F64 : A64I_FCVT_F64_F32,
	      (dest & 31), (ra_alloc1(as, lref, RSET_FPR) & 31));
    } else {  /* Integer to FP conversion. */
      Reg left = ra_alloc1(as, lref, RSET_GPR);
      A64Ins ai = irt_isfloat(ir->t) ?
	(((IRT_IS64 >> st) & 1) ?
	(st == IRT_I64 ? A64I_FCVT_F32_S64 : A64I_FCVT_F32_U64) :
	(st == IRT_INT ? A64I_FCVT_F32_S32 : A64I_FCVT_F32_U32)) :
	(((IRT_IS64 >> st) & 1) ?
	(st == IRT_I64 ? A64I_FCVT_F64_S64 : A64I_FCVT_F64_U64) :
	(st == IRT_INT ? A64I_FCVT_F64_S32 : A64I_FCVT_F64_U32));
      emit_dn(as, ai, (dest & 31), left);
    }
  } else if (stfp) {  /* FP to integer conversion. */
    if (irt_isguard(ir->t)) {
      /* Checked conversions are only supported from number to int. */
      lua_assert(irt_isint(ir->t) && st == IRT_NUM);
      asm_tointg(as, ir, ra_alloc1(as, lref, RSET_FPR));
    } else {
      Reg left = ra_alloc1(as, lref, RSET_FPR);
      Reg dest = ra_dest(as, ir, RSET_GPR);
      A64Ins ai = irt_is64(ir->t) ?
	(st == IRT_NUM ?
	(irt_isi64(ir->t) ? A64I_FCVT_S64_F64 : A64I_FCVT_U64_F64) :
	(irt_isi64(ir->t) ? A64I_FCVT_S64_F32 : A64I_FCVT_U64_F32)) :
	(st == IRT_NUM ?
	(irt_isint(ir->t) ? A64I_FCVT_S32_F64 : A64I_FCVT_U32_F64) :
	(irt_isint(ir->t) ? A64I_FCVT_S32_F32 : A64I_FCVT_U32_F32));
      emit_dn(as, ai, dest, (left & 31));
    }
  } else if (st >= IRT_I8 && st <= IRT_U16) { /* Extend to 32 bit integer. */
    Reg dest = ra_dest(as, ir, RSET_GPR);
    Reg left = ra_alloc1(as, lref, RSET_GPR);
    lua_assert(irt_isint(ir->t) || irt_isu32(ir->t));
    A64Ins ai = st == IRT_I8 ? A64I_SXTBw :
		st == IRT_U8 ? A64I_UXTBw :
		st == IRT_I16 ? A64I_SXTHw : A64I_UXTHw;
    emit_dn(as, ai, dest, left);
  } else {
    Reg dest = ra_dest(as, ir, RSET_GPR);
    if (irt_is64(ir->t)) {
      if (st64 || !(ir->op2 & IRCONV_SEXT)) {
	/* 64/64 bit no-op (cast) or 32 to 64 bit zero extension. */
	ra_leftov(as, dest, lref);  /* Do nothing, but may need to move regs. */
      } else {  /* 32 to 64 bit sign extension. */
        Reg left = ra_alloc1(as, lref, RSET_GPR);
        emit_dn(as, A64I_SXTW, dest, left);
      }
    } else {
      if (st64) {
	/* This is either a 32 bit reg/reg mov which zeroes the hiword
	** or a load of the loword from a 64 bit address.
	*/
        Reg left = ra_alloc1(as, lref, RSET_GPR);
        emit_dm(as, A64I_MOVw, dest, left);
      } else {  /* 32/32 bit no-op (cast). */
	ra_leftov(as, dest, lref);  /* Do nothing, but may need to move regs. */
      }
    }
  }
}

static void asm_strto(ASMState *as, IRIns *ir)
{
  int destused = ra_used(ir);
  int32_t ofs;
  ra_evictset(as, RSET_SCRATCH);

  if (destused) {
    if (ra_hasspill(ir->s)) {
      ofs = sps_scale(ir->s);
      if (ra_hasreg(ir->r)) {
        ra_free(as, ir->r);
        ra_modified(as, ir->r);
        emit_spload(as, ir, ir->r, ofs);
      }
    } else {
      Reg r = ra_dest(as, ir, RSET_FPR);
      emit_lso(as, A64I_LDRd, r & 31, RID_SP, 0);
    }
  }

  asm_guardcc(as, CC_EQ);
  /* Test return status. 0 is failed. */
  emit_opk(as, A64I_SUBSw, RID_ZERO, RID_RET, 0, RSET_GPR);

  const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_strscan_num];
  IRRef args[2];
  args[0] = ir->op1; /* GCstr *str */
  args[1] = ASMREF_TMP1; /* TValue *n  */
  asm_gencall(as, ci, args);

  Reg tmp = ra_releasetmp(as, ASMREF_TMP1);
  if (ra_hasspill(ir->s))
    emit_opk(as, A64I_ADDx, tmp, RID_SP, ofs, RSET_GPR);
  else
    emit_dn(as, A64I_ADDx^A64I_BINOPk, tmp, RID_SP);
}

/* -- Memory references --------------------------------------------------- */

/* Get pointer to TValue. */
static void asm_tvptr(ASMState *as, Reg dest, IRRef ref)
{
  IRIns *ir = IR(ref);
  if (irt_isnum(ir->t)) {
    if (irref_isk(ref)) {
      /* Use the number constant itself as a TValue. */
      emit_loada(as, dest, ir_knum(ir));
    } else {
      /* Otherwise force a spill and use the spill slot. */
      emit_opk(as, A64I_ADDx, dest, RID_SP, ra_spill(as, ir), RSET_GPR);
    }
  } else {
    /* Otherwise use g->tmptv to hold the TValue. */
    RegSet allow = rset_exclude(RSET_GPR, dest);
    Reg src;
    if (irref_isk(ref)) {
      TValue k;
      lj_ir_kvalue(as->J->L, &k, ir);
      emit_lso(as, A64I_STRx, RID_TMP, dest, 0);
      emit_loadu64(as, RID_TMP, k.u64);
    } else {
      src = ra_alloc1(as, ref, allow);
      emit_lso(as, A64I_STRx, src, dest, 0);
      emit_dnm(as, A64I_ORRx, src, src, RID_TMP);
      if (irt_is64(ir->t)) {
        emit_loadu64(as, RID_TMP, ((uint64_t)irt_toitype(ir->t) << 47));
      } else {
        emit_loadu64(as, RID_TMP, ((uint64_t)irt_toitype(ir->t) << 47) | (0x7fffull << 32));
      }
    }
    emit_loada(as, dest, &J2G(as->J)->tmptv);
  }
}

static void asm_aref(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  Reg idx, base;
  if (irref_isk(ir->op2)) {
    IRRef tab = IR(ir->op1)->op1;
    int32_t ofs = asm_fuseabase(as, tab);
    IRRef refa = ofs ? tab : ir->op1;
    uint32_t k;
    if (emit_isk12(ofs + 8*IR(ir->op2)->i, &k)) {
      base = ra_alloc1(as, refa, RSET_GPR);
      emit_dn(as, A64I_ADDx^k, dest, base);
      return;
    }
  }
  base = ra_alloc1(as, ir->op1, RSET_GPR);
  idx = ra_alloc1(as, ir->op2, rset_exclude(RSET_GPR, base));
  emit_dnm(as, A64I_ADDx|A64F_SH(A64SH_LSL, 3), dest, base, idx);
}

/* Inlined hash lookup. Specialized for key type and for const keys.
** The equivalent C code is:
**   Node *n = hashkey(t, key);
**   do {
**     if (lj_obj_equal(&n->key, key)) return &n->val;
**   } while ((n = nextnode(n)));
**   return niltv(L);
*/
static void asm_href(ASMState *as, IRIns *ir, IROp merge)
{
  RegSet allow = RSET_GPR;
  int destused = ra_used(ir);
  Reg dest = ra_dest(as, ir, allow);
  Reg tab = ra_alloc1(as, ir->op1, rset_clear(allow, dest));
  Reg key = 0, tmp = RID_TMP;
  IRRef refkey = ir->op2;
  IRIns *irkey = IR(refkey);
  int isk12, isk = irref_isk(ir->op2);
  IRType1 kt = irkey->t;
  uint32_t k = 0;
  uint32_t khash;
  MCLabel l_end, l_loop, l_next;
  rset_clear(allow, tab);

  if (!isk) {
    key = ra_alloc1(as, ir->op2, irt_isnum(kt) ? RSET_FPR : allow);
    if (!irt_isstr(kt))
      tmp = ra_scratch(as, rset_exclude(allow, key));
  } else if (irt_isnum(kt)) {
    int64_t val = (int64_t)ir_knum(irkey)->u64;
    isk12 = emit_isk12(val, &k);
    if (!isk12) {
      /* TODO: Cannot use ra_allock for 64-bit constants. */
      key = ra_allock(as, val, allow);
      rset_clear(allow, key);
    }
  } else if (!irt_ispri(kt)) {
    isk12 = emit_isk12(irkey->i, &k);
    if (!isk12) {
      key = ra_alloc1(as, refkey, allow);
      rset_clear(allow, key);
    }
  }

  /* Key not found in chain: jump to exit (if merged) or load niltv. */
  l_end = emit_label(as);
  as->invmcp = NULL;
  if (merge == IR_NE)
    asm_guardcc(as, CC_AL);
  else if (destused)
    emit_loada(as, dest, niltvg(J2G(as->J)));

  /* Follow hash chain until the end. */
  l_loop = --as->mcp;
  emit_n(as, A64I_CMPx^A64I_BINOPk^0, dest);
  emit_lso(as, A64I_LDRx, dest, dest, offsetof(Node, next));
  l_next = emit_label(as);

  /* Type and value comparison. */
  if (merge == IR_EQ)
    asm_guardcc(as, CC_EQ);
  else
    emit_cond_branch(as, CC_EQ, l_end);

  if (irt_isnum(kt)) {
    if (isk) {
      /* Assumes -0.0 is already canonicalized to +0.0. */
      if (isk12)
        emit_n(as, A64I_CMPx^k, tmp);
      else
        emit_nm(as, A64I_CMPx, key, tmp);
      emit_lso(as, A64I_LDRx, tmp, dest, offsetof(Node, key.u64));
    } else {
      Reg tisnum = ra_scratch(as, rset_exclude(allow, tmp));
      Reg ftmp = ra_scratch(as, rset_exclude(RSET_FPR, key));
      emit_nm(as, A64I_FCMPd, key, ftmp);
      emit_dn(as, A64I_FMOV_D_R, (ftmp & 31), (tmp & 31));
      emit_cond_branch(as, CC_LO, l_next);
      emit_nm(as, A64I_CMPx|A64F_SH(A64SH_LSR, 32), tisnum, tmp);
      emit_loadi(as, tisnum, (LJ_TISNUM)<<15);
      emit_lso(as, A64I_LDRx, tmp, dest, offsetof(Node, key.n));
    }
  } else if (irt_isaddr(kt)) {
    Reg scr = ra_scratch(as, rset_exclude(allow, tmp));
    allow = rset_exclude(allow, scr);
    if (isk) {
      TValue ktv;
      ktv.u64 = ((uint64_t)irt_toitype(irkey->t) << 47) | irkey[1].tv.u64;
      emit_nm(as, A64I_CMPx, scr, tmp);
      emit_loadu64(as, scr, ktv.u64);
      emit_lso(as, A64I_LDRx, tmp, dest, offsetof(Node, key.u64));
    } else {
      emit_nm(as, A64I_CMPx, tmp, scr);
      emit_lso(as, A64I_LDRx, scr, dest, offsetof(Node, key.u64));
    }
  } else {
    /* TODO: not tested */
    lua_todo();
    lua_assert(irt_ispri(kt) && !irt_isnil(kt));
    Reg scr = ra_scratch(as, rset_exclude(allow, tmp));
    allow = rset_exclude(allow, scr);
  //  emit_u32(as, (irt_toitype(kt)<<15)|0x7fff);
  //  emit_rmro(as, XO_ARITHi, XOg_CMP, dest, offsetof(Node, key.it));
    emit_nm(as, A64I_CMPw, scr, tmp);
    emit_loadi(as, tmp, (irt_toitype(kt)<<15)|0x7fff);
    emit_lso(as, A64I_LDRw, scr, dest, offsetof(Node, key.it));
  }

  *l_loop = A64I_Bcond | (((as->mcp-l_loop) & 0x0007ffffu) << 5) | CC_NE;
  if (!isk && irt_isaddr(kt)) {
    emit_dnm(as, A64I_ORRx, tmp, tmp, key);
    emit_loadu64(as, tmp, (uint64_t)irt_toitype(kt) << 47);
  }
  /* Load main position relative to tab->node into dest. */
  khash = isk ? ir_khash(irkey) : 1;
  if (khash == 0) {
    emit_lso(as, A64I_LDRx, dest, tab, offsetof(GCtab, node));
  } else {
    emit_dnm(as, A64I_ADDx|A64F_SH(A64SH_LSL, 3), dest, tmp, dest);
    emit_dnm(as, A64I_ADDx|A64F_SH(A64SH_LSL, 1), dest, dest, dest);
    emit_lso(as, A64I_LDRx, tmp, tab, offsetof(GCtab, node));
    if (isk) {
      Reg tmphash = ra_allock(as, khash, allow);
      emit_dnm(as, A64I_ANDw, dest, dest, tmphash);
      emit_lso(as, A64I_LDRw, dest, tab, offsetof(GCtab, hmask));
    } else if (irt_isstr(kt)) {  /* Fetch of str->hash is cheaper than ra_allock. */
      emit_dnm(as, A64I_ANDw, dest, dest, tmp);
      emit_lso(as, A64I_LDRw, tmp, key, offsetof(GCstr, hash));
      emit_lso(as, A64I_LDRw, dest, tab, offsetof(GCtab, hmask));
    } else {  /* Must match with hash*() in lj_tab.c. */
      emit_dnm(as, A64I_ANDw, dest, dest, tmp);
      emit_lso(as, A64I_LDRw, tmp, tab, offsetof(GCtab, hmask));
      emit_dnm(as, A64I_SUBw, dest, dest, tmp);
      emit_dnm(as, A64I_EXTRw|((32-HASH_ROT3)<<10), tmp, tmp, tmp);
      emit_dnm(as, A64I_EORw, dest, dest, tmp);
      emit_dnm(as, A64I_EXTRw|((32-HASH_ROT2)<<10), dest, dest, dest);
      emit_dnm(as, A64I_SUBw, tmp, tmp, dest);
      emit_dnm(as, A64I_EXTRw|((32-HASH_ROT1)<<10), dest, dest, dest);
      emit_dnm(as, A64I_EORw, tmp, tmp, dest);
      if (irt_isnum(kt)) {
        emit_dnm(as, A64I_ADDw, dest, dest, dest);
        emit_dn(as, A64I_LSRx|(32<<16)|(32<<10), dest, dest);
        emit_dn(as, A64I_SXTW, tmp, dest);
        emit_dn(as, A64I_FMOV_R_D, (dest & 31), (key & 31));
      } else {
        emit_dm(as, A64I_MOVx, tmp, key); // TODO check
	emit_opk(as, A64I_ADDx, tmp, key, HASH_BIAS,
		 rset_exclude(rset_exclude(RSET_GPR, tab), key));
      }
    }
  }
}

static void asm_hrefk(ASMState *as, IRIns *ir)
{
  IRIns *kslot = IR(ir->op2);
  IRIns *irkey = IR(kslot->op1);
  int32_t ofs = (int32_t)(kslot->op2 * sizeof(Node));
  int32_t kofs = ofs + (int32_t)offsetof(Node, key);
  int large_ofs = check_offset(A64I_LDRx, ofs) == OFS_INVALID;
  Reg dest = (ra_used(ir) || large_ofs) ? ra_dest(as, ir, RSET_GPR) : RID_NONE;
  Reg node = ra_alloc1(as, ir->op1, RSET_GPR);
  Reg key = RID_NONE, idx = node;
  RegSet allow = rset_exclude(RSET_GPR, node);
  lua_assert(ofs % sizeof(Node) == 0);
  if (large_ofs) {
    idx = dest;
    rset_clear(allow, dest);
    kofs = (int32_t)offsetof(Node, key);
  } else if (ra_hasreg(dest)) {
    emit_opk(as, A64I_ADDx, dest, node, ofs, allow); /*!!!TODO w or x */
  }
  asm_guardcc(as, CC_NE);

  uint64_t key_val = 0;
  if (irt_ispri(irkey->t)) {
    lua_assert(!irt_isnil(irkey->t));
    /* primitive type's lowest 47 bits are all 1. */
    key_val = (uint64_t)irt_toitype(irkey->t) << 47 | ((1ULL << 47) - 1);
  } else if (irt_isnum(irkey->t))  {
    /* Assumes -0.0 is already canonicalized to +0.0. */
    key_val = ir_knum(irkey)->u64;
  } else {
    lua_assert(irt_isgcv(irkey->t));
    key_val = ((uint64_t)irt_toitype(irkey->t) << 47) | (uint64_t)ir_kgc(irkey);
  }

  key = ra_scratch(as, allow);
  /* TODO: Optimize to use less registers. */
  Reg temp = ra_scratch(as, rset_exclude(allow, key));
  emit_dnm(as, A64I_CMPx, 0, key, temp);
  emit_loadu64(as, temp, key_val);
  emit_lso(as, A64I_LDRx, key, idx, kofs);

  if (large_ofs) {
    emit_opk(as, A64I_ADDx, dest, node, ofs, RSET_GPR);
  }
}

static void asm_uref(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  if (irref_isk(ir->op1)) {
    GCfunc *fn = ir_kfunc(IR(ir->op1));
    MRef *v = &gcref(fn->l.uvptr[(ir->op2 >> 8)])->uv.v;
    emit_lsptr(as, A64I_LDRx, dest, v);
  } else {
    Reg uv = ra_scratch(as, RSET_GPR);
    Reg func = ra_alloc1(as, ir->op1, RSET_GPR);
    if (ir->o == IR_UREFC) {
      asm_guardcc(as, CC_NE);
      emit_n(as, A64I_CMPx^A64I_BINOPk^(1 << 10), RID_TMP);
      emit_opk(as, A64I_ADDx, dest, uv,
	       (int32_t)offsetof(GCupval, tv), RSET_GPR);
      emit_lso(as, A64I_LDRBw, RID_TMP, uv, (int32_t)offsetof(GCupval, closed));
    } else {
      emit_lso(as, A64I_LDRx, dest, uv, (int32_t)offsetof(GCupval, v));
    }
    emit_lso(as, A64I_LDRx, uv, func,
	     (int32_t)offsetof(GCfuncL, uvptr) + 8*(int32_t)(ir->op2 >> 8));
  }
}

static void asm_fref(ASMState *as, IRIns *ir)
{
  UNUSED(as); UNUSED(ir);
  lua_assert(!ra_used(ir));
}

static void asm_strref(ASMState *as, IRIns *ir)
{
  RegSet allow = RSET_GPR;
  Reg dest = ra_dest(as, ir, allow);
  int32_t ofs = sizeof(GCstr);
  Reg base = ra_alloc1(as, ir->op1, allow);
  uint8_t idx = RID_NONE;
  IRIns *irr = IR(ir->op2);
  rset_clear(allow, base);
  if (irref_isk(ir->op2)) {
    ofs += irr->i;
  } else {
    idx = ra_alloc1(as, ir->op2, allow);
    rset_clear(allow, idx);
  }
  if (idx == RID_NONE) {
    emit_opk(as, A64I_ADDx, dest, base, ofs, allow);
  } else {
    emit_opk(as, A64I_ADDx, dest, dest, ofs, allow);
    emit_dnm(as, A64I_ADDx, dest, base, idx);
  }
}

/* -- Loads and stores ---------------------------------------------------- */

static A64Ins asm_fxloadins(IRIns *ir)
{
  switch (irt_type(ir->t)) {
  case IRT_I8: return A64I_LDRBw ^ A64I_LS_S;
  case IRT_U8: return A64I_LDRBw;
  case IRT_I16: return A64I_LDRHw ^ A64I_LS_S;
  case IRT_U16: return A64I_LDRHw;
  case IRT_NUM: return A64I_LDRd;
  case IRT_FLOAT: return A64I_LDRs;
  default:
    if (irt_is64(ir->t))
      return A64I_LDRx;
    else
      return A64I_LDRw;
  }
}

static A64Ins asm_fxstoreins(IRIns *ir)
{
  switch (irt_type(ir->t)) {
  case IRT_I8: case IRT_U8: return A64I_STRBw;
  case IRT_I16: case IRT_U16: return A64I_STRHw;
  case IRT_NUM: return A64I_STRd;
  case IRT_FLOAT: return A64I_STRs;
  default:
    if (irt_is64(ir->t))
      return A64I_STRx;
    else
      return A64I_STRw;
  }
}

static void asm_fload(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  Reg idx;
  A64Ins ai = asm_fxloadins(ir);
  int32_t ofs;
  if (ir->op1 == REF_NIL) {
    idx = RID_GL;
    ofs = ir->op2 - GG_OFS(g);
  } else {
    idx = ra_alloc1(as, ir->op1, RSET_GPR);
    if (ir->op2 == IRFL_TAB_ARRAY) {
      ofs = asm_fuseabase(as, ir->op1);
      if (ofs) {  /* Turn the t->array load into an add for colocated arrays. */
        emit_dn(as, (A64I_ADDx^A64I_BINOPk)|(ofs << 10), dest, idx);
        return;
      }
    }
    ofs = field_ofs[ir->op2];
  }
  emit_lso(as, ai, dest & 31, idx, ofs);
}

static void asm_fstore(ASMState *as, IRIns *ir)
{
  if (ir->r != RID_SINK) {
    Reg idx, src = ra_alloc1(as, ir->op2, RSET_GPR);
    A64Ins ai = asm_fxstoreins(ir);
    int32_t ofs;
    IRIns *irf = IR(ir->op1);
    if (irf->op1 == REF_NIL) {
      idx = RID_GL;
      ofs = irf->op2 - GG_OFS(g);
    } else {
      idx = ra_alloc1(as, irf->op1, rset_exclude(RSET_GPR, src));
      ofs = field_ofs[irf->op2];
    }
    emit_lso(as, ai, src & 31, idx, ofs);
  }
}

static void asm_xload(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, irt_isfp(ir->t) ? RSET_FPR : RSET_GPR);
  lua_assert(!(ir->op2 & IRXLOAD_UNALIGNED));
  asm_fusexref(as, asm_fxloadins(ir), dest, ir->op1, RSET_GPR, 0);
}

static void asm_xstore_(ASMState *as, IRIns *ir, int32_t ofs)
{
  if (ir->r != RID_SINK) {
    Reg src = ra_alloc1(as, ir->op2,
			(!LJ_SOFTFP && irt_isfp(ir->t)) ? RSET_FPR : RSET_GPR);
    asm_fusexref(as, asm_fxstoreins(ir), src, ir->op1,
		 rset_exclude(RSET_GPR, src), ofs);
  }
}

#define asm_xstore(as, ir)	asm_xstore_(as, ir, 0)

static void asm_ahuvload(ASMState *as, IRIns *ir)
{
  Reg idx, tmp, type;
  int32_t ofs = 0;
  RegSet gpr = RSET_GPR, allow = irt_isnum(ir->t) ? RSET_FPR : RSET_GPR;
  lua_assert(irt_isnum(ir->t) || irt_ispri(ir->t) || irt_isaddr(ir->t) ||
	     irt_isint(ir->t));
  if (ra_used(ir)) {
    Reg dest = ra_dest(as, ir, allow);
    gpr = rset_exclude(gpr, dest);
    tmp = irt_isnum(ir->t) ? ra_scratch(as, gpr) : dest;
    if (irt_isaddr(ir->t)) {
      /* TODO: Implement AND. LSL->LSR is suboptimal. */
      emit_dn(as, A64I_LSRx|A64F_IR(17), dest, tmp);
      emit_dn(as, A64I_LSLx|A64F_IR((-17%64)&0x3f)|A64F_IS(63-17), tmp, tmp);
    } else if (irt_isnum(ir->t)) {
      emit_dn(as, A64I_FMOV_D_R, (dest & 31), tmp);
    } else if (irt_isint(ir->t)) {
      emit_dn(as, A64I_SXTW, dest, dest);
    }
  } else
    tmp = ra_scratch(as, gpr);
  gpr = rset_exclude(gpr, tmp);
  type = ra_scratch(as, gpr);
  gpr = rset_exclude(gpr, type);
  idx = asm_fuseahuref(as, ir->op1, &ofs, gpr, A64I_LDRx);
  gpr = rset_exclude(gpr, idx);
  /* Always do the type check, even if the load result is unused. */
  asm_guardcc(as, irt_isnum(ir->t) ? CC_LS : CC_NE);
  if (irt_type(ir->t) >= IRT_NUM) {
    lua_assert(irt_isinteger(ir->t) || irt_isnum(ir->t));
    emit_nm(as, A64I_CMPx|A64F_SH(A64SH_LSR, 32),
	    ra_allock(as, LJ_TISNUM << 15, gpr), tmp);
  } else if (irt_isaddr(ir->t)) {
    emit_opk(as, A64I_CMNx, 0, type, -irt_toitype(ir->t), gpr);
    emit_dn(as, A64I_ASRx|A64F_IR(47), type, tmp);
  } else if (irt_isnil(ir->t)) {
    emit_opk(as, A64I_CMNx, 0, tmp, 1, gpr);
  } else {
    emit_nm(as, A64I_CMPx|A64F_SH(A64SH_LSR, 32),
	    ra_allock(as, (irt_toitype(ir->t) << 15) | 0x7fff, allow), tmp);
  }
  emit_lso(as, A64I_LDRx, tmp, idx, ofs);
}

static void asm_ahustore(ASMState *as, IRIns *ir)
{
  if (ir->r != RID_SINK) {
    RegSet allow = RSET_GPR;
    Reg idx, src = RID_NONE, tmp = RID_NONE;
    int32_t ofs = 0;
    if (irt_isnum(ir->t)) {
      src = ra_alloc1(as, ir->op2, RSET_FPR);
      idx = asm_fuseahuref(as, ir->op1, &ofs, allow, A64I_STRd);
      emit_lso(as, A64I_STRd, (src & 31), idx, ofs);
    } else {
      if (!irt_ispri(ir->t)) {
	src = ra_alloc1(as, ir->op2, allow);
	rset_clear(allow, src);
      }
      tmp = ra_scratch(as, allow);
      idx = asm_fuseahuref(as, ir->op1, &ofs, rset_exclude(allow, tmp),
			   A64I_STRx);
      emit_lso(as, A64I_STRx, tmp, idx, ofs);
      if (ra_hasreg(src)) {
	if (irt_isinteger(ir->t)) {
	  /* Merge TISNUM with zero-extended integer. */
	  emit_dnm(as, A64I_ADDx|A64F_EX(A64EX_UXTW), tmp, tmp, src);
	  emit_d(as, A64I_MOVZx|(((LJ_TISNUM>>1)&0xffff)<<5)|
	         A64F_LSL16(48), tmp);
	} else {
	  emit_dnm(as, A64I_ADDx|A64F_SH(A64SH_LSL, 47), tmp, src, tmp);
	  emit_d(as, A64I_MOVNx|(((~irt_toitype(ir->t))&0xffff)<<5), tmp);
	}
      } else {
	/* TODO: We should probably allocate a register for these.
	 * But how should we allocate a register for a 64-bit constant? */
	IRType t = irt_type(ir->t);
	if (t == IRT_TRUE) {
	  emit_d(as, A64I_MOVNx|(0x0001<<5)|A64F_LSL16(48), tmp);
	} else if (t == IRT_FALSE) {
	  emit_d(as, A64I_MOVNx|(0x8000<<5)|A64F_LSL16(32), tmp);
	} else {
	  emit_d(as, A64I_MOVNx, tmp);
	}
      }
    }
  }
}

static void asm_sload(ASMState *as, IRIns *ir)
{
  int32_t ofs = 8*((int32_t)ir->op1-2);
  IRType1 t = ir->t;
  Reg dest = RID_NONE, base;
  RegSet allow = RSET_GPR;
  lua_assert(!(ir->op2 & IRSLOAD_PARENT));  /* Handled by asm_head_side(). */
  lua_assert(irt_isguard(t) || !(ir->op2 & IRSLOAD_TYPECHECK));
  if ((ir->op2 & IRSLOAD_CONVERT) && irt_isguard(t) && irt_isint(t)) {
    dest = ra_scratch(as, RSET_FPR);
    asm_tointg(as, ir, dest);
    t.irt = IRT_NUM;  /* Continue with a regular number type check. */
  } else if (ra_used(ir)) {
    Reg tmp = RID_NONE;
    if ((ir->op2 & IRSLOAD_CONVERT))
      tmp = ra_scratch(as, irt_isint(t) ? RSET_FPR : RSET_GPR);
    lua_assert((irt_isnum(t)) || irt_isint(t) || irt_isaddr(t));
    dest = ra_dest(as, ir, irt_isnum(t) ? RSET_FPR : allow);
    rset_clear(allow, dest);
    base = ra_alloc1(as, REF_BASE, allow);
    if (irt_isaddr(t)) {
      /* TODO: Implement AND. LSL->LSR is suboptimal. */
      emit_dn(as, A64I_LSRx|A64F_IR(17), dest, dest);
      emit_dn(as, A64I_LSLx|A64F_IR((-17%64)&0x3f)|A64F_IS(63-17), dest, dest);
    } else if ((ir->op2 & IRSLOAD_CONVERT)) {
      if (irt_isint(t)) {
	lua_todo();
	emit_dn(as, A64I_FCVT_S32_F64, dest, (tmp & 31));
	/* If value is already loaded for type check, move it to FPR. */
	if ((ir->op2 & IRSLOAD_TYPECHECK))
	  emit_dn(as, A64I_FMOV_D_R, (tmp & 31), dest);
	else
	  dest = tmp;
	t.irt = IRT_NUM;  /* Check for original type. */
      } else {
	emit_dn(as, A64I_FCVT_F64_S32, (dest & 31), tmp);
	dest = tmp;
	t.irt = IRT_INT;  /* Check for original type. */
      }
    } else if (irt_isint(t) && (ir->op2 & IRSLOAD_TYPECHECK)) {
      emit_dn(as, A64I_SXTW, dest, dest);
    }
    goto dotypecheck;
  }
  base = ra_alloc1(as, REF_BASE, allow);
dotypecheck:
  rset_clear(allow, base);
  if ((ir->op2 & IRSLOAD_TYPECHECK)) {
    Reg tmp;
    if (ra_hasreg(dest) && rset_test(RSET_GPR, dest)) {
      tmp = dest;
    } else {
      tmp = ra_scratch(as, allow);
      allow = rset_exclude(allow, tmp);
    }
    if (irt_isnum(t) && !(ir->op2 & IRSLOAD_CONVERT))
      emit_dn(as, A64I_FMOV_D_R, (dest & 31), tmp);
    /* Need type check, even if the load result is unused. */
    asm_guardcc(as, irt_isnum(t) ? CC_LS : CC_NE);
    if (irt_type(t) >= IRT_NUM) {
      lua_assert(irt_isinteger(t) || irt_isnum(t));
      emit_nm(as, A64I_CMPx|A64F_SH(A64SH_LSR, 32),
	      ra_allock(as, LJ_TISNUM << 15, allow), tmp);
    } else if (irt_isnil(t)) {
      emit_opk(as, A64I_CMNx, 0, tmp, 1, allow);
    } else if (irt_ispri(t)) {
      emit_nm(as, A64I_CMPx|A64F_SH(A64SH_LSR, 32),
	      ra_allock(as, (irt_toitype(t) << 15) | 0x7fff, allow), tmp);
    } else {
      Reg type = ra_scratch(as, allow);
      allow = rset_exclude(allow, type);
      emit_opk(as, A64I_CMNx, 0, type, -irt_toitype(t), allow);
      emit_dn(as, A64I_ASRx|A64F_IR(47), type, tmp);
    }
    /* TODO: Do we need to check if offset is out-of-range? */
    emit_lso(as, A64I_LDRx, tmp, base, ofs);
    return;
  }
  if (ra_hasreg(dest)) {
    /* TODO: Do we need to check if offset is out-of-range? */
    emit_lso(as, irt_isnum(t) ? A64I_LDRd :
	     (irt_isint(t) ? A64I_LDRw : A64I_LDRx), (dest & 31), base, ofs);
  }
}

/* -- Allocations --------------------------------------------------------- */

#if LJ_HASFFI
static void asm_cnew(ASMState *as, IRIns *ir)
{
  CTState *cts = ctype_ctsG(J2G(as->J));
  CTypeID id = (CTypeID)IR(ir->op1)->i;
  CTSize sz;
  CTInfo info = lj_ctype_info(cts, id, &sz);
  const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_mem_newgco];
  IRRef args[4];
  RegSet allow = (RSET_GPR & ~RSET_SCRATCH);
  lua_assert(sz != CTSIZE_INVALID || (ir->o == IR_CNEW && ir->op2 != REF_NIL));

  as->gcsteps++;
       asm_setupresult(as, ir, ci);  /* GCcdata * */

  /* Initialize immutable cdata object. */
  if (ir->o == IR_CNEWI) {
    int32_t ofs = sizeof(GCcdata);
    lua_assert(sz == 4 || sz == 8);
    Reg r = ra_alloc1(as, ir->op2, allow);
    emit_lso(as, A64I_STRx, r, RID_RET, ofs);
  } else if (ir->op2 != REF_NIL) {  /* Create VLA/VLS/aligned cdata. */
    ci = &lj_ir_callinfo[IRCALL_lj_cdata_newv];
    args[0] = ASMREF_L;     /* lua_State *L */
    args[1] = ir->op1;      /* CTypeID id   */
    args[2] = ir->op2;      /* CTSize sz    */
    args[3] = ASMREF_TMP1;  /* CTSize align */
    asm_gencall(as, ci, args);
    emit_loadi(as, ra_releasetmp(as, ASMREF_TMP1), (int32_t)ctype_align(info));
    return;
  }

  /* Initialize gct and ctypeid. lj_mem_newgco() already sets marked. */
 {
   int k = (id & 0xffff) ? 0 : 1;
   Reg r = k ? RID_X1 : ra_allock(as, id, allow);
   emit_lso(as, A64I_STRBw, RID_TMP, RID_RET, offsetof(GCcdata, gct));
   emit_lso(as, A64I_STRHw, r, RID_RET, offsetof(GCcdata, ctypeid));
   emit_d(as, A64I_MOVZw|A64F_U16(~LJ_TCDATA), RID_TMP);
   if (k) emit_d(as, A64I_MOVZw|A64F_U16(id), RID_X1);
 }
  args[0] = ASMREF_L;     /* lua_State *L */
  args[1] = ASMREF_TMP1;  /* MSize size   */
  asm_gencall(as, ci, args);
  ra_allockreg(as, (int32_t)(sz+sizeof(GCcdata)),
              ra_releasetmp(as, ASMREF_TMP1));
}
#else
#define asm_cnew(as, ir)	((void)0)
#endif

/* -- Write barriers ------------------------------------------------------ */

static void asm_tbar(ASMState *as, IRIns *ir)
{
  Reg tab = ra_alloc1(as, ir->op1, RSET_GPR);
  Reg link = ra_scratch(as, rset_exclude(RSET_GPR, tab));
  Reg gr = ra_scratch(as, rset_exclude(rset_exclude(RSET_GPR, tab), link));
  Reg mark = RID_TMP;
  MCLabel l_end = emit_label(as);
  emit_lso(as, A64I_STRx, link, tab, (int32_t)offsetof(GCtab, gclist));
  emit_lso(as, A64I_STRBw, mark, tab, (int32_t)offsetof(GCtab, marked));
  emit_lso(as, A64I_STRx, tab, gr,
	   (int32_t)offsetof(global_State, gc.grayagain));
  emit_dn(as, A64I_ANDIw|emit_isk13(A64I_ANDIw, ~LJ_GC_BLACK), mark, mark);
  emit_lso(as, A64I_LDRx, link, gr,
	   (int32_t)offsetof(global_State, gc.grayagain));
  emit_loada(as, gr, i64ptr(J2G(as->J)));
  emit_cond_branch(as, CC_EQ, l_end);
  emit_n(as, A64I_TSTIw|emit_isk13(A64I_TSTIw, LJ_GC_BLACK), mark);
  emit_lso(as, A64I_LDRBw, mark, tab, (int32_t)offsetof(GCtab, marked));
}

static void asm_obar(ASMState *as, IRIns *ir)
{
  const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_gc_barrieruv];
  IRRef args[2];
  MCLabel l_end;
  RegSet allow = RSET_GPR;
  Reg obj, val, tmp, black, white;
  /* No need for other object barriers (yet). */
  lua_assert(IR(ir->op1)->o == IR_UREFC);
  ra_evictset(as, RSET_SCRATCH);
  l_end = emit_label(as);
  args[0] = ASMREF_TMP1;  /* global_State *g */
  args[1] = ir->op1;      /* TValue *tv      */
  asm_gencall(as, ci, args);
  emit_loada(as, ra_releasetmp(as, ASMREF_TMP1), J2G(as->J));
  obj = IR(ir->op1)->r;
  tmp = ra_scratch(as, rset_exclude(allow, obj));
  black = ra_scratch(as, rset_exclude(allow, tmp));
  white = ra_scratch(as, rset_exclude(allow, black));
  emit_cond_branch(as, CC_EQ, l_end);
  lua_todo(); /* TODO: use emit_isk13 */
  emit_nm(as, A64I_TSTw, black, tmp);
  emit_cond_branch(as, CC_EQ, l_end);
  emit_nm(as, A64I_TSTw, white, RID_TMP);
  val = ra_alloc1(as, ir->op2, rset_exclude(RSET_GPR, obj));
  emit_loadi(as, white, LJ_GC_WHITES);
  emit_loadi(as, black, LJ_GC_BLACK);
  emit_lso(as, A64I_LDRBw, tmp, obj,
     (int32_t)offsetof(GCupval, marked)-(int32_t)offsetof(GCupval, tv));
  emit_lso(as, A64I_LDRBw, RID_TMP, val, (int32_t)offsetof(GChead, marked));
}

/* -- Arithmetic and logic operations ------------------------------------- */

static void asm_fparith(ASMState *as, IRIns *ir, A64Ins ai)
{
  Reg dest = ra_dest(as, ir, RSET_FPR);
  Reg right, left = ra_alloc2(as, ir, RSET_FPR);
  right = (left >> 8); left &= 255;
  emit_dnm(as, ai, (dest & 31), (left & 31), (right & 31));
}

static void asm_fpunary(ASMState *as, IRIns *ir, A64Ins ai)
{
  Reg dest = ra_dest(as, ir, RSET_FPR);
  Reg left = ra_hintalloc(as, ir->op1, dest, RSET_FPR);
  emit_dn(as, ai, (dest & 31), (left & 31));
}

static void asm_fpmath(ASMState *as, IRIns *ir)
{
  IRFPMathOp fpm = (IRFPMathOp)ir->op2;
  if (fpm == IRFPM_SQRT) {
    asm_fpunary(as, ir, A64I_FSQRTd);
  } else if (fpm <= IRFPM_TRUNC) {
    asm_fpunary(as, ir, fpm == IRFPM_FLOOR ? A64I_FRINTMd :
                        fpm == IRFPM_CEIL ? A64I_FRINTPd :
                                           A64I_FRINTZd);
  } else if (fpm == IRFPM_EXP2 && asm_fpjoin_pow(as, ir)) {
    return;
  } else {
    asm_callid(as, ir, IRCALL_lj_vm_floor + fpm);
  }
}

static int asm_swapops(ASMState *as, IRRef lref, IRRef rref)
{
  if (irref_isk(rref))
    return 0;  /* Don't swap constants to the left. */
  if (irref_isk(lref))
    return 1;  /* But swap constants to the right. */
  IRIns *ir = IR(rref);
  if ((ir->o >= IR_BSHL && ir->o <= IR_BSAR) ||
      (ir->o == IR_ADD && ir->op1 == ir->op2))
    return 0;  /* Don't swap fusable operands to the left. */
  ir = IR(lref);
  if ((ir->o >= IR_BSHL && ir->o <= IR_BSAR) ||
      (ir->o == IR_ADD && ir->op1 == ir->op2))
    return 1;  /* But swap fusable operands to the right. */
  return 0;  /* Otherwise don't swap. */
}

static void asm_intop(ASMState *as, IRIns *ir, A64Ins ai)
{
  IRRef lref = ir->op1, rref = ir->op2;
  Reg left, dest = ra_dest(as, ir, RSET_GPR);
  uint32_t m;
  if (asm_swapops(as, lref, rref) &&
      (ai & ~A64I_S & ~A64I_X) != A64I_SUBw) {
    IRRef tmp = lref; lref = rref; rref = tmp;
  }
  left = ra_hintalloc(as, lref, dest, RSET_GPR);
  m = asm_fuseopm(as, ai, rref, rset_exclude(RSET_GPR, left));
  if (irt_isguard(ir->t)) {  /* For IR_ADDOV etc. */
    asm_guardcc(as, CC_VS);
    ai |= A64I_S;
  }
  emit_dn(as, ai^m, dest, left);
}

static void asm_intop_s(ASMState *as, IRIns *ir, A64Ins ai)
{
  if (as->flagmcp == as->mcp) {  /* Drop cmp r, #0. */
    as->flagmcp = NULL;
    as->mcp++;
    ai |= A64I_S;
  }
  asm_intop(as, ir, ai);
}

static void asm_intneg(ASMState *as, IRIns *ir, A64Ins ai)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  Reg left = ra_hintalloc(as, ir->op1, dest, RSET_GPR);
  emit_dm(as, ai, dest, left);
}

/* NYI: use add/shift for MUL(OV) with constants. FOLD only does 2^k. */
static void asm_intmul(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  Reg left = ra_alloc1(as, ir->op1, rset_exclude(RSET_GPR, dest));
  Reg right = ra_alloc1(as, ir->op2, rset_exclude(RSET_GPR, left));
  if (irt_isguard(ir->t)) {  /* IR_MULOV */
    asm_guardcc(as, CC_NE);
    emit_nm(as, A64I_CMPw|A64F_SH(A64SH_ASR, 31), RID_TMP, dest);
    emit_dn(as, A64I_ASRx|A64F_IR(32), RID_TMP, dest);
    emit_dnm(as, A64I_SMULL, dest, right, left);
  } else {
    emit_dnm(as, A64I_MULx, dest, left, right);
  }
}

static void asm_add(ASMState *as, IRIns *ir)
{
  if (irt_isnum(ir->t)) {
    if (!asm_fusemadd(as, ir, A64I_FMADDd, A64I_FMADDd))
      asm_fparith(as, ir, A64I_FADDd);
    return;
  }
  asm_intop_s(as, ir, irt_is64(ir->t) ? A64I_ADDx : A64I_ADDw);
}

static void asm_sub(ASMState *as, IRIns *ir)
{
  if (irt_isnum(ir->t)) {
    asm_fparith(as, ir, A64I_FSUBd);
    return;
  }
  asm_intop_s(as, ir, irt_is64(ir->t) ? A64I_SUBx : A64I_SUBw);
}

static void asm_mul(ASMState *as, IRIns *ir)
{
  if (irt_isnum(ir->t)) {
    asm_fparith(as, ir, A64I_FMULd);
    return;
  }
  asm_intmul(as, ir);
}

static void asm_div(ASMState *as, IRIns *ir)
{
#if LJ_64 && LJ_HASFFI
  if (!irt_isnum(ir->t))
    asm_callid(as, ir, irt_isi64(ir->t) ? IRCALL_lj_carith_divi64 :
					  IRCALL_lj_carith_divu64);
  else
#endif
    asm_fparith(as, ir, A64I_FDIVd);
}

static void asm_pow(ASMState *as, IRIns *ir)
{
#if LJ_64 && LJ_HASFFI
  if (!irt_isnum(ir->t))
    asm_callid(as, ir, irt_isi64(ir->t) ? IRCALL_lj_carith_powi64 :
					  IRCALL_lj_carith_powu64);
  else
#endif
    asm_callid(as, ir, IRCALL_lj_vm_powi);
}

#define asm_addov(as, ir)	asm_add(as, ir)
#define asm_subov(as, ir)	asm_sub(as, ir)
#define asm_mulov(as, ir)	asm_mul(as, ir)

#define asm_abs(as, ir)		asm_fpunary(as, ir, A64I_FABS)
#define asm_atan2(as, ir)	asm_callid(as, ir, IRCALL_atan2)
#define asm_ldexp(as, ir)	asm_callid(as, ir, IRCALL_ldexp)

static void asm_mod(ASMState *as, IRIns *ir)
{
#if LJ_64 && LJ_HASFFI
  if (!irt_isint(ir->t))
    asm_callid(as, ir, irt_isi64(ir->t) ? IRCALL_lj_carith_modi64 :
					  IRCALL_lj_carith_modu64);
  else
#endif
    asm_callid(as, ir, IRCALL_lj_vm_modi);
}

static void asm_neg(ASMState *as, IRIns *ir)
{
  if (irt_isnum(ir->t)) {
    asm_fpunary(as, ir, A64I_FNEGd);
    return;
  }
  asm_intneg(as, ir, irt_is64(ir->t) ? A64I_NEGx : A64I_NEGw);
}

static void asm_bitop(ASMState *as, IRIns *ir, A64Ins ai)
{
  if (as->flagmcp == as->mcp) {  /* Try to drop cmp r, #0. */
    uint32_t cc = (as->mcp[1] >> 28);
    as->flagmcp = NULL;
    if (cc <= CC_NE) {
      as->mcp++;
      ai |= A64I_S;
    } else if (cc == CC_GE) {
      *++as->mcp ^= ((CC_GE^CC_PL) << 28);
      ai |= A64I_S;
    } else if (cc == CC_LT) {
      *++as->mcp ^= ((CC_LT^CC_MI) << 28);
      ai |= A64I_S;
    }  /* else: other conds don't work with bit ops. */
  }
  if (ir->op2 == 0) {
    Reg dest = ra_dest(as, ir, RSET_GPR);
    uint32_t m = asm_fuseopm(as, ai, ir->op1, RSET_GPR);
    emit_d(as, ai^m, dest);
  } else {
    asm_intop(as, ir, ai);
  }
}

#define asm_bnot(as, ir) \
  asm_bitop(as, ir, irt_is64(ir->t) ? A64I_MVNx : A64I_MVNw)

static void asm_bswap(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  Reg left = ra_alloc1(as, ir->op1, RSET_GPR);
  emit_dn(as, irt_is64(ir->t) ? A64I_REVx : A64I_REVw, dest, left);
}

#define asm_band(as, ir) \
  asm_bitop(as, ir, irt_is64(ir->t) ? A64I_ANDx : A64I_ANDw)
#define asm_bor(as, ir) \
  asm_bitop(as, ir, irt_is64(ir->t) ? A64I_ORRx : A64I_ORRw)
#define asm_bxor(as, ir) \
  asm_bitop(as, ir, irt_is64(ir->t) ? A64I_EORx : A64I_EORw)

static void asm_bitshift(ASMState *as, IRIns *ir, A64Ins ai, A64Shift sh)
 {
  if (irref_isk(ir->op2)) {  /* Constant shifts. */
    Reg dest = ra_dest(as, ir, RSET_GPR);
    Reg left = ra_alloc1(as, ir->op1, RSET_GPR);
    int32_t shift = (IR(ir->op2)->i & (irt_is64(ir->t) ? 63 : 31));
    lua_assert(shift>=0 && shift<(irt_is64(ir->t) ? 64 : 32));
    int32_t immr, imms;

    switch(sh) {
    case A64SH_LSL:
      imms = irt_is64(ir->t) ? 63-shift : 31-shift;
      immr = imms+1;
      emit_dn(as, ai|(imms<<10)|(immr<<16), dest, left);
      break;
    case A64SH_LSR: case A64SH_ASR:
      if(!irt_is64(ir->t)) {
        emit_dn(as, ai|(31<<10)|(shift<<16), dest, left);
      } else {
        emit_dn(as, ai|(63<<10)|(shift<<16), dest, left);
      }
      break;
    case A64SH_ROR:
      emit_dnm(as, ai|(shift<<10), dest, left, left);
      break;
    }
  } else {
    A64Ins ai = irt_is64(ir->t) ? A64I_SHRx : A64I_SHRw;
    Reg dest = ra_dest(as, ir, RSET_GPR);
    Reg left = ra_alloc1(as, ir->op1, RSET_GPR);
    Reg right = ra_alloc1(as, ir->op2, rset_exclude(RSET_GPR, left));
    emit_dnm(as, ai|A64F_BSH(sh), dest, left, right);
  }
 }

#define asm_bshl(as, ir) \
  asm_bitshift(as, ir, irt_is64(ir->t) ? A64I_UBFMx : A64I_UBFMw, A64SH_LSL)
#define asm_bshr(as, ir) \
  asm_bitshift(as, ir, irt_is64(ir->t) ? A64I_UBFMx : A64I_UBFMw, A64SH_LSR)
#define asm_bsar(as, ir) \
  asm_bitshift(as, ir, irt_is64(ir->t) ? A64I_SBFMx : A64I_SBFMw, A64SH_ASR)
#define asm_bror(as, ir) \
  asm_bitshift(as, ir, irt_is64(ir->t) ? A64I_EXTRx : A64I_EXTRw, A64SH_ROR)
#define asm_brol(as, ir)       lua_assert(0)

static void asm_intmin_max(ASMState *as, IRIns *ir, A64CC cc)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  Reg left = ra_hintalloc(as, ir->op1, dest, RSET_GPR);
  Reg right = ra_alloc1(as, ir->op2, rset_exclude(RSET_GPR, left));

  // !!!TODO: optimize - check if op2 is immediate

  emit_dnm(as, A64I_CSELx|A64F_COND(cc), dest, left, right);
  emit_nm(as, A64I_CMPw, left, right);
}

static void asm_fpmin_max(ASMState *as, IRIns *ir, A64CC fcc)
{
  Reg dest = (ra_dest(as, ir, RSET_FPR) & 31);
  Reg right, left = ra_alloc2(as, ir, RSET_FPR);
  right = ((left >> 8) & 31); left &= 31;

  emit_dnm(as, A64I_FCSELd|A64F_COND(fcc), dest, left, right);
  emit_nm(as, A64I_FCMPd, left, right);
}

static void asm_min_max(ASMState *as, IRIns *ir, A64CC cc, A64CC fcc)
{
  if (irt_isnum(ir->t))
    asm_fpmin_max(as, ir, fcc);
  else
    asm_intmin_max(as, ir, cc);
}

#define asm_max(as, ir)		asm_min_max(as, ir, CC_GT, CC_HI)
#define asm_min(as, ir)		asm_min_max(as, ir, CC_LT, CC_LO)

/* -- Comparisons --------------------------------------------------------- */

/* Map of comparisons to flags. ORDER IR. */
static const uint8_t asm_compmap[IR_ABC+1] = {
  /* op  FP swp  int cc   FP cc */
  /* LT       */ CC_GE + (CC_HS << 4),
  /* GE    x  */ CC_LT + (CC_HI << 4),
  /* LE       */ CC_GT + (CC_HI << 4),
  /* GT    x  */ CC_LE + (CC_HS << 4),
  /* ULT   x  */ CC_HS + (CC_LS << 4),
  /* UGE      */ CC_LO + (CC_LO << 4),
  /* ULE   x  */ CC_HI + (CC_LO << 4),
  /* UGT      */ CC_LS + (CC_LS << 4),
  /* EQ       */ CC_NE + (CC_NE << 4),
  /* NE       */ CC_EQ + (CC_EQ << 4),
  /* ABC      */ CC_LS + (CC_LS << 4)  /* Same as UGT. */
};

/* FP comparisons. */
static void asm_fpcomp(ASMState *as, IRIns *ir)
{
  Reg left, right;
  A64Ins ai;
  int swp = ((ir->o ^ (ir->o >> 2)) & ~(ir->o >> 3) & 1);
  if (!swp && irref_isk(ir->op2) && ir_knum(IR(ir->op2))->u64 == 0) {
    left = (ra_alloc1(as, ir->op1, RSET_FPR) & 31);
    right = 0;
    ai = A64I_FCMPZd;
  } else {
    left = ra_alloc2(as, ir, RSET_FPR);
    if (swp) {
      right = (left & 31); left = ((left >> 8) & 31);
    } else {
      right = ((left >> 8) & 31); left &= 31;
    }
    ai = A64I_FCMPd;
  }
  asm_guardcc(as, (asm_compmap[ir->o] >> 4));
  emit_nm(as, ai, left, right);
}

/* Integer comparisons. */
static void asm_intcomp(ASMState *as, IRIns *ir)
{
  A64CC cc = (asm_compmap[ir->o] & 15);
  A64Ins ai = irt_is64(ir->t) ? A64I_CMPx : A64I_CMPw;
  IRRef lref = ir->op1, rref = ir->op2;
  Reg left;
  uint32_t m;
  int cmpprev0 = 0;
  lua_assert(irt_is64(ir->t) || irt_isint(ir->t) ||
	     irt_isu32(ir->t) || irt_isaddr(ir->t) || irt_isu8(ir->t));

  /* !!!TODO ARM exploits TST here for comp(BAND(left, right) */

  left = ra_alloc1(as, lref, RSET_GPR);

  m = asm_fuseopm(as, ai, rref, rset_exclude(RSET_GPR, left));
  asm_guardcc(as, cc);
  emit_n(as, ai^m, left);

  /* !!!TODO we should do this so that the compare can be elided by
     making the instruction before flagsetting */
  /* Signed comparison with zero and referencing previous ins? */
  if (cmpprev0 && (cc <= CC_NE || cc >= CC_GE))
    as->flagmcp = as->mcp;  /* Allow elimination of the compare. */

}

static void asm_comp(ASMState *as, IRIns *ir)
{
  if (irt_isnum(ir->t))
    asm_fpcomp(as, ir);
  else
    asm_intcomp(as, ir);
}

#define asm_equal(as, ir)	asm_comp(as, ir)

/* -- Support for 64 bit ops in 32 bit mode ------------------------------- */

/* Hiword op of a split 64 bit op. Previous op must be the loword op. */
static void asm_hiop(ASMState *as, IRIns *ir)
{
  UNUSED(as); UNUSED(ir); lua_assert(0);  /* Unused on 64 bit or without FFI. */
}

/* -- Profiling ----------------------------------------------------------- */

static void asm_prof(ASMState *as, IRIns *ir)
{
  uint32_t k;
  UNUSED(ir);
  asm_guardcc(as, CC_NE);
  k = emit_isk13(A64I_TSTIw, HOOK_PROFILE);
  lua_assert(k != (uint32_t)-1);
  emit_n(as, A64I_TSTIw | k, RID_TMP);
  emit_lsptr(as, A64I_LDRBw, RID_TMP, (void *)&J2G(as->J)->hookmask);
}

/* -- Stack handling ------------------------------------------------------ */

/* Check Lua stack size for overflow. Use exit handler as fallback. */
static void asm_stack_check(ASMState *as, BCReg topslot,
			    IRIns *irp, RegSet allow, ExitNo exitno)
{
  Reg pbase;
  uint32_t k;
  if (irp) {
    if (!ra_hasspill(irp->s)) {
      pbase = irp->r;
      lua_assert(ra_hasreg(pbase));
    } else if (allow) {
      pbase = rset_pickbot(allow);
    } else {
      pbase = RID_RET;
      emit_lso(as, A64I_LDRx, RID_RET, RID_SP, 0);  /* Restore temp. register. */
    }
  } else {
    pbase = RID_BASE;
  }
  emit_branch(as, A64I_BL, exitstub_addr(as->J, exitno));
  emit_cond_branch(as, CC_LS^1, as->mcp+1);
  emit_isk12((8*topslot), &k);
  emit_n(as, A64I_CMPx^k, RID_TMP);
  emit_dnm(as, A64I_SUBx, RID_TMP, RID_TMP, pbase);
  emit_lso(as, A64I_LDRx, RID_TMP, RID_TMP,
	   (int32_t)offsetof(lua_State, maxstack));
  if (irp) {  /* Must not spill arbitrary registers in head of side trace. */
    if (ra_hasspill(irp->s))
      emit_lso(as, A64I_LDRx, pbase, RID_SP, sps_scale(irp->s));
    emit_lso(as, A64I_LDRx, RID_TMP, RID_GL, glofs(as, &J2G(as->J)->cur_L));
    if (ra_hasspill(irp->s) && !allow)
      emit_lso(as, A64I_STRx, RID_RET, RID_SP, 0);  /* Save temp. register. */
  } else {
    emit_getgl(as, RID_TMP, cur_L);
  }
}

/* Restore Lua stack from on-trace state. */
static void asm_stack_restore(ASMState *as, SnapShot *snap)
{
  SnapEntry *map = &as->T->snapmap[snap->mapofs];
  SnapEntry *flinks = &as->T->snapmap[snap_nextofs(as->T, snap)-1-LJ_FR2];
  MSize n, nent = snap->nent;
  /* Store the value of all modified slots to the Lua stack. */
  for (n = 0; n < nent; n++) {
    SnapEntry sn = map[n];
    BCReg s = snap_slot(sn);
    int32_t ofs = 8*((int32_t)s-1-LJ_FR2);
    IRRef ref = snap_ref(sn);
    IRIns *ir = IR(ref);
    if ((sn & SNAP_NORESTORE))
      continue;
    if (irt_isnum(ir->t)) {
      Reg src = ra_alloc1(as, ref, RSET_FPR);
      emit_lso(as, A64I_STRd, src & 31, RID_BASE, ofs);
    } else {
      lua_assert(irt_ispri(ir->t) || irt_isaddr(ir->t) ||
                 (LJ_DUALNUM && irt_isinteger(ir->t)));
      if (!irref_isk(ref)) {
        Reg src = ra_alloc1(as, ref, rset_exclude(RSET_GPR, RID_BASE));
        if (irt_is64(ir->t)) {
	  /* TODO: 64 bit store + 32 bit load-modify-store is suboptimal. */
	  Reg type = ra_allock(as, irt_toitype(ir->t)<<15, RSET_GPR);
	  emit_lso(as, A64I_STRw, RID_TMP, RID_BASE, ofs+4);
	  emit_dnm(as, A64I_ORRw, RID_TMP, RID_TMP, type);
	  emit_lso(as, A64I_LDRw, RID_TMP, RID_BASE, ofs+4);

          //emit_u32(as, irt_toitype(ir->t) << 15);
          //emit_rmro(as, XO_ARITHi, XOg_OR, RID_BASE, ofs+4);
        } else if (LJ_DUALNUM && irt_isinteger(ir->t)) {
          emit_lso(as, A64I_STRw, RID_TMP, RID_BASE, ofs+4);
          emit_loadi(as, RID_TMP, LJ_TISNUM << 15);

          //emit_movmroi(as, RID_BASE, ofs+4, LJ_TISNUM << 15);
        } else {
          emit_lso(as, A64I_STRw, RID_TMP, RID_BASE, ofs+4);
          emit_loadi(as, RID_TMP, (irt_toitype(ir->t)<<15)|0x7fff);

          //emit_movmroi(as, RID_BASE, ofs+4, (irt_toitype(ir->t)<<15)|0x7fff);
        }
        if (irt_is64(ir->t))
        {
          emit_lso(as, A64I_STRx, src, RID_BASE, ofs);
          //emit_movtomro(as, REX_64IR(ir, src), RID_BASE, ofs);
        } else {
          emit_lso(as, A64I_STRw, src, RID_BASE, ofs);
          //emit_movtomro(as, REX_64IR(ir, src), RID_BASE, ofs);
        }
      } else {
        TValue k;
        lj_ir_kvalue(as->J->L, &k, ir);
        if (tvisnil(&k)) {
          emit_lso(as, A64I_STRx, RID_TMP, RID_BASE, ofs);
          // !!!TODO (uint32_t)-1 or (uint64_t)-1
          emit_loadu64(as, RID_TMP, 0xffffffffffffffff);
          //emit_i32(as, -1);
          //emit_rmro(as, XO_MOVmi, REX_64, RID_BASE, ofs);
        } else {
          emit_lso(as, A64I_STRx, RID_TMP, RID_BASE, ofs);
          emit_loadu64(as, RID_TMP, k.u64);
          //emit_movmroi(as, RID_BASE, ofs+4, k.u32.hi);
          //emit_movmroi(as, RID_BASE, ofs, k.u32.lo);
        }
      }
    }
    checkmclim(as);
  }
  lua_assert(map + nent == flinks);
}

/* -- GC handling --------------------------------------------------------- */

/* Check GC threshold and do one or more GC steps. */
static void asm_gc_check(ASMState *as)
{
  const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_gc_step_jit];
  IRRef args[2];
  MCLabel l_end;
  Reg tmp1, tmp2;
  ra_evictset(as, RSET_SCRATCH);
  l_end = emit_label(as);
  /* Exit trace if in GCSatomic or GCSfinalize. Avoids syncing GC objects. */
  asm_guardcc(as, CC_NE);  /* Assumes asm_snap_prep() already done. */
  emit_dn(as, A64I_CMPx^A64I_BINOPk, RID_ZERO, RID_RET);
  args[0] = ASMREF_TMP1;  /* global_State *g */
  args[1] = ASMREF_TMP2;  /* MSize steps     */
  asm_gencall(as, ci, args);
  tmp1 = ra_releasetmp(as, ASMREF_TMP1);
  tmp2 = ra_releasetmp(as, ASMREF_TMP2);
  emit_loadi(as, tmp2, as->gcsteps);
  /* Jump around GC step if GC total < GC threshold. */
  emit_cond_branch(as, CC_LS, l_end);
  emit_nm(as, A64I_CMPx, RID_TMP, tmp2);
  emit_lso(as, A64I_LDRx, tmp2, tmp1,
	   (int32_t)offsetof(global_State, gc.threshold));
  emit_lso(as, A64I_LDRx, RID_TMP, tmp1,
	   (int32_t)offsetof(global_State, gc.total));
  /* TODO: Should we allocate register for this? */
  emit_loadu64(as, tmp1, u64ptr(J2G(as->J)));
  as->gcsteps = 0;
  checkmclim(as);
}

/* -- Loop handling ------------------------------------------------------- */

/* Fixup the loop branch. */
static void asm_loop_fixup(ASMState *as)
{
  MCode *p = as->mctop;
  MCode *target = as->mcp;
  if (as->loopinv) {  /* Inverted loop branch? */
    ptrdiff_t delta = target - (p - 2);
    lua_assert(((delta + 0x40000) >> 19) == 0);
    /* asm_guardcc already inverted the bcc and patched the final bl. */
    p[-2] |= ((uint32_t)delta & 0x7ffff) << 5;
  } else {
    ptrdiff_t delta = target - (p - 1);
    p[-1] = A64I_B | ((uint32_t)(delta) & 0x03ffffffu);
  }
}

/* -- Head of trace ------------------------------------------------------- */

/* Reload L register from g->cur_L. */
static void asm_head_lreg(ASMState *as)
{
  IRIns *ir = IR(ASMREF_L);
  if (ra_used(ir)) {
    Reg r = ra_dest(as, ir, RSET_GPR);
    emit_getgl(as, r, cur_L);
    ra_evictk(as);
  }
}

/* Coalesce BASE register for a root trace. */
static void asm_head_root_base(ASMState *as)
{
  IRIns *ir;
  asm_head_lreg(as);
  ir = IR(REF_BASE);
  if (ra_hasreg(ir->r) && (rset_test(as->modset, ir->r) || irt_ismarked(ir->t)))
    ra_spill(as, ir);
  ra_destreg(as, ir, RID_BASE);
}

/* Coalesce BASE register for a side trace. */
static RegSet asm_head_side_base(ASMState *as, IRIns *irp, RegSet allow)
{
  IRIns *ir;
  asm_head_lreg(as);
  ir = IR(REF_BASE);
  if (ra_hasreg(ir->r) && (rset_test(as->modset, ir->r) || irt_ismarked(ir->t)))
    ra_spill(as, ir);
  if (ra_hasspill(irp->s)) {
    rset_clear(allow, ra_dest(as, ir, allow));
  } else {
    Reg r = irp->r;
    lua_assert(ra_hasreg(r));
    rset_clear(allow, r);
    if (r != ir->r && !rset_test(as->freeset, r))
      ra_restore(as, regcost_ref(as->cost[r]));
    ra_destreg(as, ir, r);
  }
  return allow;
}

/* -- Tail of trace ------------------------------------------------------- */

/* Fixup the tail code. */
static void asm_tail_fixup(ASMState *as, TraceNo lnk)
{
  MCode *p = as->mctop;
  MCode *target;
  int32_t spadj = as->T->spadjust;
  if (spadj == 0) {
    as->mctop = --p;
  } else {
    /* Patch stack adjustment. */
    uint32_t k;
    emit_isk12(spadj, &k);
    p[-2] = (A64I_ADDx^k) | A64F_D(RID_SP) | A64F_N(RID_SP);
  }
  /* Patch exit branch. */
  target = lnk ? traceref(as->J, lnk)->mcode : (MCode *)lj_vm_exit_interp;
  p[-1] = A64I_B|(((target-p)+1)&0x03ffffffu);
}

/* Prepare tail of code. */
static void asm_tail_prep(ASMState *as)
{
  /* !!!TODO shared with ARM */
  MCode *p = as->mctop - 1;  /* Leave room for exit branch. */
  if (as->loopref) {
    as->invmcp = as->mcp = p;
  } else {
    as->mcp = p-1;  /* Leave room for stack pointer adjustment. */
    as->invmcp = NULL;
  }
  *p = 0;  /* Prevent load/store merging. */
}

/* -- Trace setup --------------------------------------------------------- */

/* Ensure there are enough stack slots for call arguments. */
static Reg asm_setup_call_slots(ASMState *as, IRIns *ir, const CCallInfo *ci)
{
  IRRef args[CCI_NARGS_MAX * 2];
  uint32_t i, nargs = CCI_XNARGS(ci);
  int nslots = 0, ngpr = REGARG_NUMGPR, nfpr = REGARG_NUMFPR;
  asm_collectargs(as, ir, ci, args);
  for (i = 0; i < nargs; i++) {
    // TODO: check the case (ci->flags & CCI_VARARG)
    if (args[i] && irt_isfp(IR(args[i])->t)) {
      if (nfpr > 0) {
        nfpr--;
      } else {
        nslots += 2;
      }
    } else {
      if (ngpr > 0) {
        ngpr--;
      } else {
        nslots += 2;
      }
    }
  }
  if (nslots > as->evenspill) {
    as->evenspill = nslots;
  }
  // TODO : Return REGSP_HINT(RID_FPRET) for floating numbers?
  return REGSP_HINT(RID_RET);
}

static void asm_setup_target(ASMState *as)
{
  // !!!TODO shared with ARM.
  /* May need extra exit for asm_stack_check on side traces. */
  asm_exitstub_setup(as, as->T->nsnap + (as->parent ? 1 : 0));
}

/* -- Trace patching ------------------------------------------------------ */

/* Patch exit jumps of existing machine code to a new target. */
void lj_asm_patchexit(jit_State *J, GCtrace *T, ExitNo exitno, MCode *target)
{
  MCode *p = T->mcode;
  MCode *pe = (MCode *)((char *)p + T->szmcode);
  MCode *cstart = NULL, *cend = p;
  MCode *mcarea = lj_mcode_patch(J, p, 0);
  MCode *px = exitstub_addr(J, exitno);
  for (; p < pe; p++) {
    /* Look for bl exitstub, replace with b target. */
    uint32_t ins = *p;
    if ((ins & 0xfc000000u) == 0x94000000u &&
	((ins ^ (px-p)) & 0x03ffffffu) == 0) {
      *p = (ins & 0x7c000000u) | ((target-p) & 0x03ffffffu);
      cend = p+1;
      if (!cstart) cstart = p;
    }
  }
  lua_assert(cstart != NULL);
  lj_mcode_sync(cstart, cend);
  lj_mcode_patch(J, mcarea, 1);
}

