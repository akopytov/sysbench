/* Copyright (C) 2011 Alexey Kopytov.

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
#ifdef _WIN32
#include "sb_win.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_MATH_H
# include <math.h>
#endif

#include "sysbench.h"
#include "sb_percentile.h"
#include "sb_logger.h"

#include "ck_pr.h"

int sb_percentile_init(sb_percentile_t *percentile,
                       unsigned int size, double range_min, double range_max)
{
  if (sb_globals.percentile_rank == 0)
    return 0;

  percentile->values = (uint64_t *)
    calloc(size, sizeof(uint64_t));
  if (percentile->values == NULL)
  {
    log_text(LOG_FATAL, "Cannot allocate values array, size = %u", size);
    return 1;
  }

  percentile->range_deduct = log(range_min);
  percentile->range_mult = (size - 1) / (log(range_max) -
                                         percentile->range_deduct);
  percentile->range_min = range_min;
  percentile->range_max = range_max;
  percentile->size = size;
  percentile->total = 0;

  pthread_rwlock_init(&percentile->lock, NULL);

  return 0;
}

void sb_percentile_update(sb_percentile_t *percentile, double value)
{
  unsigned int n;

  if (sb_globals.percentile_rank == 0)
    return;

  if (value < percentile->range_min)
    value= percentile->range_min;
  else if (value > percentile->range_max)
    value= percentile->range_max;

  n = floor((log(value) - percentile->range_deduct) * percentile->range_mult
            + 0.5);

  pthread_rwlock_rdlock(&percentile->lock);

  ck_pr_add_64(&percentile->total, 1);
  ck_pr_add_64(&percentile->values[n], 1);

  pthread_rwlock_unlock(&percentile->lock);
}

double sb_percentile_calculate(sb_percentile_t *percentile, double percent)
{
  uint64_t     ncur, nmax;
  unsigned int i;

  if (sb_globals.percentile_rank)
    return 0.0;

  pthread_rwlock_rdlock(&percentile->lock);

  nmax = floor(ck_pr_load_64(&percentile->total) * percent / 100 + 0.5);
  if (nmax == 0)
  {
    pthread_rwlock_unlock(&percentile->lock);
    return 0.0;
  }

  ncur = ck_pr_load_64(&percentile->values[0]);
  for (i = 1; i < percentile->size; i++)
  {
    ncur += ck_pr_load_64(&percentile->values[i]);
    if (ncur >= nmax)
      break;
  }

  pthread_rwlock_unlock(&percentile->lock);

  return exp((i) / percentile->range_mult + percentile->range_deduct);
}

void sb_percentile_reset(sb_percentile_t *percentile)
{
  if (sb_globals.percentile_rank == 0)
    return;

  /*
     Block all percentile updates so we don't lose any events on a concurrent
     reset.
  */
  pthread_rwlock_wrlock(&percentile->lock);

  percentile->total = 0;
  memset(percentile->values, 0, percentile->size * sizeof(uint64_t));

  pthread_rwlock_unlock(&percentile->lock);
}

void sb_percentile_done(sb_percentile_t *percentile)
{
  if (sb_globals.percentile_rank == 0)
    return;

  pthread_rwlock_destroy(&percentile->lock);

  free(percentile->values);
}
