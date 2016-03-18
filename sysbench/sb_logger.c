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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef _WIN32
#include "sb_win.h"
#endif

#ifdef STDC_HEADERS
# include <stdio.h>
# include <stdarg.h>
# include <string.h>
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_MATH_H
# include <math.h>
#endif

#include "sysbench.h"
#include "sb_list.h"
#include "sb_logger.h"
#include "sb_percentile.h"

#define TEXT_BUFFER_SIZE 4096
#define ERROR_BUFFER_SIZE 256

#define OPER_LOG_GRANULARITY 100000
#define OPER_LOG_MIN_VALUE   1
#define OPER_LOG_MAX_VALUE   1E13

/* per-thread timers for response time stats */
sb_timer_t *timers;

/* Array of message handlers (one chain per message type) */

static sb_list_t handlers[LOG_MSG_TYPE_MAX];

/* set after logger initialization */
static unsigned char initialized; 

static sb_percentile_t percentile;

static pthread_mutex_t text_mutex;
static unsigned int    text_cnt;
static char            text_buf[TEXT_BUFFER_SIZE];

/* Temporary copy of timers */
static sb_timer_t *timers_copy;

/*
  Mutex protecting timers.
  TODO: replace with an rwlock (and implement pthread rwlocks for Windows).
*/
static pthread_mutex_t timers_mutex;

static int text_handler_init(void);
static int text_handler_process(log_msg_t *msg);

static int oper_handler_init(void);
static int oper_handler_process(log_msg_t *msg);
static int oper_handler_done(void);

/* Built-in log handlers */

/* Text messages handler */

static sb_arg_t text_handler_args[] =
{
  {"verbosity", "verbosity level {5 - debug, 0 - only critical messages}",
   SB_ARG_TYPE_INT, "3"},
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};
  

static log_handler_t text_handler = {
  {
    &text_handler_init,
    &text_handler_process,
    NULL,
  },
  text_handler_args,
  {0,0}
};

/* Operation start/stop messages handler */

