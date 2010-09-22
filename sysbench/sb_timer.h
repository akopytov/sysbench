/* Copyright (C) 2004 MySQL AB

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

/* Convert nanoseconds to seconds and vice versa */
#define NS2SEC(nsec) ((nsec)/1000000000.)
#define SEC2NS(sec)  ((sec)*1000000000ULL)

#define NS2MS(nsec) ((nsec)/1000000.)
#define MS2NS(msec)  ((msec)*1000000ULL)

/* Wrapper over various *gettime* functions */
#ifdef HAVE_CLOCK_GETTIME
# define SB_GETTIME(tsp) clock_gettime(CLOCK_REALTIME, tsp)
#else
# define SB_GETTIME(tsp)              \
  do {                                \
    struct timeval tv;                \
    gettimeofday(&tv, NULL);          \
    (tsp)->tv_sec = tv.tv_sec;        \
    (tsp)->tv_nsec = tv.tv_usec * 1000;         \
  } while (0)
#endif

typedef enum {TIMER_UNINITIALIZED, TIMER_INITIALIZED, TIMER_STOPPED, \
              TIMER_RUNNING} timer_state_t;

/* Timer structure definition */

typedef struct
{
  struct timespec    time_start;
  struct timespec    time_end;
  struct timespec    time_last_intermediate;
  unsigned long long my_time;
  unsigned long long min_time;
  unsigned long long max_time;
  unsigned long long sum_time;
  unsigned long long events;
  timer_state_t      state;
} sb_timer_t;


/* timer control functions */

/* Initialize timer */
void sb_timer_init(sb_timer_t *);

/* start timer */
void sb_timer_start(sb_timer_t *);

/* stop timer */
void sb_timer_stop(sb_timer_t *);

/* get timer's value in microseconds */
unsigned long long sb_timer_value(sb_timer_t *);

/* get current time without stopping timer */
unsigned long long  sb_timer_current(sb_timer_t *);

/* get average time per event */
unsigned long long get_avg_time(sb_timer_t *);

/* get total time for all events */
unsigned long long get_sum_time(sb_timer_t *);

/* get minimum time */
unsigned long long  get_min_time(sb_timer_t *);

/* get maximum time */
unsigned long long  get_max_time(sb_timer_t *);

/* sum data from two timers. used in summing data from multiple threads */
sb_timer_t merge_timers(sb_timer_t *, sb_timer_t *);

/* subtract *after from *before */
void diff_tv(long long *diff, struct timespec *before, struct timespec *after);

/* add a number of nanoseconds to a struct timespec */
void add_ns_to_timespec(struct timespec *dest, long long delta);

#endif /* SB_TIMER_H */
