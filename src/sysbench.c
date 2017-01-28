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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#ifdef HAVE_UNISTD_H 
# include <unistd.h>
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif
#ifdef HAVE_THREAD_H
# include <thread.h>
#endif
#ifdef HAVE_MATH_H
# include <math.h>
#endif
#ifdef HAVE_SCHED_H
# include <sched.h>
#endif
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#include <luajit.h>

#include "sysbench.h"
#include "sb_options.h"
#include "sb_lua.h"
#include "db_driver.h"
#include "sb_rand.h"
#include "sb_thread.h"
#include "sb_barrier.h"

#include "ck_cc.h"

#define VERSION_STRING PACKAGE" "PACKAGE_VERSION SB_GIT_SHA

/* Maximum queue length for the tx-rate mode */
#define MAX_QUEUE_LEN 100000

/* Wait at most this number of seconds for worker threads to initialize */
#define THREAD_INIT_TIMEOUT 30

/* Event queue data type for the tx-rate mode */
typedef struct {
  unsigned long long event_time;
  sb_list_item_t     listitem;
} event_queue_elem_t;

/* Mutex to protect report_interval */
static pthread_mutex_t    report_interval_mutex;

/* General options */
sb_arg_t general_args[] =
{
  SB_OPT("num-threads", "number of threads to use", "1", INT),
  SB_OPT("max-requests", "limit for total number of requests", "10000", INT),
  SB_OPT("max-time", "limit for total execution time in seconds", "0", INT),
  SB_OPT("forced-shutdown",
         "number of seconds to wait after --max-time before forcing shutdown, "
         "or 'off' to disable", "off", STRING),
  SB_OPT("thread-stack-size", "size of stack per thread", "64K", SIZE),
  SB_OPT("tx-rate", "target transaction rate (tps)", "0", INT),
  SB_OPT("report-interval", "periodically report intermediate statistics with "
         "a specified interval in seconds. 0 disables intermediate reports",
         "0", INT),
  SB_OPT("report-checkpoints", "dump full statistics and reset all counters at "
         "specified points in time. The argument is a list of comma-separated "
         "values representing the amount of time in seconds elapsed from start "
         "of test when report checkpoint(s) must be performed. Report "
         "checkpoints are off by default.", "", LIST),
  SB_OPT("debug", "print more debugging info", "off", BOOL),
  SB_OPT("validate", "perform validation checks where possible", "off", BOOL),
  SB_OPT("help", "print help and exit", "off", BOOL),
  SB_OPT("version", "print version and exit", "off", BOOL),
  SB_OPT("config-file", "File containing command line options", NULL, FILE),

  SB_OPT_END
};

/* List of available tests */
sb_list_t        tests;

/* Global variables */
sb_globals_t     sb_globals;
sb_test_t        *current_test;

/* Barrier to ensure we start the benchmark run when all workers are ready */
static sb_barrier_t thread_start_barrier;

/* structures to handle queue of events, needed for tx_rate mode */
pthread_mutex_t           event_queue_mutex;
static sb_list_t          event_queue;
static pthread_cond_t     event_queue_cv;
static event_queue_elem_t queue_array[MAX_QUEUE_LEN];

static int queue_is_full;

static int report_thread_created;
static int checkpoints_thread_created;
static int eventgen_thread_created;

/* Global execution timer */
sb_timer_t      sb_exec_timer CK_CC_CACHELINE;

/* timers for intermediate/checkpoint reports */
sb_timer_t sb_intermediate_timer CK_CC_CACHELINE;
sb_timer_t sb_checkpoint_timer1  CK_CC_CACHELINE;
sb_timer_t sb_checkpoint_timer2  CK_CC_CACHELINE;

TLS int sb_tls_thread_id;

static void print_header(void);
static void print_help(void);
static void print_run_mode(sb_test_t *);

#ifdef HAVE_ALARM
static void sigalrm_thread_init_timeout_handler(int sig)
{
  if (sig != SIGALRM)
    return;

  log_text(LOG_FATAL,
           "Worker threads failed to initialize within %u seconds!",
           THREAD_INIT_TIMEOUT);

  exit(2);
}

static void sigalrm_forced_shutdown_handler(int sig)
{
  if (sig != SIGALRM)
    return;

  sb_globals.forced_shutdown_in_progress = 1;

  sb_timer_stop(&sb_exec_timer);
  sb_timer_stop(&sb_intermediate_timer);
  sb_timer_stop(&sb_checkpoint_timer1);
  sb_timer_stop(&sb_checkpoint_timer2);

  log_text(LOG_FATAL,
           "The --max-time limit has expired, forcing shutdown...");

  if (current_test && current_test->ops.print_stats)
    current_test->ops.print_stats(SB_STAT_CUMULATIVE);

  log_done();

  exit(2);
}
#endif