static sb_arg_t oper_handler_args[] =
{
  {"percentile", "percentile rank of query response times to count",
   SB_ARG_TYPE_INT, "95"},

  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

static log_handler_t oper_handler = {
  {
    oper_handler_init,
    &oper_handler_process,
    oper_handler_done,
  },
  oper_handler_args,
  {0,0}
};


/* Register logger and all handlers */


int log_register(void)
{
  unsigned int i;

  for (i = 0; i < LOG_MSG_TYPE_MAX; i++)
    SB_LIST_INIT(handlers + i);

  log_add_handler(LOG_MSG_TYPE_TEXT, &text_handler);
  log_add_handler(LOG_MSG_TYPE_OPER, &oper_handler);
  
  return 0;
}


/* Display command line options for registered log handlers */


void log_print_help(void)
{
  unsigned int    i;
  sb_list_item_t  *pos;
  log_handler_t   *handler;

  printf("Log options:\n");

  for (i = 0; i < LOG_MSG_TYPE_MAX; i++)
  {
    SB_LIST_FOR_EACH(pos, handlers + i)
    {
      handler = SB_LIST_ENTRY(pos, log_handler_t, listitem);
      if (handler->args != NULL)
	sb_print_options(handler->args);
    }
  }
}


/* Initialize logger and all handlers */


int log_init(void)
{
  unsigned int    i;
  sb_list_item_t  *pos;
  log_handler_t   *handler;

  for (i = 0; i < LOG_MSG_TYPE_MAX; i++)
  {
    SB_LIST_FOR_EACH(pos, handlers + i)
    {
      handler = SB_LIST_ENTRY(pos, log_handler_t, listitem);
      if (handler->ops.init != NULL && handler->ops.init())
        return 1;
    }
  }

  /* required to let log_text() pass messages to handlers */
  initialized = 1; 
  
  return 0;
}


/* Uninitialize logger and all handlers */


void log_done(void)
{
  unsigned int    i;
  sb_list_item_t  *pos;
  log_handler_t   *handler;

  for (i = 0; i < LOG_MSG_TYPE_MAX; i++)
  {
    SB_LIST_FOR_EACH(pos, handlers + i)
    {
      handler = SB_LIST_ENTRY(pos, log_handler_t, listitem);
      if (handler->ops.done != NULL)
        handler->ops.done();
    }
  }

  initialized = 0;
}  


/* Add handler for a specified type of messages */


int log_add_handler(log_msg_type_t type, log_handler_t *handler)
{
  if (type <= LOG_MSG_TYPE_MIN || type >= LOG_MSG_TYPE_MAX)
    return 1;

  if (handler->args != NULL)
    sb_register_arg_set(handler->args);
  
  SB_LIST_ADD_TAIL(&handler->listitem, handlers + type);

  return 0;
}


/* Main function to dispatch log messages */


void log_msg(log_msg_t *msg)
{
  sb_list_item_t  *pos;
  log_handler_t   *handler;
  
  SB_LIST_FOR_EACH(pos, handlers + msg->type)
  {
    handler = SB_LIST_ENTRY(pos, log_handler_t, listitem);
    if (handler->ops.process != NULL)
      handler->ops.process(msg);
  }
}


/* printf-like wrapper to log text messages */


void log_text(log_msg_priority_t priority, const char *fmt, ...)
{
  log_msg_t      msg;
  log_msg_text_t text_msg;
  char           buf[TEXT_BUFFER_SIZE];
  va_list        ap;
  int            n, clen, maxlen;

  maxlen = TEXT_BUFFER_SIZE;
  clen = 0;

  va_start(ap, fmt);
  n = vsnprintf(buf + clen, maxlen, fmt, ap);
  va_end(ap);
  if (n < 0 || n >= maxlen)
    n = maxlen;
  clen += n;
  maxlen -= n;
  snprintf(buf + clen, maxlen, "\n");

  /*
    No race condition here because log_init() is supposed to be called
    in a single-threaded stage
  */
  if (!initialized)
  {
    printf("%s", buf);
    
    return;
  }
  
  msg.type = LOG_MSG_TYPE_TEXT;
  msg.data = (void *)&text_msg;
  text_msg.priority = priority;
  text_msg.text = buf;
  text_msg.flags = 0;

  log_msg(&msg);
}


/*
  variant of log_text() which prepends log lines with the elapsed time of a
  specified timer.
*/


void log_timestamp(log_msg_priority_t priority, const sb_timer_t *timer,
                   const char *fmt, ...)
{
  log_msg_t      msg;
  log_msg_text_t text_msg;
  char           buf[TEXT_BUFFER_SIZE];
  va_list        ap;
  int            n, clen, maxlen;

  maxlen = TEXT_BUFFER_SIZE;
  clen = 0;

  n = snprintf(buf, maxlen, "[%4.0fs] ", NS2SEC(timer->elapsed));
  clen += n;
  maxlen -= n;

  va_start(ap, fmt);
  n = vsnprintf(buf + clen, maxlen, fmt, ap);
  va_end(ap);
  if (n < 0 || n >= maxlen)
    n = maxlen;
  clen += n;
  maxlen -= n;
  snprintf(buf + clen, maxlen, "\n");

  /*
    No race condition here because log_init() is supposed to be called
    in a single-threaded stage
  */
  if (!initialized)
  {
    printf("%s", buf);

    return;
  }

  msg.type = LOG_MSG_TYPE_TEXT;
  msg.data = (void *)&text_msg;
  text_msg.priority = priority;
  text_msg.text = buf;
  /* Skip duplicate checks */
  text_msg.flags = LOG_MSG_TEXT_ALLOW_DUPLICATES;

  log_msg(&msg);
}


/* printf-like wrapper to log system error messages */


void log_errno(log_msg_priority_t priority, const char *fmt, ...)
{
  char           buf[TEXT_BUFFER_SIZE];
  char           errbuf[ERROR_BUFFER_SIZE];
  va_list        ap;
  int            n;
  int            old_errno;
  char           *tmp;

#ifdef _WIN32
  LPVOID         lpMsgBuf;
  old_errno = GetLastError();
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, old_errno,
                0, (LPTSTR)&lpMsgBuf, 0, NULL);
  tmp = (char *)lpMsgBuf;
#else
  old_errno = errno;
#ifdef HAVE_STRERROR_R
#ifdef STRERROR_R_CHAR_P
  tmp = strerror_r(old_errno, errbuf, sizeof(errbuf));
#else
  strerror_r(old_errno, errbuf, sizeof(errbuf));
  tmp = errbuf;
#endif /* STRERROR_R_CHAR_P */
#else /* !HAVE_STRERROR_P */
  strncpy(errbuf, strerror(old_errno), sizeof(errbuf));
  tmp = errbuf;
#endif /* HAVE_STRERROR_P */
#endif /* WIN32 */
  
  va_start(ap, fmt);
  n = vsnprintf(buf, TEXT_BUFFER_SIZE, fmt, ap);
  va_end(ap);
  if (n < 0 || n == TEXT_BUFFER_SIZE)
    return;
  snprintf(buf + n, TEXT_BUFFER_SIZE - n, " errno = %d (%s)", old_errno,
           tmp);

#ifdef _WIN32
  LocalFree(lpMsgBuf);
#endif
  
  log_text(priority, "%s", buf);
}



/* Initialize text handler */


