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
# include "sb_win.h"
#endif

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#include "sysbench.h"

/* How to test scheduler pthread_yield or sched_yield */
#ifdef HAVE_PTHREAD_YIELD
#define YIELD pthread_yield 
#elif defined (_WIN32)
#define YIELD SwitchToThread
#else
#define YIELD sched_yield
#endif

/* Threads test arguments */
static sb_arg_t threads_args[] =
{
  {
    "thread-yields", "number of yields to do per request", SB_ARG_TYPE_INT,
  "1000"
  },
  {"thread-locks", "number of locks per thread", SB_ARG_TYPE_INT, "8"},
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
}; 

/* Threads test operations */
static int threads_init(void);
static int threads_prepare(void);
static void threads_print_mode(void);
static sb_request_t threads_get_request(int tid);
static int threads_execute_request(sb_request_t *, int);
static int threads_cleanup(void);

static sb_test_t threads_test =
{
  "threads",
  "Threads subsystem performance test",
  {
    threads_init,
    threads_prepare,
    NULL,
    threads_print_mode,
    threads_get_request,
    threads_execute_request,
    NULL,
    NULL,
    threads_cleanup,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL
  },
  threads_args,
  {NULL, NULL}
};

static unsigned int thread_yields;
static unsigned int thread_locks;
static pthread_mutex_t *test_mutexes;
static unsigned int req_performed;


int register_test_threads(sb_list_t *tests)
{
  SB_LIST_ADD_TAIL(&threads_test.listitem, tests);

  return 0;
}


int threads_init(void)
{
  thread_yields = sb_get_value_int("thread-yields");
  thread_locks = sb_get_value_int("thread-locks");
  req_performed = 0;
  
  return 0;
}


int threads_prepare(void)
{
  unsigned int i;

  test_mutexes = (pthread_mutex_t *)malloc(thread_locks *
                                           sizeof(pthread_mutex_t));
  if (test_mutexes == NULL)
  {
    log_text(LOG_FATAL, "Memory allocation failure!");
    return 1;
  }
  
  for(i = 0; i < thread_locks; i++)
    pthread_mutex_init(test_mutexes + i, NULL);

  return 0;
}


int threads_cleanup(void)
{
  unsigned int i;

  for(i=0; i < thread_locks; i++)
    pthread_mutex_destroy(test_mutexes + i);
  free(test_mutexes);
  
  return 0;
}


sb_request_t threads_get_request(int tid)
{
  sb_request_t         sb_req;
  sb_threads_request_t *threads_req = &sb_req.u.threads_request;

  (void)tid; /* unused */
  
  SB_THREAD_MUTEX_LOCK();
  if (req_performed >= sb_globals.max_requests)
  {
    sb_req.type = SB_REQ_TYPE_NULL;
    SB_THREAD_MUTEX_UNLOCK();
    return sb_req;
  }
  
  sb_req.type = SB_REQ_TYPE_THREADS;
  threads_req->lock_num = req_performed % thread_locks; 
  req_performed++;
  SB_THREAD_MUTEX_UNLOCK();

  return sb_req;
}


int threads_execute_request(sb_request_t *sb_req, int thread_id)
{
  unsigned int         i;
  sb_threads_request_t *threads_req = &sb_req->u.threads_request;
  log_msg_t           msg;
  log_msg_oper_t      op_msg;
  
  /* Prepare log message */
  msg.type = LOG_MSG_TYPE_OPER;
  msg.data = &op_msg;

  LOG_EVENT_START(msg, thread_id);
  for(i = 0; i < thread_yields; i++)
  {
    pthread_mutex_lock(&test_mutexes[threads_req->lock_num]);
    YIELD();
    pthread_mutex_unlock(&test_mutexes[threads_req->lock_num]);
  }
  LOG_EVENT_STOP(msg, thread_id);

  return 0;
}


void threads_print_mode(void)
{
  log_text(LOG_INFO, "Doing thread subsystem performance test");
  log_text(LOG_INFO, "Thread yields per test: %d Locks used: %d",
         thread_yields, thread_locks);
}

