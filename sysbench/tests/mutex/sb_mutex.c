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

typedef struct
{
  pthread_mutex_t mutex;
  char            pad[256];
} thread_lock;


/* Mutex test arguments */
static sb_arg_t mutex_args[] =
{
  {"mutex-num", "total size of mutex array", SB_ARG_TYPE_INT, "4096"},
  {"mutex-locks", "number of mutex locks to do per thread",
   SB_ARG_TYPE_INT, "50000"},
  {"mutex-loops", "number of empty loops to do inside mutex lock",
   SB_ARG_TYPE_INT, "10000"},
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
}; 

/* Mutex test operations */
static int mutex_init(void);
static void mutex_print_mode(void);
static sb_request_t mutex_get_request(int tid);
static int mutex_execute_request(sb_request_t *, int);
static int mutex_done(void);

static sb_test_t mutex_test =
{
  "mutex",
  "Mutex performance test",
  {
    mutex_init,
    NULL,
    NULL,
    mutex_print_mode,
    mutex_get_request,
    mutex_execute_request,
    NULL,
    NULL,
    NULL,
    mutex_done
  },
  {
    NULL,
    NULL,
    NULL,
    NULL
  },
  mutex_args,
  {NULL, NULL}
};


static thread_lock *thread_locks;
static unsigned int mutex_num;
static unsigned int mutex_loops;
static unsigned int mutex_locks;
static unsigned int global_var;

int register_test_mutex(sb_list_t *tests)
{
  SB_LIST_ADD_TAIL(&mutex_test.listitem, tests);

  return 0;
}


int mutex_init(void)
{
  unsigned int i;
  
  mutex_num = sb_get_value_int("mutex-num");
  mutex_loops = sb_get_value_int("mutex-loops");
  mutex_locks = sb_get_value_int("mutex-locks");

  thread_locks = (thread_lock *)malloc(mutex_num * sizeof(thread_lock));
  if (thread_locks == NULL)
  {
    log_text(LOG_FATAL, "Memory allocation failure!");
    return 1;
  }

  for (i = 0; i < mutex_num; i++)
    pthread_mutex_init(&thread_locks[i].mutex, NULL);
  
  return 0;
}


int mutex_done(void)
{
  unsigned int i;

  for(i=0; i < mutex_num; i++)
    pthread_mutex_destroy(&thread_locks[i].mutex);
  free(thread_locks);
  
  return 0;
}


sb_request_t mutex_get_request(int tid)
{
  sb_request_t         sb_req;
  sb_mutex_request_t   *mutex_req = &sb_req.u.mutex_request;

  (void)tid; /* unused */
  
  sb_req.type = SB_REQ_TYPE_MUTEX;
  mutex_req->nlocks = mutex_locks;
  mutex_req->nloops = mutex_loops;

  return sb_req;
}


int mutex_execute_request(sb_request_t *sb_req, int thread_id)
{
  unsigned int         i;
  unsigned int         c=0;
  unsigned int         current_lock;
  sb_mutex_request_t   *mutex_req = &sb_req->u.mutex_request;
  log_msg_t            msg;
  log_msg_oper_t       op_msg;

  /* Prepare log message */
  msg.type = LOG_MSG_TYPE_OPER;
  msg.data = &op_msg;

  LOG_EVENT_START(msg, thread_id);
  do
  {
    current_lock = rand() % mutex_num;
    for (i = 0; i < mutex_req->nloops; i++)
      c++;
    pthread_mutex_lock(&thread_locks[current_lock].mutex);
    global_var++;
    pthread_mutex_unlock(&thread_locks[current_lock].mutex);
    mutex_req->nlocks--;
  }
  while (mutex_req->nlocks > 0);
  LOG_EVENT_STOP(msg, thread_id);

  /* Perform only one request per thread */
  return 1;
}


void mutex_print_mode(void)
{
  log_text(LOG_INFO, "Doing mutex performance test");
}

