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

#ifndef SYSBENCH_H
#define SYSBENCH_H

#ifdef STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <stdbool.h>
#endif
#ifdef HAVE_UNISTD_H 
# include <unistd.h>
# include <sys/types.h>
#endif
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#include "sb_list.h"
#include "sb_options.h"
#include "sb_timer.h"
#include "sb_logger.h"

#include "tests/sb_cpu.h"
#include "tests/sb_fileio.h"
#include "tests/sb_memory.h"
#include "tests/sb_threads.h"
#include "tests/sb_mutex.h"

/* Macros to control global execution mutex */
#define SB_THREAD_MUTEX_LOCK() pthread_mutex_lock(&sb_globals.exec_mutex) 
#define SB_THREAD_MUTEX_UNLOCK() pthread_mutex_unlock(&sb_globals.exec_mutex)

/* Maximum number of elements in --report-checkpoints list */
#define MAX_CHECKPOINTS 256

/* Request types definition */

typedef enum
{
  SB_REQ_TYPE_NULL,
  SB_REQ_TYPE_CPU,
  SB_REQ_TYPE_MEMORY,
  SB_REQ_TYPE_FILE,
  SB_REQ_TYPE_SQL,
  SB_REQ_TYPE_THREADS,
  SB_REQ_TYPE_MUTEX,
  SB_REQ_TYPE_SCRIPT
} sb_event_type_t;

/* Request structure definition */

struct sb_test; /* Forward declaration */

typedef struct
{
  int              type;
  struct sb_test_t *test;

  /* type-specific data */
  union
  {
    sb_file_request_t    file_request;
    sb_threads_request_t threads_request;
    sb_mutex_request_t   mutex_request;
  } u;
} sb_event_t;

/* Statistics */

typedef struct {
  uint32_t threads_running;     /* Number of active threads */

  double   time_interval;       /* Time elapsed since the last report */
  double   time_total;          /* Time elapsed since the benchmark start */

  double   latency_pct;         /* Latency percentile */

  double   latency_min;         /* Minimum latency (cumulative reports only) */
  double   latency_max;         /* Maximum latency (cumulative reports only) */
  double   latency_avg;         /* Average latency (cumulative reports only) */
  double   latency_sum;         /* Sum latency (cumulative reports only) */

  uint64_t events;              /* Number of executed events */
  uint64_t reads;               /* Number of read operations */
  uint64_t writes;              /* Number of write operations */
  uint64_t other;               /* Number of other operations */
  uint64_t errors;              /* Number of ignored errors */
  uint64_t reconnects;          /* Number of reconnects to server */

  uint64_t queue_length;        /* Event queue length (tx_rate-only) */
  uint64_t concurrency;         /* Number of in-flight events (tx_rate-only) */
} sb_stat_t;

/* Commands */

typedef int sb_builtin_cmd_func_t(void);
typedef int sb_custom_cmd_func_t(int);

/* Test operations definition */

typedef int sb_op_init(void);
typedef int sb_op_prepare(void);
typedef int sb_op_thread_init(int);
typedef int sb_op_thread_run(int);
typedef void sb_op_print_mode(void);
typedef sb_event_t sb_op_next_event(int);
typedef int sb_op_execute_event(sb_event_t *, int);
typedef void sb_op_report(sb_stat_t *);
typedef int sb_op_thread_done(int);
typedef int sb_op_cleanup(void);
typedef int sb_op_done(void);

/* Test commands structure definitions */

typedef struct
{
  sb_builtin_cmd_func_t *help;  /* print help */
  sb_builtin_cmd_func_t *prepare; /* prepare for the test */
  sb_builtin_cmd_func_t *run;   /* run the test */
  sb_builtin_cmd_func_t *cleanup; /* cleanup the test database, files, etc. */
} sb_builtin_cmds_t;

/* Test operations structure definition */

