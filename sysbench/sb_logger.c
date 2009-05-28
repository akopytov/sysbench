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

/* Format of the timestamp string */
#define TIMESTAMP_FMT "[%s] "

#define TEXT_BUFFER_SIZE 4096
#define ERROR_BUFFER_SIZE 256

#define OPER_LOG_GRANULARITY 100000
#define OPER_LOG_MIN_VALUE   1
#define OPER_LOG_MAX_VALUE   1E13

typedef enum {
  BATCH_STATUS_START,
  BATCH_STATUS_STOP
} batch_status_t;

/* Array of message handlers (one chain per message type) */

static sb_list_t handlers[LOG_MSG_TYPE_MAX];

/* set after logger initialization */
static unsigned char initialized; 

/* verbosity of messages */
static unsigned char verbosity; 

/* whether each message must be timestamped */
static unsigned char log_timestamp; 

/* whether batch mode must be used */
static unsigned char batch_mode;

/* delay in seconds between statistics dumps in batch mode */
static unsigned int batch_delay;

/*
  gettimeofday() is over-optimized on some architectures what results in excessive warning message
  This flag is required to issue a warning only once
*/
static unsigned char oper_time_warning;

/* array of operation response times for operations handler */
static unsigned int    operations[OPER_LOG_GRANULARITY]; 
static double          oper_log_deduct;
static double          oper_log_mult;
static unsigned int    oper_percentile;
static pthread_mutex_t oper_mutex; /* used to sync access to operations array */
static pthread_mutex_t batch_mutex; /* used to sync batch operations */
static pthread_cond_t  batch_cond;
static pthread_t       batch_thread;
static batch_status_t  batch_status;

static pthread_mutex_t text_mutex;
static unsigned int    text_cnt;
static char            text_buf[TEXT_BUFFER_SIZE];

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
   SB_ARG_TYPE_INT, "4"},
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
  {"batch", "dump current stats periodically instead of final ones",
   SB_ARG_TYPE_FLAG, "off"},
  {"batch-delay", "delay between batch dumps in seconds",
   SB_ARG_TYPE_INT, "300"},

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


/* Start routine for the batch thread */
void *batch_runner_proc(void *);


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
  struct tm      tm_now;
  time_t         t_now;

  maxlen = TEXT_BUFFER_SIZE;
  clen = 0;
  
  if (log_timestamp)
  {
    time(&t_now);
    gmtime_r((const time_t *)&t_now, &tm_now);
    n = strftime(buf, maxlen, TIMESTAMP_FMT, &tm_now);
    clen += n;
    maxlen -= n;
  }
  
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
  
  verbosity = sb_get_value_int("verbosity");

  if (verbosity > LOG_DEBUG)
  {
    printf("Invalid value for verbosity: %d\n", verbosity);
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

  if (text_msg->priority > verbosity)
    return 0;
  
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
  fflush(stdout);
  
  return 0;
}


/* Initialize operation messages handler */


int oper_handler_init(void)
{
  pthread_attr_t batch_attr;
  
  oper_percentile = sb_get_value_int("percentile");
  if (oper_percentile < 1 || oper_percentile > 100)
  {
    log_text(LOG_FATAL, "Invalid value for percentile option: %d",
             oper_percentile);
    return 1;
  }

  oper_log_deduct = log(OPER_LOG_MIN_VALUE);
  oper_log_mult = (OPER_LOG_GRANULARITY - 1) / (log(OPER_LOG_MAX_VALUE) - oper_log_deduct);

  batch_mode = sb_get_value_flag("batch");
  if (batch_mode)
    log_timestamp = 1;
  batch_delay = sb_get_value_int("batch-delay");

  pthread_mutex_init(&oper_mutex, NULL);

  if (batch_mode)
  {
    int err;
    pthread_mutex_init(&batch_mutex, NULL);
    pthread_cond_init(&batch_cond, NULL);

    /* Create batch thread */
    pthread_attr_init(&batch_attr);
    if ((err = pthread_create(&batch_thread, &batch_attr, &batch_runner_proc, NULL))
        != 0)
    {
      log_text(LOG_FATAL, "Batch thread creation failed, errno = %d (%s)",
                err, strerror(err));
      return 1;
    }
    batch_status = BATCH_STATUS_STOP;
  }
  
  return 0;
}


