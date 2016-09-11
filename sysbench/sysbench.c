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

#include "sysbench.h"
#include "sb_options.h"
#include "scripting/sb_script.h"
#include "db_driver.h"
#include "sb_barrier.h"

#define VERSION_STRING PACKAGE" "PACKAGE_VERSION

/* Large prime number to generate unique random IDs */
#define LARGE_PRIME 2147483647

/* Maximum queue length for the tx-rate mode */
#define MAX_QUEUE_LEN 100000

/* Wait at most this number of seconds for worker threads to initialize */
#define THREAD_INIT_TIMEOUT 30

/* Random numbers distributions */
typedef enum
{
  DIST_TYPE_UNIFORM,
  DIST_TYPE_GAUSSIAN,
  DIST_TYPE_SPECIAL,
  DIST_TYPE_PARETO
} rand_dist_t;

/* Event queue data type for the tx-rate mode */
typedef struct {
  unsigned long long event_time;
  sb_list_item_t     listitem;
} event_queue_elem_t;

/* If we should initialize random numbers generator */
static int rand_init;
static rand_dist_t rand_type;
static int (*rand_func)(int, int); /* pointer to random numbers generator */
static unsigned int rand_iter;
static unsigned int rand_pct;
static unsigned int rand_res;
static int rand_seed; /* optional seed set on the command line */

/* parameters for Pareto distribution */
static double pareto_h; /* parameter h */
static double pareto_power; /* parameter pre-calculated by h */

/* Random seed used to generate unique random numbers */
static unsigned long long rnd_seed;
/* Mutex to protect random seed */
static pthread_mutex_t    rnd_mutex;
/* Mutex to protect report_interval */
static pthread_mutex_t    report_interval_mutex;

/* Stack size for each thread */
static int thread_stack_size;

/* General options */
sb_arg_t general_args[] =
{
  {"num-threads", "number of threads to use", SB_ARG_TYPE_INT, "1"},
  {"max-requests", "limit for total number of requests", SB_ARG_TYPE_INT, "10000"},
  {"max-time", "limit for total execution time in seconds", SB_ARG_TYPE_INT, "0"},
  {"forced-shutdown", "amount of time to wait after --max-time before forcing shutdown",
   SB_ARG_TYPE_STRING, "off"},
  {"thread-stack-size", "size of stack per thread", SB_ARG_TYPE_SIZE, "64K"},
  {"tx-rate", "target transaction rate (tps)", SB_ARG_TYPE_INT, "0"},
  {"report-interval", "periodically report intermediate statistics "
   "with a specified interval in seconds. 0 disables intermediate reports",
    SB_ARG_TYPE_INT, "0"},
  {"report-checkpoints", "dump full statistics and reset all counters at "
   "specified points in time. The argument is a list of comma-separated values "
   "representing the amount of time in seconds elapsed from start of test "
   "when report checkpoint(s) must be performed. Report checkpoints are off by "
   "default.", SB_ARG_TYPE_LIST, ""},
  {"test", "test to run", SB_ARG_TYPE_STRING, NULL},
  {"debug", "print more debugging info", SB_ARG_TYPE_FLAG, "off"},
  {"validate", "perform validation checks where possible", SB_ARG_TYPE_FLAG, "off"},
  {"help", "print help and exit", SB_ARG_TYPE_FLAG, NULL},
  {"version", "print version and exit", SB_ARG_TYPE_FLAG, "off"},
  {"rand-init", "initialize random number generator", SB_ARG_TYPE_FLAG, "off"},
  {"rand-type", "random numbers distribution {uniform,gaussian,special,pareto}",
   SB_ARG_TYPE_STRING, "special"},
  {"rand-spec-iter", "number of iterations used for numbers generation", SB_ARG_TYPE_INT, "12"},
  {"rand-spec-pct", "percentage of values to be treated as 'special' (for special distribution)",
   SB_ARG_TYPE_INT, "1"},
  {"rand-spec-res", "percentage of 'special' values to use (for special distribution)",
   SB_ARG_TYPE_INT, "75"},
  {"rand-seed", "seed for random number generator, ignored when 0", SB_ARG_TYPE_INT, "0"},
  {"rand-pareto-h", "parameter h for pareto distibution", SB_ARG_TYPE_FLOAT,
   "0.2"},
  {"config-file", "File containing command line options", SB_ARG_TYPE_FILE, NULL},
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

/* Thread descriptors */
sb_thread_ctxt_t *threads;

/* List of available tests */
sb_list_t        tests;

/* Global variables */
sb_globals_t     sb_globals;
sb_test_t        *current_test;

/* Barrier to ensure we start the benchmark run when all workers are ready */
static sb_barrier_t thread_start_barrier;

/* Mutexes */

static pthread_attr_t  thread_attr;

/* structures to handle queue of events, needed for tx_rate mode */
pthread_mutex_t           event_queue_mutex;
static sb_list_t          event_queue;
static pthread_cond_t     event_queue_cv;
static event_queue_elem_t queue_array[MAX_QUEUE_LEN];

static int queue_is_full;

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

  sb_timer_stop(&sb_globals.exec_timer);
  sb_timer_stop(&sb_globals.cumulative_timer1);
  sb_timer_stop(&sb_globals.cumulative_timer2);

  log_text(LOG_FATAL,
           "The --max-time limit has expired, forcing shutdown...");

  if (current_test && current_test->ops.print_stats)
    current_test->ops.print_stats(SB_STAT_CUMULATIVE);

  log_done();

  exit(2);
}
#endif

