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

#include "sysbench.h"
#include "sb_options.h"
#include "scripting/sb_script.h"
#include "db_driver.h"

#define VERSION_STRING PACKAGE" "PACKAGE_VERSION

/* Large prime number to generate unique random IDs */
#define LARGE_PRIME 2147483647

/* Random numbers distributions */
typedef enum
{
  DIST_TYPE_UNIFORM,
  DIST_TYPE_GAUSSIAN,
  DIST_TYPE_SPECIAL
} rand_dist_t;

/* If we should initialize random numbers generator */
static int rand_init;
static rand_dist_t rand_type;
static int (*rand_func)(int, int); /* pointer to random numbers generator */
static unsigned int rand_iter;
static unsigned int rand_pct;
static unsigned int rand_res;
static int rand_seed; /* optional seed set on the command line */

/* Random seed used to generate unique random numbers */
static unsigned long long rnd_seed;
/* Mutex to protect random seed */
static pthread_mutex_t    rnd_mutex;

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
  {"tx-jitter", "target transaction variation, in microseconds",
    SB_ARG_TYPE_INT, "0"},
  {"report-interval", "periodically report intermediate statistics "
   "with a specified interval in seconds. 0 disables intermediate reports",
    SB_ARG_TYPE_INT, "0"},
  {"test", "test to run", SB_ARG_TYPE_STRING, NULL},
  {"debug", "print more debugging info", SB_ARG_TYPE_FLAG, "off"},
  {"validate", "perform validation checks where possible", SB_ARG_TYPE_FLAG, "off"},
  {"help", "print help and exit", SB_ARG_TYPE_FLAG, NULL},
  {"version", "print version and exit", SB_ARG_TYPE_FLAG, "off"},
  {"rand-init", "initialize random number generator", SB_ARG_TYPE_FLAG, "off"},
  {"rand-type", "random numbers distribution {uniform,gaussian,special}", SB_ARG_TYPE_STRING,
   "special"},
  {"rand-spec-iter", "number of iterations used for numbers generation", SB_ARG_TYPE_INT, "12"},
  {"rand-spec-pct", "percentage of values to be treated as 'special' (for special distribution)",
   SB_ARG_TYPE_INT, "1"},
  {"rand-spec-res", "percentage of 'special' values to use (for special distribution)",
   SB_ARG_TYPE_INT, "75"},
  {"rand-seed", "seed for random number generator, ignored when 0", SB_ARG_TYPE_INT, "0"},
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

/* Thread descriptors */
sb_thread_ctxt_t *threads;

/* List of available tests */
sb_list_t        tests;

/* Global variables */
sb_globals_t     sb_globals;
sb_test_t        *current_test;

/* Mutexes */

/* used to start test with all threads ready */
static pthread_mutex_t thread_start_mutex;
static pthread_attr_t  thread_attr;

static void print_header(void);
static void print_usage(void);
static void print_run_mode(sb_test_t *);

#ifdef HAVE_ALARM
static void sigalrm_handler(int sig)
{
  if (sig == SIGALRM)
  {
    sb_timer_stop(&sb_globals.exec_timer);
    log_text(LOG_FATAL,
             "The --max-time limit has expired, forcing shutdown...");
    if (current_test && current_test->ops.print_stats)
      current_test->ops.print_stats(SB_STAT_CUMULATIVE);
    log_done();
    exit(2);
  }
}
#endif

/* Main request provider function */ 