static int register_tests(void)
{
  SB_LIST_INIT(&tests);

  /* Register tests */
  return register_test_fileio(&tests)
    + register_test_cpu(&tests)
    + register_test_memory(&tests)
    + register_test_threads(&tests)
    + register_test_mutex(&tests)
    + db_register()
    + sb_rand_register()
    ;
}


/* Print program header */


void print_header(void)
{
  log_text(LOG_NOTICE,
           "%s (using %s %s)\n",
           VERSION_STRING, SB_WITH_LUAJIT, LUAJIT_VERSION);
}


/* Print program usage */


void print_help(void)
{
  sb_list_item_t *pos;
  sb_test_t      *test;
  
  printf("Usage:\n");
  printf("  sysbench [options]... [testname] [command]\n\n");
  printf("Commands implemented by most tests: prepare run cleanup help\n\n");
  printf("General options:\n");
  sb_print_options(general_args);

  sb_rand_print_help();

  log_print_help();

  db_print_help();

  printf("Compiled-in tests:\n");
  SB_LIST_FOR_EACH(pos, &tests)
  {
    test = SB_LIST_ENTRY(pos, sb_test_t, listitem);
    printf("  %s - %s\n", test->sname, test->lname);
  }
  printf("\n");
  printf("See 'sysbench <testname> help' for a list of options for "
         "each test.\n\n");
}

/*
  Set an option value if a default value has been previously set with
  sb_register_arg_set(), i.e. if it's a 'known' option, or ignore_unknown is
  'true'. In which case return 0, otherwise return 1.
*/

static int parse_option(char *name, bool ignore_unknown)
{
  const char        *value;
  char              *tmp;
  option_t          *opt;
  char              ctmp = 0;
  int               rc;

  tmp = strchr(name, '=');
  if (tmp != NULL)
  {
    ctmp = *tmp;
    *tmp = '\0';
    value = tmp + 1;
  }
  else
  {
    value = NULL;
  }

  opt = sb_find_option(name);
  if (opt != NULL || ignore_unknown)
    rc = set_option(name, value,
                    opt != NULL ? opt->type : SB_ARG_TYPE_STRING) == NULL;
  else
    rc =  1;

  if (tmp != NULL)
    *tmp = ctmp;

  return rc;
}

/*
  Parse general command line arguments. Test-specific argument are parsed by
  parse_test_arguments() at a later stage when a builtin test or a Lua script is
  known.
*/

static int parse_general_arguments(int argc, char *argv[])
{
  const char *      testname;
  const char *      cmdname;

  /* Set default values for general options */
  if (sb_register_arg_set(general_args))
    return 1;

  /* Parse command line arguments */
  testname = NULL;
  cmdname = NULL;

  for (int i = 1; i < argc; i++)
  {
    if (strncmp(argv[i], "--", 2))
    {
      if (testname == NULL)
      {
        testname = argv[i];
        continue;
      }

      if (cmdname == NULL)
      {
        cmdname = argv[i];
        continue;
      }

      fprintf(stderr, "Unrecognized command line argument: %s\n", argv[i]);

      return 1;
    }

    if (!parse_option(argv[i]+2, false))
    {
      /* An option from general_args. Exclude it from future processing */
      argv[i] = NULL;
    }
  }

  sb_globals.testname = testname;
  sb_globals.cmdname = cmdname;

  return 0;
}

/* Parse test-specific arguments */

static int parse_test_arguments(sb_test_t *test, int argc, char *argv[])
{
  /* Set default values */
  if (test->args != NULL && sb_register_arg_set(test->args))
    return 1;

  for (int i = 1; i < argc; i++)
  {
    /* Skip already parsed and non-option arguments */
    if (argv[i] == NULL || strncmp(argv[i], "--", 2))
      continue;

    /*
      At this stage an unrecognized option must throw a error, unless the test
      defines no options (for compatibility with legacy Lua scripts). In the
      latter case we just export all unrecognized options as strings.
    */
    if (parse_option(argv[i]+2, test->args == NULL))
    {
      fprintf(stderr, "invalid option: %s\n", argv[i]);
      return 1;
    }

    argv[i] = NULL;
  }

  return 0;
}