typedef struct
{
  sb_op_init            *init;            /* initialization function */
  sb_op_prepare         *prepare;         /* called after timers start,  but
                                             before thread execution */
  sb_op_thread_init     *thread_init;     /* thread initialization
                                             (called when each thread starts) */
  sb_op_print_mode      *print_mode;      /* print mode function */
  sb_op_next_event      *next_event;      /* event generation function */
  sb_op_execute_event   *execute_event;   /* event execution function */
  sb_op_report          *report_intermediate; /* intermediate reports handler */
  sb_op_report          *report_cumulative;   /* cumulative reports handler */
  sb_op_thread_run      *thread_run;      /* main thread loop */
  sb_op_thread_done     *thread_done;     /* thread finalize function */
  sb_op_cleanup         *cleanup;         /* called after exit from thread,
                                             but before timers stop */ 
  sb_op_done            *done;            /* finalize function */
} sb_operations_t;

/* Test structure definition */

typedef struct sb_test
{
  const char        *sname;
  const char        *lname;
  sb_operations_t   ops;
  sb_builtin_cmds_t builtin_cmds;
  sb_arg_t          *args;

  sb_list_item_t    listitem;
} sb_test_t;

/* sysbench global variables */

typedef struct
{
  int             error CK_CC_CACHELINE;        /* global error flag */
  int             argc;         /* command line arguments count */
  char            **argv;      /* command line arguments */
  unsigned int    tx_rate;      /* target transaction rate */
  uint64_t        max_events;   /* maximum number of events to execute */
  uint64_t        max_time_ns;  /* total execution time limit */
  pthread_mutex_t exec_mutex CK_CC_CACHELINE;   /* execution mutex */
  const char      *testname;    /* test name or script path to execute */
  const char      *cmdname;     /* command passed from command line */
  unsigned int    threads CK_CC_CACHELINE;  /* number of threads to use */
  unsigned int    threads_running;  /* number of threads currently active */
  unsigned int    report_interval;  /* intermediate reports interval */
  unsigned int    percentile;   /* percentile rank for latency stats */
  unsigned int    histogram;    /* show histogram in latency stats */
  /* array of report checkpoints */
  unsigned int    checkpoints[MAX_CHECKPOINTS];
  unsigned int    n_checkpoints; /* number of checkpoints */
  unsigned char   debug;        /* debug flag */
  unsigned int    timeout;      /* forced shutdown timeout */
  unsigned char   validate;     /* validation flag */
  unsigned char   verbosity CK_CC_CACHELINE;    /* log verbosity */
  int             concurrency CK_CC_CACHELINE;  /* number of concurrent requests
                                                when tx-rate is used */
  int             force_shutdown CK_CC_CACHELINE; /* whether we must force test
                                                  shutdown */
  int             forced_shutdown_in_progress;
  uint64_t        nevents CK_CC_CACHELINE; /* event counter */
} sb_globals_t;

extern sb_globals_t sb_globals CK_CC_CACHELINE;
extern pthread_mutex_t event_queue_mutex CK_CC_CACHELINE;

/* Global execution timer */
extern sb_timer_t      sb_exec_timer CK_CC_CACHELINE;

/* timers for checkpoint reports */
extern sb_timer_t      sb_intermediate_timer;
extern sb_timer_t      sb_checkpoint_timer;

extern TLS int sb_tls_thread_id;

bool sb_more_events(int thread_id);
sb_event_t sb_next_event(sb_test_t *test, int thread_id);
void sb_event_start(int thread_id);
void sb_event_stop(int thread_id);

/* Print a description of available command line options for the current test */
void sb_print_test_options(void);

/* Default intermediate reports handler */
void sb_report_intermediate(sb_stat_t *stat);

/* Default cumulative reports handler */
void sb_report_cumulative(sb_stat_t *stat);

/*
  Allocate an array of objects of the specified size for all threads, both
  worker and background ones.
*/
void *sb_alloc_per_thread_array(size_t size);

#endif