/* Main request provider function */ 


static sb_request_t get_request(sb_test_t *test, int thread_id)
{ 
  sb_request_t r;
  (void)thread_id; /* unused */

  if (test->ops.get_request != NULL)
    r = test->ops.get_request(thread_id);
  else
  { 
    log_text(LOG_ALERT, "Unsupported mode! Creating NULL request.");
    r.type = SB_REQ_TYPE_NULL;        
  }
  
  return r; 
}


/* Main request execution function */


static int execute_request(sb_test_t *test, sb_request_t *r,int thread_id)
{
  unsigned int rc;

  if (test->ops.execute_request != NULL)
    rc = test->ops.execute_request(r, thread_id);
  else
  {
    log_text(LOG_FATAL, "Unknown request. Aborting");
    rc = 1;
  }

  return rc;
}


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
    ;
}


/* Print program header */


void print_header(void)
{
  log_text(LOG_NOTICE, VERSION_STRING
         ":  multi-threaded system evaluation benchmark\n");
}


/* Print program usage */


void print_help(void)
{
  sb_list_item_t *pos;
  sb_test_t      *test;
  
  printf("Usage:\n");
  printf("  sysbench --test=<test-name> [options]... <command>\n\n");
  printf("Commands: prepare run cleanup help version\n\n");
  printf("General options:\n");
  sb_print_options(general_args);

  log_print_help();

  db_print_help();

  printf("Compiled-in tests:\n");
  SB_LIST_FOR_EACH(pos, &tests)
  {
    test = SB_LIST_ENTRY(pos, sb_test_t, listitem);
    printf("  %s - %s\n", test->sname, test->lname);
  }
  printf("\n");
  printf("See 'sysbench --test=<name> help' for a list of options for each test.\n\n");
}


static sb_cmd_t parse_command(char *cmd)
{
  if (!strcmp(cmd, "prepare"))
    return SB_COMMAND_PREPARE;
  else if (!strcmp(cmd, "run"))
    return SB_COMMAND_RUN;
  else if (!strcmp(cmd, "help"))
    return SB_COMMAND_HELP;
  else if (!strcmp(cmd, "cleanup"))
    return SB_COMMAND_CLEANUP;
  else if (!strcmp(cmd, "version"))
    return SB_COMMAND_VERSION;

  return SB_COMMAND_NULL;
}