void print_run_mode(sb_test_t *test)
{
  log_text(LOG_NOTICE, "Running the test with following options:");
  log_text(LOG_NOTICE, "Number of threads: %d", sb_globals.num_threads);

  if (sb_globals.tx_rate > 0)
  {
    log_text(LOG_NOTICE,
            "Target transaction rate: %d/sec", sb_globals.tx_rate);
  }

  if (sb_globals.report_interval)
  {
    log_text(LOG_NOTICE, "Report intermediate results every %d second(s)",
             sb_globals.report_interval);
  }

  if (sb_globals.n_checkpoints > 0)
  {
    char         list_str[MAX_CHECKPOINTS * 12];
    char         *tmp = list_str;
    unsigned int i;
    int          n, size = sizeof(list_str);

    for (i = 0; i < sb_globals.n_checkpoints - 1; i++)
    {
      n = snprintf(tmp, size, "%u, ", sb_globals.checkpoints[i]);
      if (n >= size)
        break;
      tmp += n;
      size -= n;
    }
    if (i == sb_globals.n_checkpoints - 1)
      snprintf(tmp, size, "%u", sb_globals.checkpoints[i]);
    log_text(LOG_NOTICE, "Report checkpoint(s) at %s seconds",
             list_str);
  }

  if (sb_globals.debug)
    log_text(LOG_NOTICE, "Debug mode enabled.\n");
  
  if (sb_globals.validate)
    log_text(LOG_NOTICE, "Additional request validation enabled.\n");

  if (sb_rand_seed)
  {
    log_text(LOG_NOTICE,
             "Initializing random number generator from seed (%d).\n",
             sb_rand_seed);
    srandom(sb_rand_seed);
  }
  else
  {
    log_text(LOG_NOTICE,
             "Initializing random number generator from current time\n");
    srandom(time(NULL));
  }

  if (sb_globals.force_shutdown)
    log_text(LOG_NOTICE, "Forcing shutdown in %u seconds",
             (unsigned) NS2SEC(sb_globals.max_time_ns) + sb_globals.timeout);
  
  log_text(LOG_NOTICE, "");

  if (test->ops.print_mode != NULL)
    test->ops.print_mode();
}

bool sb_more_events(int thread_id)
{
  (void) thread_id; /* unused */

  if (sb_globals.error)
    return false;

  /* Check if we have a time limit */
  if (sb_globals.max_time_ns > 0 &&
      sb_timer_value(&sb_exec_timer) >= sb_globals.max_time_ns)
  {
    log_text(LOG_INFO, "Time limit exceeded, exiting...");
    return false;
  }

  /* Check if we have a limit on the number of events */
  if (sb_globals.max_requests > 0 &&
      ck_pr_faa_64(&sb_globals.nevents, 1) >= sb_globals.max_requests)
    return false;

  return true;
}

/*
   Get the next event, or return an 'empty' event with type = SB_REQ_TYPE_NULL,
   if there are no more events to execute.
*/

sb_event_t sb_next_event(sb_test_t *test, int thread_id)
{
  sb_list_item_t      *pos;
  event_queue_elem_t  *elem;
  unsigned long long  queue_start_time = 0;

  /* If we are in tx_rate mode, we take events from queue */
  if (sb_globals.tx_rate > 0)
  {
    if (queue_is_full)
    {
      log_text(LOG_FATAL, "Event queue is full.");

      sb_event_t          event;
      event.type = SB_REQ_TYPE_NULL;

      return event;
    }
    pthread_mutex_lock(&event_queue_mutex);
    while(!sb_globals.event_queue_length)
      pthread_cond_wait(&event_queue_cv, &event_queue_mutex);

    SB_LIST_ONCE(pos, &event_queue)
    {
      elem = SB_LIST_ENTRY(pos, event_queue_elem_t, listitem);
      queue_start_time = elem->event_time;;

      SB_LIST_DELETE(pos);

      sb_globals.event_queue_length--;
    }

    sb_globals.concurrency++;

    pthread_mutex_unlock(&event_queue_mutex);

    timers[thread_id].queue_time = sb_timer_value(&sb_exec_timer) -
      queue_start_time;

  }

  return test->ops.next_event(thread_id);
}


void sb_event_start(int thread_id)
{
  sb_timer_t     *timer = &timers[thread_id];

  if (sb_globals.n_checkpoints > 0)
    pthread_mutex_lock(&timers_mutex);

  sb_timer_start(timer);

  if (sb_globals.n_checkpoints > 0)
    pthread_mutex_unlock(&timers_mutex);
}


