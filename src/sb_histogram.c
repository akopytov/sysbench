/* Copyright (C) 2011-2017 Alexey Kopytov.

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
# include <stdio.h>
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_MATH_H
# include <math.h>
#endif

#include "sysbench.h"
#include "sb_histogram.h"
#include "sb_logger.h"
#include "sb_rand.h"

#include "sb_ck_pr.h"
#include "ck_cc.h"

#include "sb_util.h"


/*
   Number of slots for current histogram array. TODO: replace this constant with
   some autodetection code based on the number of available cores or some such.
*/
#define SB_HISTOGRAM_NSLOTS 128

/* Global latency histogram */
sb_histogram_t sb_latency_histogram CK_CC_CACHELINE;


int sb_histogram_init(sb_histogram_t *h, size_t size,
                      double range_min, double range_max)
{
  size_t i;
  uint64_t *tmp;

  /* Allocate memory for cumulative_array + temp_array + all slot arrays */
  tmp = (uint64_t *) calloc(size * (SB_HISTOGRAM_NSLOTS + 2), sizeof(uint64_t));
  h->interm_slots = (uint64_t **) malloc(SB_HISTOGRAM_NSLOTS *
                                         sizeof(uint64_t *));

  if (tmp == NULL || h->interm_slots == NULL)
  {
    log_text(LOG_FATAL,
             "Failed to allocate memory for a histogram object, size = %zd",
             size);
    return 1;
  }

  h->cumulative_array = tmp;
  tmp += size;

  h->temp_array = tmp;
  tmp += size;

  for (i = 0; i < SB_HISTOGRAM_NSLOTS; i++)
  {
    h->interm_slots[i] = tmp;
    tmp += size;
  }

  h->range_deduct = log(range_min);
  h->range_mult = (size - 1) / (log(range_max) - h->range_deduct);

  h->range_min = range_min;
  h->range_max = range_max;

  h->array_size = size;

  pthread_rwlock_init(&h->lock, NULL);

  return 0;
}


void sb_histogram_update(sb_histogram_t *h, double value)
{
  size_t      slot;
  ssize_t     i;

  slot = sb_rand_uniform_uint64() % SB_HISTOGRAM_NSLOTS;

  i = floor((log(value) - h->range_deduct) * h->range_mult + 0.5);
  if (SB_UNLIKELY(i < 0))
    i = 0;
  else if (SB_UNLIKELY(i >= (ssize_t) (h->array_size)))
    i = h->array_size - 1;

  ck_pr_inc_64(&h->interm_slots[slot][i]);
}


double sb_histogram_get_pct_intermediate(sb_histogram_t *h,
                                         double percentile)
{
  size_t   i, s;
  uint64_t nevents, ncur, nmax;
  double   res;

  nevents = 0;

  /*
    This can be called concurrently with other sb_histogram_get_pct_*()
    functions, so use the lock to protect shared structures. This will not block
    sb_histogram_update() calls, but we make sure we don't lose any concurrent
    increments by atomically fetching each array element and replacing it with
    0.
  */
  pthread_rwlock_wrlock(&h->lock);

  /*
    Merge intermediate slots into temp_array.
  */
  const size_t size = h->array_size;
  uint64_t * const array = h->temp_array;

  for (i = 0; i < size; i++)
  {
    array[i] = ck_pr_fas_64(&h->interm_slots[0][i], 0);
    nevents += array[i];
  }

  for (s = 1; s < SB_HISTOGRAM_NSLOTS; s++)
  {
    for (i = 0; i < size; i++)
    {
      uint64_t t;

      t = ck_pr_fas_64(&h->interm_slots[s][i], 0);

      array[i] += t;
      nevents += t;
    }
  }

  /*
    Now that we have an aggregate 'snapshot' of current arrays and the total
    number of events in it, calculate the current, intermediate percentile value
    to return.
  */
  nmax = floor(nevents * percentile / 100 + 0.5);

  ncur = 0;
  for (i = 0; i < size; i++)
  {
    ncur += array[i];
    if (ncur >= nmax)
      break;
  }

  res = exp(i / h->range_mult + h->range_deduct);

  /* Finally, add temp_array into accumulated values in cumulative_array. */
  for (i = 0; i < size; i++)
  {
    h->cumulative_array[i] += array[i];
  }

  h->cumulative_nevents += nevents;

  pthread_rwlock_unlock(&h->lock);

  return res;
}