int text_handler_init(void)
{
#ifdef HAVE_SETVBUF
  /* Set stdout to unbuffered mode */
  setvbuf(stdout, NULL, _IONBF, 0);
#endif
  
  sb_globals.verbosity = sb_get_value_int("verbosity");

  if (sb_globals.verbosity > LOG_DEBUG)
  {
    printf("Invalid value for verbosity: %d\n", sb_globals.verbosity);
    return 1;
  }

  pthread_mutex_init(&text_mutex, NULL);
  text_cnt = 0;
  text_buf[0] = '\0';
  
  return 0;
}


/* Print text message to the log */


int text_handler_process(log_msg_t *msg)
{
  char *prefix;
  log_msg_text_t *text_msg = (log_msg_text_t *)msg->data;

  if (text_msg->priority > sb_globals.verbosity)
    return 0;

  if (!text_msg->flags & LOG_MSG_TEXT_ALLOW_DUPLICATES)
  {
    pthread_mutex_lock(&text_mutex);
    if (!strcmp(text_buf, text_msg->text))
    {
      text_cnt++;
      pthread_mutex_unlock(&text_mutex);

      return 0;
    }
    else
    {
      if (text_cnt > 0)
        printf("(last message repeated %u times)\n", text_cnt);

      text_cnt = 0;
      strncpy(text_buf, text_msg->text, TEXT_BUFFER_SIZE);
    }
    pthread_mutex_unlock(&text_mutex);
  }

  switch (text_msg->priority) {
    case LOG_FATAL:
      prefix = "FATAL: ";
      break;
    case LOG_ALERT:
      prefix = "ALERT: ";
      break;
    case LOG_WARNING:
      prefix = "WARNING: ";
      break;
    case LOG_DEBUG:
      prefix = "DEBUG: ";
      break;
    default:
      prefix = "";
      break;
  }
  
  printf("%s%s", prefix, text_msg->text);
  
  return 0;
}


/* Initialize operation messages handler */


int oper_handler_init(void)
{
  unsigned int i, tmp;

  tmp = sb_get_value_int("percentile");
  if (tmp < 1 || tmp > 100)
  {
    log_text(LOG_FATAL, "Invalid value for percentile option: %d",
             tmp);
    return 1;
  }
  sb_globals.percentile_rank = tmp;

  if (sb_percentile_init(&percentile, OPER_LOG_GRANULARITY, OPER_LOG_MIN_VALUE,
                         OPER_LOG_MAX_VALUE))
    return 1;

  timers = (sb_timer_t *)malloc(sb_globals.num_threads * sizeof(sb_timer_t));
  timers_copy = (sb_timer_t *)malloc(sb_globals.num_threads *
                                     sizeof(sb_timer_t));
  if (timers == NULL || timers_copy == NULL)
  {
    log_text(LOG_FATAL, "Memory allocation failure");
    return 1;
  }

  for (i = 0; i < sb_globals.num_threads; i++)
    sb_timer_init(&timers[i]);

  if (sb_globals.n_checkpoints > 0)
    pthread_mutex_init(&timers_mutex, NULL);

  return 0;
}


/* Process operation start/stop messages */


int oper_handler_process(log_msg_t *msg)
{
  log_msg_oper_t *oper_msg = (log_msg_oper_t *)msg->data;
  sb_timer_t     *timer = &timers[oper_msg->thread_id];
  long long      value;

  if (oper_msg->action == LOG_MSG_OPER_START)
  {
    if (sb_globals.n_checkpoints > 0)
      pthread_mutex_lock(&timers_mutex);
    sb_timer_start(timer);
    if (sb_globals.n_checkpoints > 0)
      pthread_mutex_unlock(&timers_mutex);

    return 0;
  }

  if (sb_globals.n_checkpoints > 0)
    pthread_mutex_lock(&timers_mutex);

  sb_timer_stop(timer);

  value = sb_timer_value(timer);

  if (sb_globals.n_checkpoints > 0)
    pthread_mutex_unlock(&timers_mutex);

  sb_percentile_update(&percentile, value);

  return 0;
}

/*
  Print global stats either from the last checkpoint (if used) or
  from the test start.
*/