void sb_event_stop(int thread_id)
{
  sb_timer_t     *timer = &timers[thread_id];
  long long      value;

  if (sb_globals.n_checkpoints > 0)
    pthread_mutex_lock(&timers_mutex);

  value = sb_timer_stop(timer);

  if (sb_globals.n_checkpoints > 0)
    pthread_mutex_unlock(&timers_mutex);

  if (sb_globals.percentile > 0)
    sb_histogram_update(&sb_latency_histogram, NS2MS(value));

  db_thread_stat_inc(thread_id, DB_STAT_TRX);

  if (sb_globals.tx_rate > 0)
  {
    pthread_mutex_lock(&event_queue_mutex);
    sb_globals.concurrency--;
    pthread_mutex_unlock(&event_queue_mutex);
  }
}


/* Main event loop -- the default thread_run implementation */


static int thread_run(sb_test_t *test, int thread_id)
{
  sb_event_t        event;
  int               rc = 0;

  while (sb_more_events(thread_id) && rc == 0)
  {
    event = sb_next_event(test, thread_id);
    if (event.type == SB_REQ_TYPE_NULL)
      break;

    sb_event_start(thread_id);

    rc = test->ops.execute_event(&event, thread_id);

    sb_event_stop(thread_id);
  }

  return rc;
}


/* Main worker thread */


static void *worker_thread(void *arg)
{
  sb_thread_ctxt_t   *ctxt;
  unsigned int       thread_id;
  int                rc;

  ctxt = (sb_thread_ctxt_t *)arg;
  sb_test_t * const test = current_test;

  sb_tls_thread_id = thread_id = ctxt->id;

  /* Initialize thread-local RNG state */
  sb_rand_thread_init();

  log_text(LOG_DEBUG, "Worker thread (#%d) started", thread_id);

  if (test->ops.thread_init != NULL && test->ops.thread_init(thread_id) != 0)
  {
    log_text(LOG_DEBUG, "Worker thread (#%d) failed to initialize!", thread_id);
    sb_globals.error = 1;
    /* Avoid blocking the main thread */
    sb_barrier_wait(&thread_start_barrier);
    return NULL;
  }

  log_text(LOG_DEBUG, "Worker thread (#%d) initialized", thread_id);

  /* Wait for other threads to initialize */
  if (sb_barrier_wait(&thread_start_barrier) < 0)
    return NULL;

  if (test->ops.thread_run != NULL)
  {
    /* Use benchmark-provided thread_run implementation */
    rc = test->ops.thread_run(thread_id);
  }
  else
  {
    /* Use default thread_run implementation */
    rc = thread_run(test, thread_id);
  }

  if (rc != 0)
    sb_globals.error = 1;
  else if (test->ops.thread_done != NULL)
    test->ops.thread_done(thread_id);

  return NULL;
}

static void *eventgen_thread_proc(void *arg)
{
  unsigned long long pause_ns;
  unsigned long long next_ns;
  unsigned long long curr_ns;
  unsigned long long intr_ns;
  int                i;

  (void)arg; /* unused */

  SB_LIST_INIT(&event_queue);
  i = 0;

  log_text(LOG_DEBUG, "Event generating thread started");

  /* Wait for other threads to initialize */
  if (sb_barrier_wait(&thread_start_barrier) < 0)
    return NULL;

  eventgen_thread_created = 1;

  curr_ns = sb_timer_value(&sb_exec_timer);
  /* emulate exponential distribution with Lambda = tx_rate */
  intr_ns = (long) (log(1 - sb_rand_uniform_double()) /
                    (-(double) sb_globals.tx_rate)*1000000);
  next_ns = curr_ns + intr_ns*1000;

  for (;;)
  {
    curr_ns = sb_timer_value(&sb_exec_timer);

    /* emulate exponential distribution with Lambda = tx_rate */
    intr_ns = (long) (log(1 - sb_rand_uniform_double()) /
                      (-(double)sb_globals.tx_rate)*1000000);

    next_ns = next_ns + intr_ns*1000;
    if (next_ns > curr_ns)
    {
      pause_ns = next_ns - curr_ns;
      usleep(pause_ns / 1000);
    }
    else
    {
      log_timestamp(LOG_DEBUG, NS2SEC(curr_ns),
                    "Event generation thread is too slow");
    }


    queue_array[i].event_time = sb_timer_value(&sb_exec_timer);
    pthread_mutex_lock(&event_queue_mutex);
    SB_LIST_ADD_TAIL(&queue_array[i].listitem, &event_queue);
    sb_globals.event_queue_length++;
    if (sb_globals.event_queue_length >= MAX_QUEUE_LEN)
      queue_is_full = 1;
    pthread_cond_signal(&event_queue_cv);
    pthread_mutex_unlock(&event_queue_mutex);

    if (queue_is_full)
    {
      log_text(LOG_FATAL, "Event queue is full.");
      return NULL;
    }

    i++;
    if (i >= MAX_QUEUE_LEN)
      i = 0;
  }

  return NULL;
}

