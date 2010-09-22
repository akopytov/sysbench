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

#ifndef SYSBENCH_H
#define SYSBENCH_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
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
#include "tests/sb_oltp.h"

/* Macros to control global execution mutex */
#define SB_THREAD_MUTEX_LOCK() pthread_mutex_lock(&sb_globals.exec_mutex) 
#define SB_THREAD_MUTEX_UNLOCK() pthread_mutex_unlock(&sb_globals.exec_mutex)

#define SB_MAX_RND 0x3fffffffu

/* random() is not thread-safe on most platforms, use lrand48() if available */
#ifdef HAVE_LRAND48
#define sb_rnd() (lrand48() % SB_MAX_RND)
#define sb_srnd(seed) srand48(seed)
#else
#define sb_rnd() (random() % SB_MAX_RND)
#define sb_srnd(seed) srandom((unsigned int)seed)
#endif

/* Sysbench commands */
typedef enum
{
  SB_COMMAND_NULL,
  SB_COMMAND_PREPARE,
  SB_COMMAND_RUN,
  SB_COMMAND_CLEANUP,
  SB_COMMAND_HELP,
  SB_COMMAND_VERSION
} sb_cmd_t;

/* Request types definition */

typedef enum
{
  SB_REQ_TYPE_NULL,
  SB_REQ_TYPE_CPU,
  SB_REQ_TYPE_MEMORY,
  SB_REQ_TYPE_FILE,
  SB_REQ_TYPE_SQL,
  SB_REQ_TYPE_THREADS,
  SB_REQ_TYPE_MUTEX
} sb_request_type_t;

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
    sb_mem_request_t     mem_request;
    sb_threads_request_t threads_request;
    sb_mutex_request_t   mutex_request;
    sb_sql_request_t     sql_request;
  } u;
} sb_request_t;

/* Test commands definition */

typedef int sb_cmd_help(void);
typedef int sb_cmd_prepare(void);
typedef int sb_cmd_run(void);
typedef int sb_cmd_cleanup(void);


/* Test operations definition */

typedef int sb_op_init(void);
typedef int sb_op_prepare(void);
typedef int sb_op_thread_init(int);
typedef void sb_op_print_mode(void);
typedef sb_request_t sb_op_get_request(int);
typedef int sb_op_execute_request(sb_request_t *, int);
typedef void sb_op_print_stats(int);
typedef int sb_op_thread_done(int);
typedef int sb_op_cleanup(void);
typedef int sb_op_done(void);

/* Test commands structure definitions */

typedef struct
{
  sb_cmd_help     *help;     /* print help */
  sb_cmd_prepare  *prepare;  /* prepare for the test */
  sb_cmd_run      *run;      /* run the test */
  sb_cmd_cleanup  *cleanup;  /* cleanup the test database, files, etc. */
} sb_commands_t;

/* Test operations structure definition */

typedef struct
{
  sb_op_init            *init;            /* initialization function */
  sb_op_prepare         *prepare;         /* called after timers start,  but
                                             before thread execution */
  sb_op_thread_init     *thread_init;     /* thread initialization
                                             (called when each thread starts) */
  sb_op_print_mode      *print_mode;      /* print mode function */
  sb_op_get_request     *get_request;     /* request generation function */
  sb_op_execute_request *execute_request; /* request execution function */
  sb_op_print_stats     *print_stats;     /* stats generation function */
  sb_op_thread_done     *thread_done;     /* thread finalize function */
  sb_op_cleanup         *cleanup;         /* called after exit from thread,
                                             but before timers stop */ 
  sb_op_done            *done;            /* finalize function */
} sb_operations_t;

/* Test structure definition */

typedef struct sb_test
{
  char             *sname;
  char             *lname;
  sb_operations_t  ops;
  sb_commands_t    cmds;
  sb_arg_t         *args;

  sb_list_item_t   listitem;
} sb_test_t;

/* Thread context definition */

typedef struct
{
  sb_test_t     *test;
  pthread_t     thread;
  unsigned int  id;
} sb_thread_ctxt_t;


/* sysbench global variables */

typedef struct
{
  sb_cmd_t         command;        /* command passed from command line */
  int              error;          /* global error - everyone exit */
  pthread_mutex_t  exec_mutex;     /* execution mutex */
  sb_timer_t       *op_timers;     /* timers to measure each thread's run time */
  sb_timer_t       exec_timer;     /* total execution timer */
  unsigned int     num_threads;    /* number of threads to use */
  unsigned int     num_running;    /* number of threads currently active */
  unsigned int     intermediate_result_timer; /* Intermediate result timer */
  int              stopped;        /* Has test been stopped yet */
  unsigned int     tx_rate;        /* target transaction rate */
  unsigned int     tx_jitter;      /* target transaction variation (us) */
  unsigned int     max_requests;   /* maximum number of requests */
  unsigned int     max_time;       /* total execution time limit */
  int              force_shutdown; /* whether we must force test shutdown */
  unsigned int     timeout;        /* forced shutdown timeout */
  unsigned char    debug;          /* debug flag */
  unsigned char    validate;       /* validation flag */
} sb_globals_t;

extern sb_globals_t sb_globals;

/* used to get options passed from command line */

int sb_get_value_flag(char *);
int sb_get_value_int(char *);
unsigned long long sb_get_value_size(char *);
float sb_get_value_float(char *);
char *sb_get_value_string(char *);

#endif
