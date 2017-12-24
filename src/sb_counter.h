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

#ifndef SB_COUNTER_H
#define SB_COUNTER_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <inttypes.h>
#endif

#include "sb_util.h"
#include "sb_ck_pr.h"

/* Statistic counter types */

typedef enum {
  SB_CNT_OTHER,        /* unknown type of queries */
  SB_CNT_READ,         /* reads */
  SB_CNT_WRITE,        /* writes */
  SB_CNT_EVENT,        /* events */
  SB_CNT_ERROR,        /* errors */
  SB_CNT_RECONNECT,    /* reconnects */
  SB_CNT_MAX
} sb_counter_type_t;

/*
  sizeof(sb_counter_t) must be a multiple of CK_MD_CACHELINE to avoid cache
  line sharing.
*/
typedef uint64_t
sb_counters_t[SB_ALIGN(SB_CNT_MAX * sizeof(uint64_t), CK_MD_CACHELINE) /
              sizeof(uint64_t)];

extern sb_counters_t *sb_counters;

int sb_counters_init(void);

void sb_counters_done(void);

/*
  Cannot use C99 inline here, because CK atomic primitives use static inlines
  which in turn leads to compiler warnings. So for performance reasons the
  following functions are defined 'static inline' too everywhere except sb_lua.c
  where they are declared and defined as external linkage functions to be
  available from LuaJIT FFI.
*/
#ifndef SB_LUA_EXPORT
# define SB_LUA_INLINE static inline
#else
# define SB_LUA_INLINE
uint64_t sb_counter_val(int thread_id, sb_counter_type_t type);
void sb_counter_inc(int thread_id, sb_counter_type_t type);
#endif

/* Return the current value for a given counter */
SB_LUA_INLINE
uint64_t sb_counter_val(int thread_id, sb_counter_type_t type)
{
  return ck_pr_load_64(&sb_counters[thread_id][type]);
}

/* Increment a given stat counter for a given thread  */
SB_LUA_INLINE
void sb_counter_inc(int thread_id, sb_counter_type_t type)
{
  ck_pr_store_64(&sb_counters[thread_id][type],
                 sb_counter_val(thread_id, type)+1);
}

#undef SB_LUA_INLINE

/*
  Return aggregate counter values since the last intermediate report. This is
  not thread-safe as it updates the global last report state, so it must be
  called from a single thread.
*/
void sb_counters_agg_intermediate(sb_counters_t val);

/*
  Return aggregate counter values since the last cumulative report. This is
  not thread-safe as it updates the global last report state, so it must be
  called from a single thread.
*/
void sb_counters_agg_cumulative(sb_counters_t val);

#endif