/* Intermediate reports thread */

static void *report_thread_proc(void *arg)
{
  unsigned long long       pause_ns;
  unsigned long long       prev_ns;
  unsigned long long       next_ns;
  unsigned long long       curr_ns;
  const unsigned long long interval_ns = SEC2NS(sb_globals.report_interval);

  (void)arg; /* unused */

  log_text(LOG_DEBUG, "Reporting thread started");

  /* Wait for other threads to initialize */
  if (sb_barrier_wait(&thread_start_barrier) < 0)
    return NULL;

  if (current_test->ops.print_stats == NULL)
  {
    log_text(LOG_DEBUG, "Reporting is not supported by the current test, "
             "terminating the reporting thread");
    return NULL;
  }

  report_thread_created = 1;

  pause_ns = interval_ns;
  prev_ns = sb_timer_value(&sb_exec_timer) + interval_ns;
  for (;;)
  {
    usleep(pause_ns / 1000);
    /*
      sb_globals.report_interval may be set to 0 by the master thread
      to silence report at the end of the test
    */
    pthread_mutex_lock(&report_interval_mutex);
    if (sb_globals.report_interval > 0)
      current_test->ops.print_stats(SB_STAT_INTERMEDIATE);
    pthread_mutex_unlock(&report_interval_mutex);

    curr_ns = sb_timer_value(&sb_exec_timer);
    do
    {
      next_ns = prev_ns + interval_ns;
      prev_ns = next_ns;
    } while (curr_ns >= next_ns);
    pause_ns = next_ns - curr_ns;
  }

  return NULL;
}

/* Checkpoints reports thread */

static void *checkpoints_thread_proc(void *arg)
{
  unsigned long long       pause_ns;
  unsigned long long       next_ns;
  unsigned long long       curr_ns;
  unsigned int             i;

  (void)arg; /* unused */

  log_text(LOG_DEBUG, "Checkpoints report thread started");

  /* Wait for other threads to initialize */
  if (sb_barrier_wait(&thread_start_barrier) < 0)
    return NULL;

  if (current_test->ops.print_stats == NULL)
  {
    log_text(LOG_DEBUG, "Reporting is not supported by the current test, "
             "terminating the checkpoints thread");
    return NULL;
  }

  checkpoints_thread_created = 1;

  for (i = 0; i < sb_globals.n_checkpoints; i++)
  {
    next_ns = SEC2NS(sb_globals.checkpoints[i]);
    curr_ns = sb_timer_value(&sb_exec_timer);
    if (next_ns <= curr_ns)
      continue;

    pause_ns = next_ns - curr_ns;
    usleep(pause_ns / 1000);

    SB_THREAD_MUTEX_LOCK();
    log_timestamp(LOG_NOTICE, NS2SEC(sb_timer_value(&sb_exec_timer)),
                  "Checkpoint report:");
    current_test->ops.print_stats(SB_STAT_CUMULATIVE);
    print_global_stats();
    SB_THREAD_MUTEX_UNLOCK();
  }

  return NULL;
}

/* Callback to start timers when all threads are ready */

static int threads_started_callback(void *arg)
{
  (void) arg; /* unused */

  /* Report initialization errors to the main thread */
  if (sb_globals.error)
    return 1;

  sb_globals.num_running = sb_globals.num_threads;

  sb_timer_start(&sb_exec_timer);
  sb_timer_copy(&sb_intermediate_timer, &sb_exec_timer);
  sb_timer_copy(&sb_checkpoint_timer1, &sb_exec_timer);
  sb_timer_copy(&sb_checkpoint_timer2, &sb_exec_timer);

  log_text(LOG_NOTICE, "Threads started!\n");

  return 0;
}


/*
  Main test function: start threads, wait for them to finish and measure time.
*/