/* Process operation start/stop messages */


int oper_handler_process(log_msg_t *msg)
{
  double         optime;
  unsigned int   ncell;
  log_msg_oper_t *oper_msg = (log_msg_oper_t *)msg->data;

  if (batch_mode)
  {
    pthread_mutex_lock(&batch_mutex);
    if (batch_status != BATCH_STATUS_START)
    {
      /* Wake up the batch thread */
      batch_status = BATCH_STATUS_START;
      pthread_cond_signal(&batch_cond);
    }
    pthread_mutex_unlock(&batch_mutex);
  }
  
  if (oper_msg->action == LOG_MSG_OPER_START)
  {
    sb_timer_init(&oper_msg->timer);
    sb_timer_start(&oper_msg->timer);
    return 0;
  }

  optime = sb_timer_current(&oper_msg->timer);
  if (optime < OPER_LOG_MIN_VALUE)
  {
    /* Warn only once */
    if (!oper_time_warning) {
      log_text(LOG_WARNING, "Operation time (%f) is less than minimal counted value, "
               "counting as %f", optime, (double)OPER_LOG_MIN_VALUE);
      log_text(LOG_WARNING, "Percentile statistics will be inaccurate");
      oper_time_warning = 1;
    }
    optime = OPER_LOG_MIN_VALUE;
  }
  else if (optime > OPER_LOG_MAX_VALUE)
  {
    /* Warn only once */
    if (!oper_time_warning) {
      log_text(LOG_WARNING, "Operation time (%f) is greater than maximal counted value, "
               "counting as %f", optime, (double)OPER_LOG_MAX_VALUE);
      log_text(LOG_WARNING, "Percentile statistics will be inaccurate");
      oper_time_warning = 1;
    }
    optime = OPER_LOG_MAX_VALUE;
  }
  
  ncell = floor((log(optime) - oper_log_deduct) * oper_log_mult + 0.5);
  pthread_mutex_lock(&oper_mutex);
  operations[ncell]++;
  pthread_mutex_unlock(&oper_mutex);
  
  return 0;
}


/* Uninitialize operations messages handler */


int oper_handler_done(void)
{
  double       p;
  double       diff;
  double       pdiff;
  double       optime;
  unsigned int i;
  unsigned int noper = 0;
  unsigned int nthreads;
  sb_timer_t   t;
  /* variables to count thread fairness */
  double       events_avg;
  double       events_stddev;
  double       time_avg;
  double       time_stddev;

  if (batch_mode)
  {
    int err;
    /* Stop the batch thread */
    pthread_mutex_lock(&batch_mutex);
    batch_status = BATCH_STATUS_STOP;
    pthread_cond_signal(&batch_cond);
    pthread_mutex_unlock(&batch_mutex);

    if ((err = pthread_join(batch_thread, NULL)))
    {
      log_text(LOG_FATAL, "Batch thread join failed, errno = %d (%s)",
               err, strerror(err));
      return 1;
    }

    pthread_mutex_destroy(&batch_mutex);
    pthread_cond_destroy(&batch_cond);
  }
  
  sb_timer_init(&t);
  nthreads = sb_globals.num_threads;
  for(i = 0; i < nthreads; i++)
    t = merge_timers(&t,&(sb_globals.op_timers[i]));

  /* Print total statistics */
  log_text(LOG_NOTICE, "");
  log_text(LOG_NOTICE, "Test execution summary:");
  log_text(LOG_NOTICE, "    total time:                          %.4fs",
           NS2SEC(sb_timer_value(&sb_globals.exec_timer)));
  log_text(LOG_NOTICE, "    total number of events:              %lld",
           t.events);
  log_text(LOG_NOTICE, "    total time taken by event execution: %.4f",
           NS2SEC(get_sum_time(&t)));

  log_text(LOG_NOTICE, "    per-request statistics:");
  log_text(LOG_NOTICE, "         min:                            %10.2fms",
           NS2MS(get_min_time(&t)));
  log_text(LOG_NOTICE, "         avg:                            %10.2fms",
           NS2MS(get_avg_time(&t)));
  log_text(LOG_NOTICE, "         max:                            %10.2fms",
           NS2MS(get_max_time(&t)));

  /* Print approx. percentile value for event execution times */
  if (t.events > 0)
  {
    /* Calculate element with a given percentile rank */
    pdiff = oper_percentile;
    for (i = 0; i < OPER_LOG_GRANULARITY; i++)
    {
      noper += operations[i];
      p = (double)noper / t.events * 100;
      diff = fabs(p - oper_percentile);
      if (diff > pdiff || fabs(diff) < 1e-6)
        break;
      pdiff = diff;
    }
    if (i > 0)
      i--;
  
    /* Calculate response time corresponding to this element */
    optime = exp((double)i / oper_log_mult + oper_log_deduct);
    log_text(LOG_NOTICE, "         approx. %3d percentile:         %10.2fms",
             oper_percentile, NS2MS(optime));
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
    diff = fabs(events_avg - sb_globals.op_timers[i].events);
    events_stddev += diff*diff;
    
    diff = fabs(time_avg - NS2SEC(get_sum_time(&sb_globals.op_timers[i])));
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
               NS2SEC(get_min_time(&sb_globals.op_timers[i])),
               NS2SEC(get_avg_time(&sb_globals.op_timers[i])),
               NS2SEC(get_max_time(&sb_globals.op_timers[i])),
               sb_globals.op_timers[i].events);
      log_text(LOG_DEBUG, "                 "
               "total time taken by even execution: %.4fs",
               NS2SEC(get_sum_time(&sb_globals.op_timers[i]))
               );
    }
    log_text(LOG_NOTICE, "");
  }

  pthread_mutex_destroy(&oper_mutex);
  
  return 0;
}


