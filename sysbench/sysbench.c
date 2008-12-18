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

#include "sysbench.h"
#include "sb_options.h"
#include "scripting/sb_script.h"
#include "db_driver.h"

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
  {"thread-stack-size", "size of stack per thread", SB_ARG_TYPE_SIZE, "64K"},
  {"test", "test to run", SB_ARG_TYPE_STRING, NULL},
  {"debug", "print more debugging info", SB_ARG_TYPE_FLAG, "off"},
  {"validate", "perform validation checks where possible", SB_ARG_TYPE_FLAG, "off"},
  {"help", "print help and exit", SB_ARG_TYPE_FLAG, NULL},
  {"rand-init", "initialize random number generator", SB_ARG_TYPE_FLAG, "off"},
  {"rand-type", "random numbers distribution {uniform,gaussian,special}", SB_ARG_TYPE_STRING,
   "special"},
  {"rand-spec-iter", "number of iterations used for numbers generation", SB_ARG_TYPE_INT, "12"},
  {"rand-spec-pct", "percentage of values to be treated as 'special' (for special distribution)",
   SB_ARG_TYPE_INT, "1"},
  {"rand-spec-res", "percentage of 'special' values to use (for special distribution)",
   SB_ARG_TYPE_INT, "75"},
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

/* Thread descriptors */
sb_thread_ctxt_t *threads;

/* List of available tests */
sb_list_t        tests;

/* Global variables */
sb_globals_t     sb_globals;

/* Mutexes */

/* used to start test with all threads ready */
static pthread_mutex_t thread_start_mutex;
static pthread_attr_t  thread_attr;

static void print_header(void);
static void print_usage(void);
static void print_run_mode(sb_test_t *);


/* Main request provider function */ 


sb_request_t get_request(sb_test_t *test, int thread_id)
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


int execute_request(sb_test_t *test, sb_request_t *r,int thread_id)
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


int register_test(sb_test_t *test)
{
  SB_LIST_ADD_TAIL(&test->listitem, &tests);

  return 0;
}


int register_tests(void)
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
  log_text(LOG_NOTICE, PACKAGE" v"PACKAGE_VERSION
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
  printf("Compiled-in tests:\n");
  SB_LIST_FOR_EACH(pos, &tests)
  {
    test = SB_LIST_ENTRY(pos, sb_test_t, listitem);
    printf("  %s - %s\n", test->sname, test->lname);
  }
  printf("\n");
  printf("Commands: prepare run cleanup help\n\n");
  printf("See 'sysbench --test=<name> help' for a list of options for each test.\n\n");
}


sb_arg_t *find_argument(char *name, sb_arg_t *args)
{
  unsigned int i;

  if (args == NULL)
    return NULL;
  
  for (i = 0; args[i].name != NULL; i++)
    if (!strcasecmp(args[i].name, name))
      return &(args[i]);

  return NULL;
}


sb_cmd_t parse_command(char *cmd)
{
  if (!strcmp(cmd, "prepare"))
    return SB_COMMAND_PREPARE;
  else if (!strcmp(cmd, "run"))
    return SB_COMMAND_RUN;
  else if (!strcmp(cmd, "help"))
    return SB_COMMAND_HELP;
  else if (!strcmp(cmd, "cleanup"))
    return SB_COMMAND_CLEANUP;

  return SB_COMMAND_NULL;
}


int parse_arguments(int argc, char *argv[])
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

    if (!strcmp(name, "help"))
      return 1;
    
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

  if (sb_globals.debug)
    log_text(LOG_NOTICE, "Debug mode enabled.\n");
  
  if (sb_globals.validate)
    log_text(LOG_NOTICE, "Additional request validation enabled.\n");

  if (rand_init)
  {
    log_text(LOG_NOTICE, "Initializing random number generator from timer.\n");
    sb_srnd(time(NULL));
  }   
  
  log_text(LOG_NOTICE, "");

  if (test->ops.print_mode != NULL)
    test->ops.print_mode();
}


/* Main runner test thread */


void *runner_thread(void *arg)
{
  sb_request_t     request;
  sb_thread_ctxt_t *ctxt;
  sb_test_t        *test;
  unsigned int     thread_id;
  
  ctxt = (sb_thread_ctxt_t *)arg;
  test = ctxt->test;
  thread_id = ctxt->id;
  
  log_text(LOG_DEBUG, "Runner thread started (%d)!", thread_id);
  if (test->ops.thread_init != NULL && test->ops.thread_init(thread_id) != 0)
  {
    sb_globals.error = 1;
    return NULL; /* thread initialization failed  */
  }
  
  /* 
    We do this to make sure all threads get to this barrier 
    about the same time 
  */
  pthread_mutex_lock(&thread_start_mutex);
  pthread_mutex_unlock(&thread_start_mutex);
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
        NS2SEC(sb_timer_current(&sb_globals.exec_timer))
        >= sb_globals.max_time)
    {
      log_text(LOG_INFO, "Time limit exceeded, exiting...");
      break;
    }
  } while ((request.type != SB_REQ_TYPE_NULL) && (!sb_globals.error) );

  if (test->ops.thread_done != NULL)
    test->ops.thread_done(thread_id);
  
  return NULL; 
}


/* 
  Main test function. Start threads. 
  Wait for them to complete and measure time 
*/


int run_test(sb_test_t *test)
{
  unsigned int i;
  int err;
  
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
  sb_timer_start(&sb_globals.exec_timer);

  /* prepare test */
  if (test->ops.prepare != NULL && test->ops.prepare() != 0)
    return 1;

  pthread_mutex_init(&sb_globals.exec_mutex, NULL);

  /* start mutex used for barrier */
  pthread_mutex_init(&thread_start_mutex,NULL);    
  pthread_mutex_lock(&thread_start_mutex);    
  
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

  /* Starting the test threads */
  for(i = 0; i < sb_globals.num_threads; i++)
  {
    if (sb_globals.error)
      return 1;
    if ((err = pthread_create(&(threads[i].thread), &thread_attr, &runner_thread, 
                              (void*)&(threads[i]))) != 0)
    {
      log_text(LOG_FATAL, "Thread #%d creation failed, errno = %d (%s)",
               i, err, strerror(err));
      return 1;
    }
  }
  pthread_mutex_unlock(&thread_start_mutex);
  
  log_text(LOG_NOTICE, "Threads started!\n");  
  for(i = 0; i < sb_globals.num_threads; i++)
  {
    if((err = pthread_join(threads[i].thread, NULL)) != 0)
    {
      log_text(LOG_FATAL, "Thread #%d join failed, errno = %d (%s)",
               i, err, strerror(err));
      return 1;    
    }
  }
  log_text(LOG_INFO, "Done.\n");

  /* cleanup test */
  if (test->ops.cleanup != NULL && test->ops.cleanup() != 0)
    return 1;
  
  sb_timer_stop(&sb_globals.exec_timer);

  /* print test-specific stats */
  if (test->ops.print_stats != NULL && !sb_globals.error)
    test->ops.print_stats();

  pthread_mutex_destroy(&rnd_mutex);

  pthread_mutex_destroy(&sb_globals.exec_mutex);

  pthread_mutex_destroy(&thread_start_mutex);

  /* finalize test */
  if (test->ops.done != NULL)
    (*(test->ops.done))();

  return sb_globals.error != 0;
}


sb_test_t *find_test(char *name)
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


int init(void)
{
  option_t *opt;
  char     *s;
  
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