static int run_test(sb_test_t *test)
{
  int          err;
  pthread_t    report_thread;
  pthread_t    checkpoints_thread;
  pthread_t    eventgen_thread;
  unsigned int barrier_threads;

  /* initialize test */
  if (test->ops.init != NULL && test->ops.init() != 0)
    return 1;
  
  /* print test mode */
  print_run_mode(test);

  /* initialize timers */
  sb_timer_init(&sb_exec_timer);
  sb_timer_init(&sb_intermediate_timer);
  sb_timer_init(&sb_checkpoint_timer1);
  sb_timer_init(&sb_checkpoint_timer2);

  /* prepare test */
  if (test->ops.prepare != NULL && test->ops.prepare() != 0)
    return 1;

  pthread_mutex_init(&sb_globals.exec_mutex, NULL);


  pthread_mutex_init(&event_queue_mutex, NULL);    
  pthread_cond_init(&event_queue_cv, NULL);
  sb_globals.event_queue_length = 0;
  queue_is_full = 0;

  sb_globals.num_running = 0;

  pthread_mutex_init(&report_interval_mutex, NULL);

  /* Calculate the required number of threads for the start barrier */
  barrier_threads = 1 + sb_globals.num_threads +
    (sb_globals.report_interval > 0) +
    (sb_globals.tx_rate > 0) +
    (sb_globals.n_checkpoints > 0);

  /* Initialize the start barrier */
  if (sb_barrier_init(&thread_start_barrier, barrier_threads,
                      threads_started_callback, NULL)) {
    log_errno(LOG_FATAL, "sb_barrier_init() failed");
    return 1;
  }

  if (sb_globals.report_interval > 0)
  {
    /* Create a thread for intermediate statistic reports */
    if ((err = sb_thread_create(&report_thread, &sb_thread_attr,
                                &report_thread_proc, NULL)) != 0)
    {
      log_errno(LOG_FATAL,
                "sb_thread_create() for the reporting thread failed.");
      return 1;
    }
  }

  if (sb_globals.tx_rate > 0)
  {
    if ((err = sb_thread_create(&eventgen_thread, &sb_thread_attr,
                                &eventgen_thread_proc, NULL)) != 0)
    {
      log_errno(LOG_FATAL,
                "sb_thread_create() for the reporting thread failed.");
      return 1;
    }
  }

  if (sb_globals.n_checkpoints > 0)
  {
    /* Create a thread for checkpoint statistic reports */
    if ((err = sb_thread_create(&checkpoints_thread, &sb_thread_attr,
                                &checkpoints_thread_proc, NULL)) != 0)
    {
      log_errno(LOG_FATAL,
                "sb_thread_create() for the checkpoint thread failed.");
      return 1;
    }
  }

  if ((err = sb_thread_create_workers(&worker_thread)))
    return err;

#ifdef HAVE_ALARM
  /* Exit with an error if thread initialization timeout expires */
  signal(SIGALRM, sigalrm_thread_init_timeout_handler);

  alarm(THREAD_INIT_TIMEOUT);
#endif

  if (sb_barrier_wait(&thread_start_barrier) < 0)
  {
    log_text(LOG_FATAL, "Thread initialization failed!");
    return 1;
  }

#ifdef HAVE_ALARM
  alarm(0);

  if (sb_globals.force_shutdown)
  {
    /* Set the alarm to force shutdown */
    signal(SIGALRM, sigalrm_forced_shutdown_handler);

    alarm(NS2SEC(sb_globals.max_time_ns) + sb_globals.timeout);
  }
#endif

  if ((err = sb_thread_join_workers()))
    return err;

  sb_timer_stop(&sb_exec_timer);
  sb_timer_stop(&sb_intermediate_timer);
  sb_timer_stop(&sb_checkpoint_timer1);
  sb_timer_stop(&sb_checkpoint_timer2);

  /* Silence periodic reports if they were on */
  pthread_mutex_lock(&report_interval_mutex);
  sb_globals.report_interval = 0;
  pthread_mutex_unlock(&report_interval_mutex);

#ifdef HAVE_ALARM
  alarm(0);
#endif

  log_text(LOG_INFO, "Done.\n");

  /* cleanup test */
  if (test->ops.cleanup != NULL && test->ops.cleanup() != 0)
    return 1;
  
  /* print test-specific stats */
  if (test->ops.print_stats != NULL && !sb_globals.error)
    test->ops.print_stats(SB_STAT_CUMULATIVE);

  pthread_mutex_destroy(&sb_globals.exec_mutex);

  /* finalize test */
  if (test->ops.done != NULL)
    (*(test->ops.done))();

  /* Delay killing the reporting threads to avoid mutex lock leaks */
  if (report_thread_created)
  {
    if (sb_thread_cancel(report_thread) || sb_thread_join(report_thread, NULL))
      log_errno(LOG_FATAL, "Terminating the reporting thread failed.");
  }

  pthread_mutex_destroy(&report_interval_mutex);

  if (eventgen_thread_created)
  {
    if (sb_thread_cancel(eventgen_thread) ||
        sb_thread_join(eventgen_thread, NULL))
      log_text(LOG_FATAL, "Terminating the event generator thread failed.");
  }

  if (checkpoints_thread_created)
  {
    if (sb_thread_cancel(checkpoints_thread) ||
        sb_thread_join(checkpoints_thread, NULL))
      log_errno(LOG_FATAL, "Terminating the checkpoint thread failed.");
  }

  return sb_globals.error != 0;
}


