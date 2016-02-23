/* Copyright (C) 2004 MySQL AB
   Copyright (C) 2004-2015 Alexey Kopytov <akopytov@gmail.com>

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

/* Convert nanoseconds to seconds and vice versa */
#define NS2SEC(nsec) ((nsec)/1000000000.)
#define SEC2NS(sec)  ((sec)*1000000000ULL)

/* Convert nanoseconds to milliseconds and vice versa */
#define NS2MS(nsec) ((nsec)/1000000.)
#define MS2NS(sec)  ((sec)*1000000ULL)

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
  struct timespec    time_start;
  struct timespec    time_end;
  struct timespec    time_split;
  unsigned long long elapsed;
  unsigned long long min_time;
  unsigned long long max_time;
  unsigned long long sum_time;
  unsigned long long events;
  unsigned long long queue_time;
  timer_state_t      state;
} sb_timer_t;


/* timer control functions */

/* Initialize timer */
void sb_timer_init(sb_timer_t *);

/* Reset timer counters, but leave the current state intact */
void sb_timer_reset(sb_timer_t *t);

/* check whether the timer is initialized */
int sb_timer_initialized(sb_timer_t *t);

/* check whether the timer is running */
int sb_timer_running(sb_timer_t *t);

/* start timer */
void sb_timer_start(sb_timer_t *);

/* stop timer */
void sb_timer_stop(sb_timer_t *);

/* get the current timer value in nanoseconds */
unsigned long long sb_timer_value(sb_timer_t *);

/*
  get time elapsed since the previos call to sb_timer_split() for the specified
  timer without stopping it.  The first call returns time elapsed since the
  timer was started.
*/
unsigned long long sb_timer_split(sb_timer_t *);

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

/* add a number of nanoseconds to a struct timespec */
void add_ns_to_timespec(struct timespec *dest, long long delta);

#endif /* SB_TIMER_H */
