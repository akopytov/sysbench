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

#include "sb_logger.h"
#include "sb_timer.h"

/* Some functions for simple time operations */

/* initialize timer */


void sb_timer_init(sb_timer_t *t)
{
  t->min_time = 0xffffffffffffffffULL;
  t->max_time = 0;
  t->sum_time = 0;
  t->events = 0;
  t->state = TIMER_INITIALIZED;
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
  t->time_last_intermediate = t->time_start;
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

  SB_GETTIME(&t->time_end);
  t->my_time = SEC2NS(t->time_end.tv_sec - t->time_start.tv_sec) +
    (t->time_end.tv_nsec - t->time_start.tv_nsec);
  t->events++;
  t->sum_time += t->my_time;
  if (t->my_time < t->min_time)
    t->min_time = t->my_time;
  if (t->my_time > t->max_time)
    t->max_time = t->my_time;

  t->state = TIMER_STOPPED;
}


/* get timer's value in nanoseconds */


unsigned long long  sb_timer_value(sb_timer_t *t)
{
  return t->my_time;
}


/* get current time without stopping timer */


unsigned long long  sb_timer_current(sb_timer_t *t)
{
  struct timespec time_end;

  switch (t->state) {
    case TIMER_INITIALIZED:
      return 0;
    case TIMER_STOPPED:
      return t->my_time;
    case TIMER_RUNNING:
      break;
    default:
      log_text(LOG_FATAL, "uninitialized timer examined");
      abort();
  }
  
  SB_GETTIME(&time_end);

  return SEC2NS(time_end.tv_sec - t->time_start.tv_sec) +
    (time_end.tv_nsec - t->time_start.tv_nsec);
}

/* get current time without stopping timer AND update structure */
void sb_timer_intermediate(sb_timer_t *t)
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

  SB_GETTIME(&t->time_end);
  t->my_time = SEC2NS(t->time_end.tv_sec - t->time_last_intermediate.tv_sec) +
    (t->time_end.tv_nsec - t->time_last_intermediate.tv_nsec);
  t->time_last_intermediate = t->time_end;
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

  t.my_time = t1->my_time+t2->my_time;
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

/* Subtract *before from *after.  Result given via pointer *diff. */
void
diff_tv(long long *diff, struct timespec *before, struct timespec *after)
{
  time_t sec;

  sec = after->tv_sec - before->tv_sec;
  if (sec != 0)
    *diff = sec * 1000000000LL + after->tv_nsec - before->tv_nsec;
  else
    *diff = after->tv_nsec - before->tv_nsec;
}

/* Add a number of nanoseconds to a struct timespec */
void
add_ns_to_timespec(struct timespec *dest, long long delta)
{
  long long x;
  time_t sec;

  x = dest->tv_nsec + delta;
  if (x > 1000000000) {
    /* Future second */
    dest->tv_sec += x / 1000000000;
    dest->tv_nsec = x % 1000000000;
  } else if (x < 0) {
    /* Past second */
    dest->tv_sec = dest->tv_sec - 1 + (x / 1000000000);
    dest->tv_nsec = (x % 1000000000) + 1000000000;
  } else {
    /* Within the same second */
    dest->tv_nsec = x;
  }
}
