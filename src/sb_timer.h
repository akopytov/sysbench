/* Copyright (C) 2004 MySQL AB
   Copyright (C) 2004-2017 Alexey Kopytov <akopytov@gmail.com>

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

#ifndef SB_TIMER_H
#define SB_TIMER_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef _WIN32
#include "sb_win.h"
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <stdint.h>
#include <stdbool.h>

#include "sb_util.h"
#include "ck_spinlock.h"

/* Convert nanoseconds to seconds and vice versa */
#define NS2SEC(nsec) ((nsec)/1000000000.)
#define SEC2NS(sec)  ((uint64_t)(sec) * 1000000000)

/* Convert nanoseconds to milliseconds and vice versa */
#define NS2MS(nsec) ((nsec)/1000000.)
#define MS2NS(sec)  ((sec)*1000000ULL)

/* Convert milliseconds to seconds and vice versa */
#define MS2SEC(msec) ((msec)/1000.)
#define SEC2MS(sec) ((sec)*1000)

/* Difference between two 'timespec' values in nanoseconds */
#define TIMESPEC_DIFF(a,b) (SEC2NS(a.tv_sec - b.tv_sec) + \
			    (a.tv_nsec - b.tv_nsec))

/* Wrapper over various *gettime* functions */
#ifdef HAVE_CLOCK_GETTIME
# define SB_GETTIME(tsp) clock_gettime(CLOCK_MONOTONIC, tsp)
#else
# define SB_GETTIME(tsp)                        \
  do {                                          \
    struct timeval tv;                          \
    gettimeofday(&tv, NULL);                    \
    (tsp)->tv_sec = tv.tv_sec;                  \
    (tsp)->tv_nsec = tv.tv_usec * 1000;         \
  } while (0)
#endif

typedef enum {TIMER_UNINITIALIZED, TIMER_INITIALIZED, TIMER_STOPPED, \
              TIMER_RUNNING} timer_state_t;

/* Timer structure definition */

typedef struct
{
  struct timespec time_start;
  struct timespec time_end;
  uint64_t        events;
  uint64_t        queue_time;
  uint64_t        min_time;
  uint64_t        max_time;
  uint64_t        sum_time;

  ck_spinlock_t   lock;

  char pad[SB_CACHELINE_PAD(sizeof(struct timespec)*2 + sizeof(uint64_t)*5 +
                            sizeof(ck_spinlock_t))];
} sb_timer_t;


/* timer control functions */

/* Initialize timer */
void sb_timer_init(sb_timer_t *);

/* Reset timer counters, but leave the current state intact */
void sb_timer_reset(sb_timer_t *t);

/* check whether the timer is running */
bool sb_timer_running(sb_timer_t *t);

/* start timer */
static inline void sb_timer_start(sb_timer_t *t)
{
  ck_spinlock_lock(&t->lock);

  SB_GETTIME(&t->time_start);

  ck_spinlock_unlock(&t->lock);
}

/* stop timer */
static inline uint64_t sb_timer_stop(sb_timer_t *t)
{
  ck_spinlock_lock(&t->lock);

  SB_GETTIME(&t->time_end);

  uint64_t elapsed = TIMESPEC_DIFF(t->time_end, t->time_start) + t->queue_time;

  t->events++;
  t->sum_time += elapsed;

  if (SB_UNLIKELY(elapsed < t->min_time))
    t->min_time = elapsed;
  if (SB_UNLIKELY(elapsed > t->max_time))
    t->max_time = elapsed;

  ck_spinlock_unlock(&t->lock);

  return elapsed;
}

/*
  get the current timer value in nanoseconds without affecting its state, i.e.
  is safe to be used concurrently on a shared timer.
*/
static inline uint64_t sb_timer_value(sb_timer_t *t)
{
  struct timespec ts;

  SB_GETTIME(&ts);
  return TIMESPEC_DIFF(ts, t->time_start) + t->queue_time;
}

/* Clone a timer */
void sb_timer_copy(sb_timer_t *to, sb_timer_t *from);

/*
  get time elapsed since the previous call to sb_timer_checkpoint() for the
  specified timer without stopping it.  The first call returns time elapsed
  since the timer was started.
*/
uint64_t sb_timer_current(sb_timer_t *t);

/*
  Atomically reset a given timer after copying its state into the timer pointed
  to by 'old'.
*/
void sb_timer_checkpoint(sb_timer_t *t, sb_timer_t *old);

/* get average time per event */
uint64_t sb_timer_avg(sb_timer_t *);

/* get total time for all events */
uint64_t sb_timer_sum(sb_timer_t *);

/* get minimum time */
uint64_t sb_timer_min(sb_timer_t *);

/* get maximum time */
uint64_t sb_timer_max(sb_timer_t *);

/* sum data from two timers. used in summing data from multiple threads */
sb_timer_t sb_timer_merge(sb_timer_t *, sb_timer_t *);

#endif /* SB_TIMER_H */