/* Worker thread to periodically dump stats in batch mode */


void *batch_runner_proc(void *arg)
{
  sb_timer_t         t;
  struct timespec    delay;
#ifndef HAVE_CLOCK_GETTIME
  struct timeval     tv;
#endif 
  int                rc;
  unsigned int       i, noper;
  double             diff, pdiff, p, percent, optime;
  
  (void)arg; /* unused */
  
  /* Wait for the test to start */
  pthread_mutex_lock(&batch_mutex);
  pthread_cond_wait(&batch_cond, &batch_mutex);
  pthread_mutex_unlock(&batch_mutex);
  
  /* Main batch loop */
  while(batch_status != BATCH_STATUS_STOP)
  {
    /* Wait for batch_delay seconds */
    pthread_mutex_lock(&batch_mutex);

#ifdef HAVE_CLOCK_GETTIME
    clock_gettime(CLOCK_REALTIME, &delay);
    delay.tv_sec += batch_delay;
#else
    gettimeofday(&tv, NULL);
    delay.tv_sec = tv.tv_sec + batch_delay;
    delay.tv_nsec = tv.tv_usec * 1000;
#endif
    rc = pthread_cond_timedwait(&batch_cond, &batch_mutex, &delay);
    pthread_mutex_unlock(&batch_mutex);

    /* Status changed? */
    if (rc != ETIMEDOUT)
      continue;

    /* Dump current statistics */

    /* Calculate min/avg/max values for the last period */
    sb_timer_init(&t);
    for(i = 0; i < sb_globals.num_threads; i++)
      t = merge_timers(&t,&(sb_globals.op_timers[i]));

    /* Do we have any events to measure? */
    if (t.events == 0)
      continue;

    /* Calculate percentile value for the last period */
    percent = 0;
    pthread_mutex_lock(&oper_mutex);

    /* Calculate element with a given percentile rank */
    pdiff = oper_percentile;
    noper = 0;
    for (i = 0; i < OPER_LOG_GRANULARITY; i++)
    {
      noper += operations[i];
      p = (double)noper / t.events * 100;
      diff = fabs(p - oper_percentile);
      if (diff > pdiff || fabs(diff) < 1e-6)
        break;
      pdiff = diff;
    }
    if (i > 0)
      i--;
  
    /* Calculate response time corresponding to this element */
    optime = exp((double)i / oper_log_mult + oper_log_deduct);
    pthread_mutex_unlock(&oper_mutex);

    log_text(LOG_NOTICE, "min: %.4f  avg: %.4f  max: %.4f  percentile: %.4f",
             NS2SEC(get_min_time(&t)), NS2SEC(get_avg_time(&t)),
             NS2SEC(get_max_time(&t)), NS2SEC(optime));
  }

  return NULL;
}
