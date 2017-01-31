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

#ifndef SB_HISTOGRAM_H
#define SB_HISTOGRAM_H

#include <stdint.h>

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

typedef struct {
  /*
     Cumulative histogram array. Updated 'on demand' by
     sb_histogram_get_pct_intermediate(). Protected by 'lock'.
  */
  uint64_t              *cumulative_array;
  /*
     Total number of events in cumulative_array. Updated on demand by
     sb_histogram_get_pct_intermediate(). Protected by 'lock'.
  */
  uint64_t              cumulative_nevents;
  /*
    Temporary array for intermediate percentile calculations. Protected by
    'lock'.
  */
  uint64_t              *temp_array;
  /*
     Intermediate histogram values are split into multiple slots and updated
     with atomics. Aggregations into cumulative values is performed by
     sb_histogram_get_pct_intermediate() function.
  */
  uint64_t              **interm_slots;
  /* Number of elements in each array */
  size_t                array_size;
  /* Lower bound of values to track */
  double                range_min;
  /* Upper bound of values to track */
  double                range_max;
  /* Value to deduct to calculate histogram range based on array element */
  double                range_deduct;
  /* Value to multiply to calculate histogram range based array element */
  double                range_mult;
  /*
     rwlock to protect cumulative_array and cumulative_nevents from concurrent
     updates.
  */
  pthread_rwlock_t      lock;
} sb_histogram_t;

/* Global latency histogram */
extern sb_histogram_t sb_latency_histogram;

/*
  Allocate a new histogram and initialize it with sb_histogram_init().
*/
sb_histogram_t *sb_histogram_new(size_t size, double range_min,
                                 double range_max);

/*
  Deallocate a histogram allocated with sb_histogram_new().
*/
void sb_histogram_delete(sb_histogram_t *h);

/*
  Initialize a new histogram object with a given array size and value bounds.
*/
int sb_histogram_init(sb_histogram_t *h, size_t size,
                      double range_min, double range_max);

/* Update histogram with a given value. */
void sb_histogram_update(sb_histogram_t *h, double value);

/*
  Calculate a given percentile value from the intermediate histogram values,
  then merge intermediate values into cumulative ones atomically, i.e. in a way
  that no concurrent updates are lost and will be accounted in either the
  current or later merge of intermediate clues.
*/
double sb_histogram_get_pct_intermediate(sb_histogram_t *h, double percentile);

/*
  Merge intermediate histogram values into cumulative ones and calculate a given
  percentile value from the cumulative array.
*/
double sb_histogram_get_pct_cumulative(sb_histogram_t *h, double percentile);

/*
   Similar to sb_histogram_get_pct_cumulative(), but also resets cumulative
   stats right after calculating the returned percentile. The reset happens
   atomically so that no conucrrent updates are lost after percentile
   calculation. This is currently used only by 'checkpoint' reports.
*/
double sb_histogram_get_pct_checkpoint(sb_histogram_t *h, double percentile);

/*
  Print a given histogram to stdout
*/
void sb_histogram_print(sb_histogram_t *h);

/* Destroy a given histogram object */
void sb_histogram_done(sb_histogram_t *h);

#endif