static sb_request_t get_request(sb_test_t *test, int thread_id)
{ 
  sb_request_t r;
  (void)thread_id; /* unused */

  if (test->ops.get_request != NULL)
    r = test->ops.get_request();
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


void print_usage(void)
{
  sb_list_item_t *pos;
  sb_test_t      *test;
  
  printf("Usage:\n");
  printf("  sysbench [general-options]... --test=<test-name> "
         "[test-options]... command\n\n");
  printf("General options:\n");
  sb_print_options(general_args);

  printf("Log options:\n");
  log_usage();

  printf("Compiled-in tests:\n");
  SB_LIST_FOR_EACH(pos, &tests)
  {
    test = SB_LIST_ENTRY(pos, sb_test_t, listitem);
    printf("  %s - %s\n", test->sname, test->lname);
  }
  printf("\n");
  printf("Commands: prepare run cleanup help version\n\n");
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
            "Target transaction rate: %d/sec, with jitter %d usec",
             sb_globals.tx_rate, sb_globals.tx_jitter);
  }

  if (sb_globals.report_interval)
  {
    log_text(LOG_NOTICE, "Report intermediate results every %d second(s)",
             sb_globals.report_interval);
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


/* Main runner test thread */


static void *runner_thread(void *arg)
{
  sb_request_t     request;
  sb_thread_ctxt_t *ctxt;
  sb_test_t        *test;
  unsigned int     thread_id;
  long long        period_ns = 0;
  long long        jitter_ns = 0;
  long long        pause_ns;
  struct timespec  target_tv, now_tv;
  
  ctxt = (sb_thread_ctxt_t *)arg;
  test = ctxt->test;
  thread_id = ctxt->id;
  
  log_text(LOG_DEBUG, "Runner thread started (%d)!", thread_id);
  if (test->ops.thread_init != NULL && test->ops.thread_init(thread_id) != 0)
  {
    sb_globals.error = 1;
    return NULL; /* thread initialization failed  */
  }

  if (sb_globals.tx_rate > 0)
  {
    /* initialize tx_rate variables */
    period_ns = round(1000000000.0 / sb_globals.tx_rate *
                      sb_globals.num_threads);
    if (sb_globals.tx_jitter > 0)
      jitter_ns = sb_globals.tx_jitter * 1000;
    else
      /* Default jitter is 1/10th of the period */
      jitter_ns = period_ns / 10;
  }
 
  /* 
    We do this to make sure all threads get to this barrier 
    about the same time 
  */
  pthread_mutex_lock(&thread_start_mutex);
  sb_globals.num_running++;
  pthread_mutex_unlock(&thread_start_mutex);

  if (sb_globals.tx_rate > 0)
  {
    /* we are time-rating transactions */
    SB_GETTIME(&target_tv);
    /* For the first transaction - ramp up */
    pause_ns = period_ns / sb_globals.num_threads * thread_id;
    add_ns_to_timespec(&target_tv, period_ns);
    usleep(pause_ns / 1000);
  }

  do
  {
    request = get_request(test, thread_id);
    /* check if we shall execute it */
    if (request.type != SB_REQ_TYPE_NULL)
    {
      if (execute_request(test, &request, thread_id))
        break; /* break if error returned (terminates only one thread) */
    }
    /* Check if we have a time limit */
    if (sb_globals.max_time != 0 &&
        sb_timer_value(&sb_globals.exec_timer) >= SEC2NS(sb_globals.max_time))
    {
      log_text(LOG_INFO, "Time limit exceeded, exiting...");
      break;
    }

    /* check if we are time-rating transactions and need to pause */
    if (sb_globals.tx_rate > 0)
    {
      add_ns_to_timespec(&target_tv, period_ns);
      SB_GETTIME(&now_tv);
      pause_ns = TIMESPEC_DIFF(target_tv, now_tv) - (jitter_ns / 2) +
	(sb_rnd() % jitter_ns);
      if (pause_ns > 5000)
        usleep(pause_ns / 1000);
    }

  } while ((request.type != SB_REQ_TYPE_NULL) && (!sb_globals.error) );

  if (test->ops.thread_done != NULL)
    test->ops.thread_done(thread_id);

  pthread_mutex_lock(&thread_start_mutex);
  sb_globals.num_running--;
  pthread_mutex_unlock(&thread_start_mutex);
  
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

  if (current_test->ops.print_stats == NULL)
  {
    log_text(LOG_DEBUG, "Reporting not supported by the current test, ",
             "terminating the reporting thread");
    return NULL;
  }

  log_text(LOG_DEBUG, "Reporting thread started");

  pthread_mutex_lock(&thread_start_mutex);
  pthread_mutex_unlock(&thread_start_mutex);

  pause_ns = interval_ns;
  prev_ns = sb_timer_value(&sb_globals.exec_timer) + interval_ns;
  for (;;)
  {
    usleep(pause_ns / 1000);
    /*
      sb_globals.report_interval may be set to 0 by the master thread
      to silence report at the end of the test
    */
    if (sb_globals.report_interval > 0)
      current_test->ops.print_stats(SB_STAT_INTERMEDIATE);
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


/* 
  Main test function. Start threads. 
  Wait for them to complete and measure time 
*/


static int run_test(sb_test_t *test)
{
  unsigned int i;
  int          err;
  pthread_t    report_thread;
  int          report_thread_created = 0;

  /* initialize test */
  if (test->ops.init != NULL && test->ops.init() != 0)
    return 1;
  
  /* print test mode */
  print_run_mode(test);

  /* initialize and start timers */
  sb_timer_init(&sb_globals.exec_timer);
  for(i = 0; i < sb_globals.num_threads; i++)
  {
    sb_timer_init(&sb_globals.op_timers[i]);
    threads[i].id = i;
    threads[i].test = test;
  }    

  /* prepare test */
  if (test->ops.prepare != NULL && test->ops.prepare() != 0)
    return 1;

  pthread_mutex_init(&sb_globals.exec_mutex, NULL);

  /* start mutex used for barrier */
  pthread_mutex_init(&thread_start_mutex,NULL);    
  pthread_mutex_lock(&thread_start_mutex);
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

  /* Starting the test threads */
  for(i = 0; i < sb_globals.num_threads; i++)
  {
    if (sb_globals.error)
      return 1;
    if ((err = pthread_create(&(threads[i].thread), &thread_attr,
                              &runner_thread, (void*)(threads + i))) != 0)
    {
      log_errno(LOG_FATAL, "pthread_create() for thread #%d failed.", i);
      return 1;
    }
  }

  sb_timer_start(&sb_globals.exec_timer); /* Start benchmark timer */

#ifdef HAVE_ALARM
  /* Set the alarm to force shutdown */
  if (sb_globals.force_shutdown)
    alarm(sb_globals.max_time + sb_globals.timeout);
#endif
  
  pthread_mutex_unlock(&thread_start_mutex);
  
  log_text(LOG_NOTICE, "Threads started!\n");  
  for(i = 0; i < sb_globals.num_threads; i++)
  {
    if((err = pthread_join(threads[i].thread, NULL)) != 0)
      log_errno(LOG_FATAL, "pthread_join() for thread #%d failed.", i);
  }

  sb_timer_stop(&sb_globals.exec_timer);

  /* Silence periodic reports if they were on */
  sb_globals.report_interval = 0;

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

  pthread_mutex_destroy(&thread_start_mutex);

  /* finalize test */
  if (test->ops.done != NULL)
    (*(test->ops.done))();

  /* Delay killing the reporting thread to avoid mutex lock leaks */
  if (report_thread_created)
  {
    if (pthread_cancel(report_thread) || pthread_join(report_thread, NULL))
      log_errno(LOG_FATAL, "Terminating the reporting thread failed.");
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


static int init(void)
{
  option_t *opt;
  char     *s;
  char     *tmp;
  
  sb_globals.num_threads = sb_get_value_int("num-threads");
  if (sb_globals.num_threads <= 0)
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
  
  sb_globals.op_timers = (sb_timer_t *)malloc(sb_globals.num_threads * 
                                              sizeof(sb_timer_t));
  threads = (sb_thread_ctxt_t *)malloc(sb_globals.num_threads * 
                                       sizeof(sb_thread_ctxt_t));
  if (sb_globals.op_timers == NULL || threads == NULL)
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
  else
  {
    log_text(LOG_FATAL, "Invalid random numbers distribution: %s.", s);
    return 1;
  }

  rand_iter = sb_get_value_int("rand-spec-iter");
  rand_pct = sb_get_value_int("rand-spec-pct");
  rand_res = sb_get_value_int("rand-spec-res");

  sb_globals.tx_rate = sb_get_value_int("tx-rate");
  sb_globals.tx_jitter = sb_get_value_int("tx-jitter");
  sb_globals.report_interval =
    sb_get_value_int("report-interval");
  
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
    print_usage();
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
    print_usage();
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
      print_usage();
    else
    {
      if (test->args != NULL)
      {
        printf("%s options:\n", test->sname);
        sb_print_options(test->args);
      }
      if (test->cmds.help != NULL)
        test->cmds.help();
      else
        fprintf(stderr, "No help available for test '%s'.\n",
                test->sname);
    }
    exit(0);
  }

  if (testname == NULL)
  {
    fprintf(stderr, "Missing required argument: --test.\n");
    print_usage();
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
#ifdef HAVE_ALARM
  signal(SIGALRM, sigalrm_handler);
#endif
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