static int parse_arguments(int argc, char *argv[])
{
  int               i;
  char              *name;
  char              *value;
  char              *tmp;
  sb_list_item_t    *pos;
  sb_test_t         *test;
  option_t          *opt;
  
  sb_globals.command = SB_COMMAND_NULL;

  /* Set default values for general options */
  if (sb_register_arg_set(general_args))
    return 1;
  /* Set default values for test specific options */
  SB_LIST_FOR_EACH(pos, &tests)
  {
    test = SB_LIST_ENTRY(pos, sb_test_t, listitem);
    if (test->args == NULL)
      break;
    if (sb_register_arg_set(test->args))
      return 1;
  }
  
  /* Parse command line arguments */
  for (i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--", 2)) {
      if (sb_globals.command != SB_COMMAND_NULL)
      {
        fprintf(stderr, "Multiple commands are not allowed.\n");
        return 1;
      }
      sb_globals.command = parse_command(argv[i]);
      if (sb_globals.command == SB_COMMAND_NULL)
      {
        fprintf(stderr, "Unknown command: %s.\n", argv[i]);
        return 1;
      }
      continue;
    }
    name = argv[i] + 2;
    tmp = strchr(name, '=');
    if (tmp != NULL)
    {
      *tmp = '\0';
      value = tmp + 1;
    } else
      value = NULL;

    if (sb_globals.command == SB_COMMAND_HELP)
      return 1;
    if (sb_globals.command == SB_COMMAND_VERSION)
      return 0;
    
    /* Search available options */
    opt = sb_find_option(name);
    if (opt == NULL)
    {
      if (set_option(name, value, SB_ARG_TYPE_STRING))
        return 1;
    }
    else if (set_option(name, value, opt->type))
      return 1;
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

  if (rand_init)
  {
    log_text(LOG_NOTICE, "Initializing random number generator from timer.\n");
    sb_srnd(time(NULL));
  }

  if (rand_seed)
  {
    log_text(LOG_NOTICE, "Initializing random number generator from seed (%d).\n", rand_seed);
    sb_srnd(rand_seed);
  }
  else
  {
    log_text(LOG_NOTICE, "Random number generator seed is 0 and will be ignored\n");
  }

  if (sb_globals.force_shutdown)
    log_text(LOG_NOTICE, "Forcing shutdown in %u seconds",
             sb_globals.max_time + sb_globals.timeout);
  
  log_text(LOG_NOTICE, "");

  if (test->ops.print_mode != NULL)
    test->ops.print_mode();
}


/* Main worker test thread */