static sb_test_t *find_test(const char *name)
{
  sb_list_item_t *pos;
  sb_test_t      *test;

  SB_LIST_FOR_EACH(pos, &tests)
  {
    test = SB_LIST_ENTRY(pos, sb_test_t, listitem);
    if (!strcmp(test->sname, name))
      return test;
  }

  return NULL;
}


static int checkpoint_cmp(const void *a_ptr, const void *b_ptr)
{
  const unsigned int a = *(const unsigned int *) a_ptr;
  const unsigned int b = *(const unsigned int *) b_ptr;

  return (int) (a - b);
}


static int init(void)
{
  option_t *opt;
  char     *tmp;
  sb_list_t         *checkpoints_list;
  sb_list_item_t    *pos_val;
  value_t           *val;
  long              res;

  sb_globals.num_threads = sb_get_value_int("num-threads");
  if (sb_globals.num_threads <= 0)
  {
    log_text(LOG_FATAL, "Invalid value for --num-threads: %d.\n", sb_globals.num_threads);
    return 1;
  }
  sb_globals.max_requests = sb_get_value_int("max-requests");
  sb_globals.max_time_ns = SEC2NS(sb_get_value_int("max-time"));

  if (!sb_globals.max_requests && !sb_globals.max_time_ns)
    log_text(LOG_WARNING, "WARNING: Both max-requests and max-time are 0, running endless test");

  if (sb_globals.max_time_ns > 0)
  {
    /* Parse the --forced-shutdown value */
    tmp = sb_get_value_string("forced-shutdown");
    if (tmp == NULL)
    {
      sb_globals.force_shutdown = 1;
      sb_globals.timeout = NS2SEC(sb_globals.max_time_ns) / 20;
    }
    else if (strcasecmp(tmp, "off"))
    {
      char *endptr;
    
      sb_globals.force_shutdown = 1;
      sb_globals.timeout = (unsigned) strtol(tmp, &endptr, 10);
      if (*endptr == '%')
        sb_globals.timeout = (unsigned) (sb_globals.timeout *
                                         NS2SEC(sb_globals.max_time_ns) / 100);
      else if (*tmp == '\0' || *endptr != '\0')
      {
        log_text(LOG_FATAL, "Invalid value for --forced-shutdown: '%s'", tmp);
        return 1;
      }
    }
    else
      sb_globals.force_shutdown = 0;
  }

  int err;
  if ((err = sb_thread_init()))
    return err;

  sb_globals.debug = sb_get_value_flag("debug");
  /* Automatically set logger verbosity to 'debug' */
  if (sb_globals.debug)
  {
    opt = sb_find_option("verbosity");
    if (opt != NULL)
      set_option(opt->name, "5", opt->type);
  }
  
  sb_globals.validate = sb_get_value_flag("validate");

  if (sb_rand_init())
  {
    return 1;
  }

  sb_globals.tx_rate = sb_get_value_int("tx-rate");
  sb_globals.report_interval = sb_get_value_int("report-interval");

  sb_globals.n_checkpoints = 0;
  checkpoints_list = sb_get_value_list("report-checkpoints");
  SB_LIST_FOR_EACH(pos_val, checkpoints_list)
  {
    char *endptr;

    val = SB_LIST_ENTRY(pos_val, value_t, listitem);
    res = strtol(val->data, &endptr, 10);
    if (*endptr != '\0' || res < 0 || res > UINT_MAX)
    {
      log_text(LOG_FATAL, "Invalid value for --report-checkpoints: '%s'",
               val->data);
      return 1;
    }
    if (++sb_globals.n_checkpoints > MAX_CHECKPOINTS)
    {
      log_text(LOG_FATAL, "Too many checkpoints in --report-checkpoints "
               "(up to %d can be defined)", MAX_CHECKPOINTS);
      return 1;
    }
    sb_globals.checkpoints[sb_globals.n_checkpoints-1] = (unsigned int) res;
  }

  if (sb_globals.n_checkpoints > 0)
  {
    qsort(sb_globals.checkpoints, sb_globals.n_checkpoints,
          sizeof(unsigned int), checkpoint_cmp);
  }

  return 0;
}