int print_global_stats(void)
{
  double       diff;
  unsigned int i;
  unsigned int nthreads;
  sb_timer_t   t;
  /* variables to count thread fairness */
  double       events_avg;
  double       events_stddev;
  double       time_avg;
  double       time_stddev;
  double       percentile_val;
  unsigned long long total_time_ns;

  sb_timer_init(&t);
  nthreads = sb_globals.num_threads;

  /* Create a temporary copy of timers and reset them */
  if (sb_globals.n_checkpoints > 0)
    pthread_mutex_lock(&timers_mutex);

  memcpy(timers_copy, timers, sb_globals.num_threads * sizeof(sb_timer_t));
  for (i = 0; i < sb_globals.num_threads; i++)
    sb_timer_reset(&timers[i]);

  total_time_ns = sb_timer_split(&sb_globals.cumulative_timer2);

  percentile_val = sb_percentile_calculate(&percentile,
                                           sb_globals.percentile_rank);
  sb_percentile_reset(&percentile);

  if (sb_globals.n_checkpoints > 0)
    pthread_mutex_unlock(&timers_mutex);

  if (sb_globals.forced_shutdown_in_progress)
  {
    /*
      In case we print statistics on forced shutdown, there may be (potentially
      long running or hung) transactions which are still in progress.

      We still want to reflect them in statistics, so stop running timers to
      consider long transactions as done at the forced shutdown time, and print
      a counter of still running transactions.
    */
    unsigned int unfinished = 0;

    for (i = 0; i < nthreads; i++)
    {
      if (sb_timer_running(&timers_copy[i]))
      {
        unfinished++;
        sb_timer_stop(&timers_copy[i]);
      };
    }

    if (unfinished > 0)
    {
      log_text(LOG_NOTICE, "");
      log_text(LOG_NOTICE, "Number of unfinished transactions on "
               "forced shutdown: %u", unfinished);
    }
  }

  for(i = 0; i < nthreads; i++)
    t = merge_timers(&t, &timers_copy[i]);

/* Print total statistics */
  log_text(LOG_NOTICE, "");
  log_text(LOG_NOTICE, "General statistics:");
  log_text(LOG_NOTICE, "    total time:                          %.4fs",
           NS2SEC(total_time_ns));
  log_text(LOG_NOTICE, "    total number of events:              %lld",
           t.events);
  log_text(LOG_NOTICE, "    total time taken by event execution: %.4fs",
           NS2SEC(get_sum_time(&t)));

  log_text(LOG_NOTICE, "    response time:");
  log_text(LOG_NOTICE, "         min:                            %10.2fms",
           NS2MS(get_min_time(&t)));
  log_text(LOG_NOTICE, "         avg:                            %10.2fms",
           NS2MS(get_avg_time(&t)));
  log_text(LOG_NOTICE, "         max:                            %10.2fms",
           NS2MS(get_max_time(&t)));

  /* Print approx. percentile value for event execution times */
  if (t.events > 0)
  {
    log_text(LOG_NOTICE, "         approx. %3d percentile:         %10.2fms",
             sb_globals.percentile_rank, NS2MS(percentile_val));
  }
  log_text(LOG_NOTICE, "");

  /*
    Check how fair thread distribution over task is.
    We check amount of events/thread as well as avg event execution time.
    Fairness is calculated in %, looking at maximum and average difference
    from the average time/request and req/thread
   */
  events_avg = (double)t.events / nthreads;
  time_avg = NS2SEC(get_sum_time(&t)) / nthreads;
  events_stddev = 0;
  time_stddev = 0;
  for(i = 0; i < nthreads; i++)
  {
    diff = fabs(events_avg - timers_copy[i].events);
    events_stddev += diff*diff;
    
    diff = fabs(time_avg - NS2SEC(get_sum_time(&timers_copy[i])));
    time_stddev += diff*diff;
  }
  events_stddev = sqrt(events_stddev / nthreads);
  time_stddev = sqrt(time_stddev / nthreads);
  
  log_text(LOG_NOTICE, "Threads fairness:");
  log_text(LOG_NOTICE, "    events (avg/stddev):           %.4f/%3.2f",
           events_avg, events_stddev);
  log_text(LOG_NOTICE, "    execution time (avg/stddev):   %.4f/%3.2f",
           time_avg, time_stddev);
  log_text(LOG_NOTICE, "");

  if (sb_globals.debug)
  {
    log_text(LOG_DEBUG, "Verbose per-thread statistics:\n");
    for(i = 0; i < nthreads; i++)
    {
      log_text(LOG_DEBUG, "    thread #%3d: min: %.4fs  avg: %.4fs  max: %.4fs  "
               "events: %lld",i,
               NS2SEC(get_min_time(&timers_copy[i])),
               NS2SEC(get_avg_time(&timers_copy[i])),
               NS2SEC(get_max_time(&timers_copy[i])),
               timers_copy[i].events);
      log_text(LOG_DEBUG, "                 "
               "total time taken by even execution: %.4fs",
               NS2SEC(get_sum_time(&timers_copy[i]))
               );
    }
    log_text(LOG_NOTICE, "");
  }

  return 0;
}

/* Uninitialize operations messages handler */

int oper_handler_done(void)
{
  print_global_stats();

  free(timers);
  free(timers_copy);

  if (sb_globals.n_checkpoints > 0)
    pthread_mutex_destroy(&timers_mutex);

  return 0;
}