static void *worker_thread(void *arg)
{
  sb_request_t        request;
  sb_thread_ctxt_t   *ctxt;
  sb_test_t          *test;
  unsigned int        thread_id;
  unsigned long long  queue_start_time = 0;
  sb_list_item_t     *pos;
  event_queue_elem_t *event;

  ctxt = (sb_thread_ctxt_t *)arg;
  test = ctxt->test;
  thread_id = ctxt->id;

  if (test->ops.thread_init != NULL && test->ops.thread_init(thread_id) != 0)
  {
    log_text(LOG_DEBUG, "Worker thread (#%d) failed to initialize!", thread_id);
    sb_globals.error = 1;
    /* Avoid blocking the main thread */
    sb_barrier_wait(&thread_start_barrier);
    return NULL;
  }

  log_text(LOG_DEBUG, "Worker thread (#%d) started!", thread_id);

  /* Wait for other threads to initialize */
  if (sb_barrier_wait(&thread_start_barrier) < 0)
    return NULL;

  do
  {

    /* If we are in tx_rate mode, we take events from queue */
    if (sb_globals.tx_rate > 0)
    {
      if (queue_is_full)
      {
        log_text(LOG_FATAL, "Event queue is full.");
        break;
      }
      pthread_mutex_lock(&event_queue_mutex);
      while(!sb_globals.event_queue_length)
        pthread_cond_wait(&event_queue_cv, &event_queue_mutex);

      SB_LIST_ONCE(pos, &event_queue)
      {
        event = SB_LIST_ENTRY(pos, event_queue_elem_t, listitem);
        queue_start_time = event->event_time;;

        SB_LIST_DELETE(pos);

        sb_globals.event_queue_length--;
      }

      sb_globals.concurrency++;

      pthread_mutex_unlock(&event_queue_mutex);

      timers[thread_id].queue_time = sb_timer_value(&sb_globals.exec_timer) -
        queue_start_time;

    }

    request = get_request(test, thread_id);

    /* check if we shall execute it */
    if (request.type != SB_REQ_TYPE_NULL)
    {
      if (execute_request(test, &request, thread_id))
        break; /* break if error returned (terminates only one thread) */
    }

    if (sb_globals.tx_rate > 0)
    {
      pthread_mutex_lock(&event_queue_mutex);
      sb_globals.concurrency--;
      pthread_mutex_unlock(&event_queue_mutex);
    }

    /* Check if we have a time limit */
    if (sb_globals.max_time != 0 &&
        sb_timer_value(&sb_globals.exec_timer) >= SEC2NS(sb_globals.max_time))
    {
      log_text(LOG_INFO, "Time limit exceeded, exiting...");
      break;
    }

  } while ((request.type != SB_REQ_TYPE_NULL) && (!sb_globals.error) );

  if (test->ops.thread_done != NULL)
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

  curr_ns = sb_timer_value(&sb_globals.exec_timer);
  /* emulate exponential distribution with Lambda = tx_rate */
  intr_ns = (long) (log(1 - (double) sb_rnd() / (double) SB_MAX_RND) /
                    (-(double) sb_globals.tx_rate)*1000000);
  next_ns = curr_ns + intr_ns*1000;

  for (;;)
  {
    curr_ns = sb_timer_value(&sb_globals.exec_timer);

    /* emulate exponential distribution with Lambda = tx_rate */
    intr_ns = (long) (log(1 - (double)sb_rnd() / (double)SB_MAX_RND) /
                      (-(double)sb_globals.tx_rate)*1000000);

    next_ns = next_ns + intr_ns*1000;
    if (next_ns > curr_ns)
    {
      pause_ns = next_ns - curr_ns;
      usleep(pause_ns / 1000);
    }
    else
    {
      log_timestamp(LOG_DEBUG, &sb_globals.exec_timer,
                    "Event generation thread is too slow");
    }


    queue_array[i].event_time = sb_timer_value(&sb_globals.exec_timer);
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
    log_text(LOG_DEBUG, "Reporting not supported by the current test, ",
             "terminating the reporting thread");
    return NULL;
  }

  pause_ns = interval_ns;
  prev_ns = sb_timer_value(&sb_globals.exec_timer) + interval_ns;
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

    curr_ns = sb_timer_value(&sb_globals.exec_timer);
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
    log_text(LOG_DEBUG, "Reporting not supported by the current test, ",
             "terminating the checkpoints thread");
    return NULL;
  }

  for (i = 0; i < sb_globals.n_checkpoints; i++)
  {
    next_ns = SEC2NS(sb_globals.checkpoints[i]);
    curr_ns = sb_timer_value(&sb_globals.exec_timer);
    if (next_ns <= curr_ns)
      continue;

    pause_ns = next_ns - curr_ns;
    usleep(pause_ns / 1000);
    /*
      Just to update elapsed time in timer which is alter used by
      log_timestamp.
    */
    curr_ns = sb_timer_value(&sb_globals.exec_timer);

    SB_THREAD_MUTEX_LOCK();
    log_timestamp(LOG_NOTICE, &sb_globals.exec_timer, "Checkpoint report:");
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

  sb_timer_start(&sb_globals.exec_timer);
  sb_timer_start(&sb_globals.cumulative_timer1);
  sb_timer_start(&sb_globals.cumulative_timer2);

  return 0;
}

/*
  Main test function. Start threads.
  Wait for them to complete and measure time
*/