int main(int argc, char *argv[])
{
  sb_test_t *test = NULL;

  /* Initialize options library */
  sb_options_init();

  /* First register the logger */
  if (log_register())
    return EXIT_FAILURE;

  /* Register available tests */
  if (register_tests())
  {
    fprintf(stderr, "Failed to register tests.\n");
    return EXIT_FAILURE;
  }

  /* Parse command line arguments */
  if (parse_general_arguments(argc, argv))
    return EXIT_FAILURE;

  if (sb_get_value_flag("help"))
  {
    print_help();
    return EXIT_SUCCESS;
  }

  if (sb_get_value_flag("version"))
  {
    printf("%s\n", VERSION_STRING);
    return EXIT_SUCCESS;
  }
  
  /* Initialize global variables and logger */
  if (init() || log_init() || db_thread_stat_init())
    return EXIT_FAILURE;

  print_header();

  if (sb_globals.testname != NULL)
  {
    /* Is it a built-in test name? */
    test = find_test(sb_globals.testname);

    if (test != NULL && sb_globals.cmdname == NULL)
    {
      /* Command is a mandatory argument for built-in tests */
      fprintf(stderr, "The '%s' test requires a command argument. "
              "See 'sysbench %s help'\n", test->sname, test->sname);
      return EXIT_FAILURE;
    }

    if (test == NULL)
    {
      /* Is it a path? */
      if (access(sb_globals.testname, R_OK))
      {
        fprintf(stderr, "Cannot find script %s: %s\n", sb_globals.testname,
                strerror(errno));
        return EXIT_FAILURE;
      }

      if ((test = sb_load_lua(sb_globals.testname)) == NULL)
      {
        fprintf(stderr, "Script execution failed");
        return EXIT_FAILURE;
      }

      if (sb_globals.cmdname == NULL)
      {
        /* No command specified, there's nothing more todo */
        return test != NULL ? EXIT_SUCCESS: EXIT_FAILURE;
      }
    }
  }
  else
  {
    if (SB_ISATTY())
      log_text(LOG_NOTICE, "Reading the script from the standard input:\n");

    test = sb_load_lua(NULL);

    return test != NULL ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  current_test = test;

  /* Load and parse test-specific options */
  if (parse_test_arguments(test, argc, argv))
    return EXIT_FAILURE;

  if (sb_lua_custom_command_defined(sb_globals.cmdname))
  {
    return sb_lua_call_custom_command(sb_globals.cmdname);
  }
  else if (!strcmp(sb_globals.cmdname, "help"))
  {
    if (test->builtin_cmds.help != NULL)
    {
      test->builtin_cmds.help();
      return EXIT_SUCCESS;
    }
    else if (test->args != NULL)
    {
      printf("%s options:\n", test->sname);
      sb_print_test_options();
      return EXIT_SUCCESS;
    }

    /* We don't know want to print as help text, let the user know */
    fprintf(stderr, "'%s' test does not implement the 'help' command.\n",
            test->sname);
    return EXIT_FAILURE;
  }
  else if (!strcmp(sb_globals.cmdname, "prepare"))
  {
    if (test->builtin_cmds.prepare == NULL)
    {
      fprintf(stderr, "'%s' test does not implement the 'prepare' command.\n",
              test->sname);
      return EXIT_FAILURE;
    }

    return test->builtin_cmds.prepare();
  }
  else if (!strcmp(sb_globals.cmdname, "cleanup"))
  {
    if (test->builtin_cmds.cleanup == NULL)
    {
      fprintf(stderr, "'%s' test does not implement the 'cleanup' command.\n",
              test->sname);
      return EXIT_FAILURE;
    }

    return test->builtin_cmds.cleanup();
  }
  else if (!strcmp(sb_globals.cmdname, "run"))
  {
    if (run_test(test))
      return EXIT_FAILURE;
  }
  else
  {
    fprintf(stderr, "Unknown command: %s", sb_globals.cmdname);
    return EXIT_FAILURE;
  }

  db_done();

  log_done();

  sb_options_done();

  sb_rand_done();

  sb_thread_done();

  exit(0);
}

/* Print a description of available command line options for the current test */

void sb_print_test_options(void)
{
  if (current_test != NULL)
    sb_print_options(current_test->args);
}
