/*
   Copyright (C) 2017 Alexey Kopytov <akopytov@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright 2010 Samy Al Bahra.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
  Compatibility wrappers for ConcurrencyKit functions. ConcurrencyKit does not
  implement 64-bit atomic primitives on some 32-bit architectures (e.g. x86,
  ARMv6) natively. Newer versions support the CK_USE_CC_BUILTINS define which
  allows resorting to compiler builtins emulating 64-bit atomics on 32-bit
  architectures. However, one might want to build with an old,
  distribution-provided CK version where that option is not available. For those
  versions, define 64-bit atomics as aliases to GCC builtins. A cleaner solution
  would be to define the corresponding sysbench data structures as 32-bit
  integers on those architectures, but the amount of extra code to implement and
  support is not worth it. Code below has been copied with modifications from
  gcc/ck_pr.h from the ConcurrencyKit distribution.
*/

#ifndef SB_CK_H
#define SB_CK_H

#include "ck_pr.h"

#if !defined(CK_F_PR_LOAD_64)

#ifndef __GNUC__
#  error Unsupported platform
#endif

#include <ck_cc.h>

#define CK_PR_ACCESS(x) (*(volatile __typeof__(x) *)&(x))

#define CK_PR_LOAD(S, M, T)		 			\
        CK_CC_INLINE static T					\
        ck_pr_md_load_##S(const M *target)			\
        {							\
                T r;						\
                ck_pr_barrier();				\
                r = CK_PR_ACCESS(*(const T *)target);		\
                ck_pr_barrier();				\
                return (r);					\
        }							\
        CK_CC_INLINE static void				\
        ck_pr_md_store_##S(M *target, T v)			\
        {							\
                ck_pr_barrier();				\
                CK_PR_ACCESS(*(T *)target) = v;			\
                ck_pr_barrier();				\
                return;						\
        }

#define CK_PR_LOAD_S(S, T) CK_PR_LOAD(S, T, T)

CK_PR_LOAD_S(64, uint64_t)

#undef CK_PR_LOAD_S
#undef CK_PR_LOAD

#define CK_PR_LOAD_SAFE(SRC, TYPE) ck_pr_md_load_##TYPE((SRC))

#define CK_PR_STORE_SAFE(DST, VAL, TYPE)			\
    ck_pr_md_store_##TYPE(					\
        ((void)sizeof(*(DST) = (VAL)), (DST)),			\
        (VAL))

#define ck_pr_load_64(SRC) CK_PR_LOAD_SAFE((SRC), 64)
#define ck_pr_store_64(DST, VAL) CK_PR_STORE_SAFE((DST), (VAL), 64)

/*
 * Atomic compare and swap.
 */
#define CK_PR_CAS(S, M, T)							\
        CK_CC_INLINE static bool						\
        ck_pr_cas_##S(M *target, T compare, T set)				\
        {									\
                bool z;								\
                z = __sync_bool_compare_and_swap((T *)target, compare, set);	\
                return z;							\
        }

#define CK_PR_CAS_S(S, T) CK_PR_CAS(S, T, T)

CK_PR_CAS_S(64, uint64_t)

#undef CK_PR_CAS_S
#undef CK_PR_CAS

#define CK_PR_CAS_O(S, T)						\
        CK_CC_INLINE static bool					\
        ck_pr_cas_##S##_value(T *target, T compare, T set, T *v)	\
        {								\
                set = __sync_val_compare_and_swap(target, compare, set);\
                *v = set;						\
                return (set == compare);				\
        }

CK_PR_CAS_O(64, uint64_t)

#undef CK_PR_CAS_O

/*
 * Atomic store-only binary operations.
 */
#define CK_PR_BINARY(K, S, M, T)				\
        CK_CC_INLINE static void				\
        ck_pr_##K##_##S(M *target, T d)				\
        {							\
                d = __sync_fetch_and_##K((T *)target, d);	\
                return;						\
        }

#define CK_PR_BINARY_S(K, S, T) CK_PR_BINARY(K, S, T, T)

#define CK_PR_GENERATE(K)			\
        CK_PR_BINARY_S(K, 64, uint64_t)

CK_PR_GENERATE(add)
CK_PR_GENERATE(sub)
CK_PR_GENERATE(and)
CK_PR_GENERATE(or)
CK_PR_GENERATE(xor)

#undef CK_PR_GENERATE
#undef CK_PR_BINARY_S
#undef CK_PR_BINARY

#define CK_PR_UNARY(S, M, T)			\
        CK_CC_INLINE static void		\
        ck_pr_inc_##S(M *target)		\
        {					\
                ck_pr_add_##S(target, (T)1);	\
                return;				\
        }					\
        CK_CC_INLINE static void		\
        ck_pr_dec_##S(M *target)		\
        {					\
                ck_pr_sub_##S(target, (T)1);	\
                return;				\
        }

#define CK_PR_UNARY_S(S, M) CK_PR_UNARY(S, M, M)

CK_PR_UNARY_S(64, uint64_t)

#undef CK_PR_UNARY_S
#undef CK_PR_UNARY

#define CK_PR_FAA(S, M, T, C)						\
        CK_CC_INLINE static C						\
        ck_pr_faa_##S(M *target, T delta)				\
        {								\
                T previous;						\
                C punt;							\
                punt = (C)ck_pr_md_load_##S(target);			\
                previous = (T)punt;					\
                while (ck_pr_cas_##S##_value(target,			\
                                             (C)previous,		\
                                             (C)(previous + delta),	\
                                             &previous) == false)	\
                        ck_pr_stall();					\
                                                                        \
                return ((C)previous);					\
        }

#define CK_PR_FAS(S, M, C)						\
        CK_CC_INLINE static C						\
        ck_pr_fas_##S(M *target, C update)				\
        {								\
                C previous;						\
                previous = ck_pr_md_load_##S(target);			\
                while (ck_pr_cas_##S##_value(target,			\
                                             previous,			\
                                             update,			\
                                             &previous) == false)	\
                        ck_pr_stall();					\
                                                                        \
                return (previous);					\
        }

#define CK_PR_FAA_S(S, M) CK_PR_FAA(S, M, M, M)
#define CK_PR_FAS_S(S, M) CK_PR_FAS(S, M, M)

CK_PR_FAA_S(64, uint64_t)
CK_PR_FAS_S(64, uint64_t)

#undef CK_PR_FAA_S
#undef CK_PR_FAA
#undef CK_PR_FAS_S
#undef CK_PR_FAS

#endif /* !CK_F_PR_LOAD_64*/
#endif /* SB_CK_H */