static int run_test(sb_test_t *test)
{
  unsigned int i;
  int          err;
  pthread_t    report_thread;
  pthread_t    checkpoints_thread;
  pthread_t    eventgen_thread;
  int          report_thread_created      = 0;
  int          checkpoints_thread_created = 0;
  int          eventgen_thread_created    = 0;
  unsigned int barrier_threads;

  /* initialize test */
  if (test->ops.init != NULL && test->ops.init() != 0)
    return 1;
  
  /* print test mode */
  print_run_mode(test);

  /* initialize and start timers */
  sb_timer_init(&sb_globals.exec_timer);
  sb_timer_init(&sb_globals.cumulative_timer1);
  sb_timer_init(&sb_globals.cumulative_timer2);
  for(i = 0; i < sb_globals.num_threads; i++)
  {
    threads[i].id = i;
    threads[i].test = test;
  }    

  /* prepare test */
  if (test->ops.prepare != NULL && test->ops.prepare() != 0)
    return 1;

  pthread_mutex_init(&sb_globals.exec_mutex, NULL);


  pthread_mutex_init(&event_queue_mutex, NULL);    
  pthread_cond_init(&event_queue_cv, NULL);
  sb_globals.event_queue_length = 0;
  queue_is_full = 0;

  sb_globals.num_running = 0;

  /* initialize attr */
  pthread_attr_init(&thread_attr);
#ifdef PTHREAD_SCOPE_SYSTEM
  pthread_attr_setscope(&thread_attr,PTHREAD_SCOPE_SYSTEM);
#endif
  pthread_attr_setstacksize(&thread_attr, thread_stack_size);

#ifdef HAVE_THR_SETCONCURRENCY
  /* Set thread concurrency (required on Solaris) */
  thr_setconcurrency(sb_globals.num_threads);
#endif
  
  /* Initialize random seed  */
  rnd_seed = LARGE_PRIME;
  pthread_mutex_init(&rnd_mutex, NULL);
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
    if ((err = pthread_create(&report_thread, &thread_attr, &report_thread_proc,
                              NULL)) != 0)
    {
      log_errno(LOG_FATAL, "pthread_create() for the reporting thread failed.");
      return 1;
    }
    report_thread_created = 1;
  }

  if (sb_globals.tx_rate > 0)
  {
    if ((err = pthread_create(&eventgen_thread, &thread_attr, &eventgen_thread_proc,
                              NULL)) != 0)
    {
      log_errno(LOG_FATAL, "pthread_create() for the reporting thread failed.");
      return 1;
    }
    eventgen_thread_created = 1;
  }

  if (sb_globals.n_checkpoints > 0)
  {
    /* Create a thread for checkpoint statistic reports */
    if ((err = pthread_create(&checkpoints_thread, &thread_attr,
                              &checkpoints_thread_proc, NULL)) != 0)
    {
      log_errno(LOG_FATAL, "pthread_create() for the checkpoint thread "
                "failed.");
      return 1;
    }
    checkpoints_thread_created = 1;
  }

  /* Starting the worker threads */
  for(i = 0; i < sb_globals.num_threads; i++)
  {
    if ((err = pthread_create(&(threads[i].thread), &thread_attr,
                              &worker_thread, (void*)(threads + i))) != 0)
    {
      log_errno(LOG_FATAL, "pthread_create() for thread #%d failed.", i);
      return 1;
    }
  }

#ifdef HAVE_ALARM
  /* Exit with an error if thread initialization timeout expires */
  signal(SIGALRM, sigalrm_thread_init_timeout_handler);

  alarm(THREAD_INIT_TIMEOUT);
#endif

  log_text(LOG_NOTICE, "Initializing worker threads...\n");

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

    alarm(sb_globals.max_time + sb_globals.timeout);
  }
#endif

  log_text(LOG_NOTICE, "Threads started!\n");

  for(i = 0; i < sb_globals.num_threads; i++)
  {
    if((err = pthread_join(threads[i].thread, NULL)) != 0)
      log_errno(LOG_FATAL, "pthread_join() for thread #%d failed.", i);

    sb_globals.num_running--;
  }

  sb_timer_stop(&sb_globals.exec_timer);
  sb_timer_stop(&sb_globals.cumulative_timer1);
  sb_timer_stop(&sb_globals.cumulative_timer2);

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

  pthread_mutex_destroy(&rnd_mutex);

  pthread_mutex_destroy(&sb_globals.exec_mutex);

  /* finalize test */
  if (test->ops.done != NULL)
    (*(test->ops.done))();

  /* Delay killing the reporting threads to avoid mutex lock leaks */
  if (report_thread_created)
  {
    if (pthread_cancel(report_thread) || pthread_join(report_thread, NULL))
      log_errno(LOG_FATAL, "Terminating the reporting thread failed.");
  }

  pthread_mutex_destroy(&report_interval_mutex);

  if (eventgen_thread_created)
  {
    if (pthread_cancel(eventgen_thread) || pthread_join(eventgen_thread, NULL))
      log_text(LOG_FATAL, "Terminating the event generator thread failed.");
  }

  if (checkpoints_thread_created)
  {
    if (pthread_cancel(checkpoints_thread) ||
        pthread_join(checkpoints_thread, NULL))
      log_errno(LOG_FATAL, "Terminating the checkpoint thread failed.");
  }

  return sb_globals.error != 0;
}


