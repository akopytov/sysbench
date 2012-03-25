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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "sb_logger.h"
#include "sb_timer.h"

/* Some functions for simple time operations */

static inline void sb_timer_update(sb_timer_t *t)
{
  SB_GETTIME(&t->time_end);
  t->elapsed = TIMESPEC_DIFF(t->time_end, t->time_start) + t->queue_time;
}

/* initialize timer */


void sb_timer_init(sb_timer_t *t)
{
  memset(&t->time_start, 0, sizeof(struct timespec));
  memset(&t->time_end, 0, sizeof(struct timespec));
  memset(&t->time_split, 0, sizeof(struct timespec));
  sb_timer_reset(t);
  t->state = TIMER_INITIALIZED;
}


/* Reset timer counters, but leave the current state intact */
void sb_timer_reset(sb_timer_t *t)
{
  t->min_time = 0xffffffffffffffffULL;
  t->max_time = 0;
  t->sum_time = 0;
  t->events = 0;
  t->elapsed = 0;
  t->queue_time = 0;
}


/* check whether the timer is initialized */
int sb_timer_initialized(sb_timer_t *t)
{
  return t->state != TIMER_UNINITIALIZED;
}

/* check whether the timer is running */
int sb_timer_running(sb_timer_t *t)
{
  return t->state == TIMER_RUNNING;
}

/* start timer */


void sb_timer_start(sb_timer_t *t)
{
  switch (t->state) {
    case TIMER_INITIALIZED:
    case TIMER_STOPPED:
      break;
    case TIMER_RUNNING:
      log_text(LOG_WARNING, "timer was already started");
      break;
    default:
      log_text(LOG_FATAL, "uninitialized timer started");
      abort();
  }
  
  SB_GETTIME(&t->time_start);
  t->time_split = t->time_start;
  t->state = TIMER_RUNNING;
}


/* stop timer */


void sb_timer_stop(sb_timer_t *t)
{
  switch (t->state) {
    case TIMER_INITIALIZED:
      log_text(LOG_WARNING, "timer was never started");
      break;
    case TIMER_STOPPED:
      log_text(LOG_WARNING, "timer was already stopped");
      break;
    case TIMER_RUNNING:
      break;
    default:
      log_text(LOG_FATAL, "uninitialized timer stopped");
      abort();
  }

  sb_timer_update(t);
  t->events++;
  t->sum_time += t->elapsed;
  if (t->elapsed < t->min_time)
    t->min_time = t->elapsed;
  if (t->elapsed > t->max_time)
    t->max_time = t->elapsed;

  t->state = TIMER_STOPPED;
}


/* get the current timer value in nanoseconds */


unsigned long long  sb_timer_value(sb_timer_t *t)
{
  switch (t->state) {
    case TIMER_INITIALIZED:
      log_text(LOG_WARNING, "timer was never started");
      return 0;
    case TIMER_STOPPED:
      return t->elapsed;
    case TIMER_RUNNING:
      break;
    default:
      log_text(LOG_FATAL, "uninitialized timer queried");
      abort();
  }

  sb_timer_update(t);

  return t->elapsed;
}


/*
  get time elapsed since the previos call to sb_timer_split() for the specified
  timer without stopping it.  The first call returns time elapsed since the
  timer was started.
*/


unsigned long long sb_timer_split(sb_timer_t *t)
{
  struct timespec    tmp;
  unsigned long long res;

  switch (t->state) {
    case TIMER_INITIALIZED:
      log_text(LOG_WARNING, "timer was never started");
      return 0;
    case TIMER_STOPPED:
      res = TIMESPEC_DIFF(t->time_end, t->time_split);
      t->time_split = t->time_end;
      if (res)
        return res;
      else
      {
        log_text(LOG_WARNING, "timer was already stopped");
        return 0;
      }
    case TIMER_RUNNING:
      break;
    default:
      log_text(LOG_FATAL, "uninitialized timer queried");
      abort();
  }

  SB_GETTIME(&tmp);
  t->elapsed = TIMESPEC_DIFF(tmp, t->time_start);
  res = TIMESPEC_DIFF(tmp, t->time_split);
  t->time_split = tmp;

  return res;
}

/* get average time per event */


unsigned long long get_avg_time(sb_timer_t *t)
{
  if(t->events == 0)
    return 0; /* return zero if there were no events */
  return (t->sum_time / t->events);
}


/* get total time for all events */


unsigned long long get_sum_time(sb_timer_t *t)
{
  return t->sum_time;
}


/* get minimum time */


unsigned long long  get_min_time(sb_timer_t *t)
{
  return t->min_time;
}


/* get maximum time */


unsigned long long  get_max_time(sb_timer_t *t)
{
  return t->max_time;
}


/* sum data from several timers. used in summing data from multiple threads */


sb_timer_t merge_timers(sb_timer_t *t1, sb_timer_t *t2)
{
  sb_timer_t t;

  /* Initialize to avoid warnings */
  memset(&t, 0, sizeof(sb_timer_t));

  t.elapsed = t1->elapsed + t2->elapsed;
  t.sum_time = t1->sum_time+t2->sum_time;
  t.events = t1->events+t2->events;

  if (t1->max_time > t2->max_time)
    t.max_time = t1->max_time;
  else 
    t.max_time = t2->max_time;

  if (t1->min_time<t2->min_time)
    t.min_time = t1->min_time;
  else 
    t.min_time = t2->min_time;
     
  return t;       
}


/* Add a number of nanoseconds to a struct timespec */


void add_ns_to_timespec(struct timespec *dest, long long delta)
{
  long long x;

  x = dest->tv_nsec + delta;
  if (x > 1000000000)
  {
    /* Future second */
    dest->tv_sec += x / 1000000000;
    dest->tv_nsec = x % 1000000000;
  }
  else if (x < 0)
  {
    /* Past second */
    dest->tv_sec = dest->tv_sec - 1 + (x / 1000000000);
    dest->tv_nsec = (x % 1000000000) + 1000000000;
  }
  else
  {
    /* Within the same second */
    dest->tv_nsec = x;
  }
}