/*
  Aggregate arrays from intermediate slots into cumulative_array. This should be
  called with the histogram lock write-locked.
*/
static void merge_intermediate_into_cumulative(sb_histogram_t *h)
{
  size_t   i, s;
  uint64_t nevents;

  nevents = h->cumulative_nevents;

  const size_t size = h->array_size;
  uint64_t * const array = h->cumulative_array;

  for (s = 0; s < SB_HISTOGRAM_NSLOTS; s++)
  {
    for (i = 0; i < size; i++)
    {
      uint64_t t = ck_pr_fas_64(&h->interm_slots[s][i], 0);
      array[i] += t;
      nevents += t;
    }
  }

  h->cumulative_nevents = nevents;
}


/*
  Calculate a given percentile from the cumulative array. This should be called
  with the histogram lock either read- or write-locked.
*/
static double get_pct_cumulative(sb_histogram_t *h, double percentile)
{
  size_t   i;
  uint64_t ncur, nmax;

  nmax = floor(h->cumulative_nevents * percentile / 100 + 0.5);

  ncur = 0;
  for (i = 0; i < h->array_size; i++)
  {
    ncur += h->cumulative_array[i];
    if (ncur >= nmax)
      break;
  }

  return exp(i / h->range_mult + h->range_deduct);
}


double sb_histogram_get_pct_cumulative(sb_histogram_t *h, double percentile)
{
  double res;

  /*
    This can be called concurrently with other sb_histogram_get_pct_*()
    functions, so use the lock to protect shared structures. This will not block
    sb_histogram_update() calls, but we make sure we don't lose any concurrent
    increments by atomically fetching each array element and replacing it with
    0.
  */
  pthread_rwlock_wrlock(&h->lock);

  merge_intermediate_into_cumulative(h);

  res = get_pct_cumulative(h, percentile);

  pthread_rwlock_unlock(&h->lock);

  return res;
}


double sb_histogram_get_pct_checkpoint(sb_histogram_t *h,
                                       double percentile)
{
  double   res;

  /*
    This can be called concurrently with other sb_histogram_get_pct_*()
    functions, so use the lock to protect shared structures. This will not block
    sb_histogram_update() calls, but we make sure we don't lose any concurrent
    increments by atomically fetching each array element and replacing it with
    0.
  */
  pthread_rwlock_wrlock(&h->lock);

  merge_intermediate_into_cumulative(h);

  res = get_pct_cumulative(h, percentile);

  /* Reset the cumulative array */
  memset(h->cumulative_array, 0, h->array_size * sizeof(uint64_t));
  h->cumulative_nevents = 0;

  pthread_rwlock_unlock(&h->lock);

  return res;
}


void sb_histogram_print(sb_histogram_t *h)
{
  uint64_t maxcnt;
  int      width;
  size_t   i;

  pthread_rwlock_wrlock(&h->lock);

  merge_intermediate_into_cumulative(h);

  uint64_t * const array = h->cumulative_array;

  maxcnt = 0;
  for (i = 0; i < h->array_size; i++)
  {
    if (array[i] > maxcnt)
      maxcnt = array[i];
  }

  if (maxcnt == 0)
    return;

  printf("       value  ------------- distribution ------------- count\n");

  for (i = 0; i < h->array_size; i++)
  {
    if (array[i] == 0)
      continue;

    width = floor(array[i] * (double) 40 / maxcnt + 0.5);

    printf("%12.3f |%-40.*s %lu\n",
           exp(i / h->range_mult + h->range_deduct),          /* value */
           width, "****************************************", /* distribution */
           (unsigned long) array[i]);                /* count */
  }

  pthread_rwlock_unlock(&h->lock);
}


void sb_histogram_done(sb_histogram_t *h)
{
  pthread_rwlock_destroy(&h->lock);

  free(h->cumulative_array);
  free(h->interm_slots);
}

/*
  Allocate a new histogram and initialize it with sb_histogram_init().
*/

sb_histogram_t *sb_histogram_new(size_t size, double range_min,
                                 double range_max)
{
  sb_histogram_t *h;

  if ((h = malloc(sizeof(*h))) == NULL)
    return NULL;

  if (sb_histogram_init(h, size, range_min, range_max))
  {
    free(h);
    return NULL;
  }

  return h;
}

/*
  Deallocate a histogram allocated with sb_histogram_new().
*/

void sb_histogram_delete(sb_histogram_t *h)
{
  sb_histogram_done(h);
  free(h);
}