static sb_test_t *find_test(char *name)
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
  char     *s;
  char     *tmp;
  sb_list_t         *checkpoints_list;
  sb_list_item_t    *pos_val;
  value_t           *val;
  long              res;

  sb_globals.num_threads = sb_get_value_int("num-threads");
  if (sb_globals.num_threads == 0)
  {
    log_text(LOG_FATAL, "Invalid value for --num-threads: %d.\n", sb_globals.num_threads);
    return 1;
  }
  sb_globals.max_requests = sb_get_value_int("max-requests");
  sb_globals.max_time = sb_get_value_int("max-time");
  if (!sb_globals.max_requests && !sb_globals.max_time)
    log_text(LOG_WARNING, "WARNING: Both max-requests and max-time are 0, running endless test");

  if (sb_globals.max_time > 0)
  {
    /* Parse the --forced-shutdown value */
    tmp = sb_get_value_string("forced-shutdown");
    if (tmp == NULL)
    {
      sb_globals.force_shutdown = 1;
      sb_globals.timeout = sb_globals.max_time / 20;
    }
    else if (strcasecmp(tmp, "off"))
    {
      char *endptr;
    
      sb_globals.force_shutdown = 1;
      sb_globals.timeout = (unsigned int)strtol(tmp, &endptr, 10);
      if (*endptr == '%')
        sb_globals.timeout = (unsigned int)(sb_globals.timeout *
                                            (double)sb_globals.max_time / 100);
      else if (*tmp == '\0' || *endptr != '\0')
      {
        log_text(LOG_FATAL, "Invalid value for --forced-shutdown: '%s'", tmp);
        return 1;
      }
    }
    else
      sb_globals.force_shutdown = 0;
  }

  threads = (sb_thread_ctxt_t *)malloc(sb_globals.num_threads * 
                                       sizeof(sb_thread_ctxt_t));
  if (threads == NULL)
  {
    log_text(LOG_FATAL, "Memory allocation failure.\n");
    return 1;
  }

  thread_stack_size = sb_get_value_size("thread-stack-size");
  if (thread_stack_size <= 0)
  {
    log_text(LOG_FATAL, "Invalid value for thread-stack-size: %d.\n", thread_stack_size);
    return 1;
  }

  sb_globals.debug = sb_get_value_flag("debug");
  /* Automatically set logger verbosity to 'debug' */
  if (sb_globals.debug)
  {
    opt = sb_find_option("verbosity");
    if (opt != NULL)
      set_option(opt->name, "5", opt->type);
  }
  
  sb_globals.validate = sb_get_value_flag("validate");

  rand_init = sb_get_value_flag("rand-init"); 
  rand_seed = sb_get_value_int("rand-seed"); 
  if (rand_init && rand_seed)
  {
    log_text(LOG_FATAL, "Cannot set both --rand-init and --rand-seed");
    return 1;
  }

  s = sb_get_value_string("rand-type");
  if (!strcmp(s, "uniform"))
  {
    rand_type = DIST_TYPE_UNIFORM;
    rand_func = &sb_rand_uniform;
  }
  else if (!strcmp(s, "gaussian"))
  {
    rand_type = DIST_TYPE_GAUSSIAN;
    rand_func = &sb_rand_gaussian;
  }
  else if (!strcmp(s, "special"))
  {
    rand_type = DIST_TYPE_SPECIAL;
    rand_func = &sb_rand_special;
  }
  else if (!strcmp(s, "pareto"))
  {
    rand_type = DIST_TYPE_PARETO;
    rand_func = &sb_rand_pareto;
  }
  else
  {
    log_text(LOG_FATAL, "Invalid random numbers distribution: %s.", s);
    return 1;
  }

  rand_iter = sb_get_value_int("rand-spec-iter");
  rand_pct = sb_get_value_int("rand-spec-pct");
  rand_res = sb_get_value_int("rand-spec-res");

  pareto_h  = sb_get_value_float("rand-pareto-h");
  pareto_power = log(pareto_h) / log(1.0-pareto_h);

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
  char      *testname;
  sb_test_t *test = NULL;
  
  /* Initialize options library */
  sb_options_init();

  /* First register the logger */
  if (log_register())
    exit(1);

  /* Register available tests */
  if (register_tests())
  {
    fprintf(stderr, "Failed to register tests.\n");
    exit(1);
  }

  /* Parse command line arguments */
  if (parse_arguments(argc,argv))
  {
    print_help();
    exit(1);
  }

  if (sb_globals.command == SB_COMMAND_VERSION || sb_get_value_flag("version"))
  {
    printf("%s\n", VERSION_STRING);
    exit(0);
  }
  
  if (sb_globals.command == SB_COMMAND_NULL)
  {
    fprintf(stderr, "Missing required command argument.\n");
    print_help();
    exit(1);
  }

  /* Initialize global variables and logger */
  if (init() || log_init())
    exit(1);
  
  print_header();

  testname = sb_get_value_string("test");
  if (testname != NULL)
  {
    test = find_test(testname);
    
    /* Check if the testname is a script filename */
    if (test == NULL)
      test = script_load(testname);
  }

  /* 'help' command */
  if (sb_globals.command == SB_COMMAND_HELP)
  {
    if (test == NULL)
      print_help();
    else
    {
      if (test->args != NULL)
      {
        printf("%s options:\n", test->sname);
        sb_print_options(test->args);
      }
      if (test->cmds.help != NULL)
        test->cmds.help();
    }
    exit(0);
  }

  if (testname == NULL)
  {
    fprintf(stderr, "Missing required argument: --test.\n");
    print_help();
    exit(1);
  }

  if (test == NULL)
  {
      fprintf(stderr, "Invalid test name: %s.\n", testname);
      exit(1);
  }
  
  /* 'prepare' command */
  if (sb_globals.command == SB_COMMAND_PREPARE)
  {
    if (test->cmds.prepare == NULL)
    {
      fprintf(stderr, "'%s' test does not have the 'prepare' command.\n",
              test->sname);
      exit(1);
    }

    return test->cmds.prepare();
  }

  /* 'cleanup' command */
  if (sb_globals.command == SB_COMMAND_CLEANUP)
  {
    if (test->cmds.cleanup == NULL)
    {
      fprintf(stderr, "'%s' test does not have the 'cleanup' command.\n",
              test->sname);
      exit(1);
    }

    exit(test->cmds.cleanup());
  }
  
  /* 'run' command */
  current_test = test;
  if (run_test(test))
    exit(1);

  /* Uninitialize logger */
  log_done();
  
  exit(0);
}

