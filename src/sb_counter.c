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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sb_counter.h"
#include "sysbench.h"
#include "sb_util.h"
#include "sb_ck_pr.h"

static sb_counters_t last_intermediate_counters;
static sb_counters_t last_cumulative_counters;

/* Initialize per-thread stats */

int sb_counters_init(void)
{
  SB_COMPILE_TIME_ASSERT(sizeof(sb_counters_t) % CK_MD_CACHELINE == 0);

  sb_counters = sb_alloc_per_thread_array(sizeof(sb_counters_t));

  return sb_counters == NULL;
}

void sb_counters_done(void)
{
  if (sb_counters != NULL)
  {
    free(sb_counters);
    sb_counters = NULL;
  }
}

static void sb_counters_merge(sb_counters_t dst)
{
  for (size_t t = 0; t < SB_CNT_MAX; t++)
    for (size_t i = 0; i < sb_globals.threads; i++)
      dst[t] += sb_counter_val(i, t);
}

static void sb_counters_checkpoint(sb_counters_t dst, sb_counters_t cp)
{
  for (size_t i = 0; i < SB_CNT_MAX; i++)
  {
    uint64_t tmp = cp[i];
    cp[i] = dst[i];
    dst[i] = dst[i] - tmp;
  }
}

void sb_counters_agg_intermediate(sb_counters_t val)
{
  memset(val, 0, sizeof(sb_counters_t));

  sb_counters_merge(val);
  sb_counters_checkpoint(val, last_intermediate_counters);
}

void sb_counters_agg_cumulative(sb_counters_t val)
{
  memset(val, 0, sizeof(sb_counters_t));

  sb_counters_merge(val);
  sb_counters_checkpoint(val, last_cumulative_counters);
}