/*
  Return random number in specified range with distribution specified
  with the --rand-type command line option
*/

int sb_rand(int a, int b)
{
  return rand_func(a,b);
}

/* uniform distribution */

int sb_rand_uniform(int a, int b)
{
  return a + sb_rnd() % (b - a + 1);
}

/* gaussian distribution */

int sb_rand_gaussian(int a, int b)
{
  int          sum;
  unsigned int i, t;

  t = b - a + 1;
  for(i=0, sum=0; i < rand_iter; i++)
    sum += sb_rnd() % t;
  
  return a + sum / rand_iter;
}

/* 'special' distribution */

int sb_rand_special(int a, int b)
{
  int          sum = 0;
  unsigned int i;
  unsigned int d;
  unsigned int t;
  unsigned int res;
  unsigned int range_size;
  
  if (a >= b)
    return 0;

  t = b - a + 1;
  
  /* Increase range size for special values. */
  range_size = t * (100 / (100 - rand_res));
  
  /* Generate uniformly distributed one at this stage  */
  res = sb_rnd() % range_size;
  
  /* For first part use gaussian distribution */
  if (res < t)
  {
    for(i = 0; i < rand_iter; i++)
      sum += sb_rnd() % t;
    return a + sum / rand_iter;  
  }

  /*
   * For second part use even distribution mapped to few items 
   * We shall distribute other values near by the center
   */
  d = t * rand_pct / 100;
  if (d < 1)
    d = 1;
  res %= d;
   
  /* Now we have res values in SPECIAL_PCT range of the data */
  res += (t / 2 - t * rand_pct / (100 * 2));
   
  return a + res;
}

/* Pareto distribution */

int sb_rand_pareto(int a, int b)
{
  return a + (int)(b - a + 1) * pow(sb_rnd_double(), pareto_power);
}

/* Generate unique random id */


int sb_rand_uniq(int a, int b)
{
  int res;

  pthread_mutex_lock(&rnd_mutex);
  res = (unsigned int) (rnd_seed % (b - a + 1)) ;
  rnd_seed += LARGE_PRIME;
  pthread_mutex_unlock(&rnd_mutex);

  return res + a;
}


/* Generate random string */


void sb_rand_str(const char *fmt, char *buf)
{
  unsigned int i;

  for (i=0; fmt[i] != '\0'; i++)
  {
    if (fmt[i] == '#')
      buf[i] = sb_rand_uniform('0', '9');
    else if (fmt[i] == '@')
      buf[i] = sb_rand_uniform('a', 'z');
    else
      buf[i] = fmt[i];
  }
}
