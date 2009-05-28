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

#ifdef HAVE_MATH_H
# include <math.h>
#endif

#include "sysbench.h"

/* CPU test arguments */
static sb_arg_t cpu_args[] =
{
  {"cpu-max-prime", "upper limit for primes generator", SB_ARG_TYPE_INT, "10000"},
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

/* CPU test operations */
static int cpu_init(void);
static void cpu_print_mode(void);
static sb_request_t cpu_get_request(int);
static int cpu_execute_request(sb_request_t *, int);
static void cpu_print_stats(void);
static int cpu_done(void);

static sb_test_t cpu_test =
{
  "cpu",
  "CPU performance test",
  {
    cpu_init,
    NULL,
    NULL,
    cpu_print_mode,
    cpu_get_request,
    cpu_execute_request,
    cpu_print_stats,
    NULL,
    NULL,
    cpu_done
  },
  {
    NULL,
    NULL,
    NULL,
    NULL
  },
  cpu_args,
  {NULL, NULL}
};

/* Upper limit for primes */
static unsigned int    max_prime;
/* Request counter */
static unsigned int    req_performed;
/* Counter mutex */
static pthread_mutex_t request_mutex;

int register_test_cpu(sb_list_t * tests)
{
  SB_LIST_ADD_TAIL(&cpu_test.listitem, tests);

  return 0;
}

int cpu_init(void)
{
  max_prime = sb_get_value_int("cpu-max-prime");
  if (max_prime <= 0)
  {
    log_text(LOG_FATAL, "Invalid value of cpu-max-prime: %d.", max_prime);
    return 1;
  }

  req_performed = 0;

  pthread_mutex_init(&request_mutex, NULL);
  
  return 0;
}


sb_request_t cpu_get_request(int tid)
{
  sb_request_t req;

  (void)tid; /* unused */
  
  if (req_performed >= sb_globals.max_requests)
  {
    req.type = SB_REQ_TYPE_NULL;
    return req;
  }
  req.type = SB_REQ_TYPE_CPU;
  pthread_mutex_lock(&request_mutex);
  req_performed++;
  pthread_mutex_unlock(&request_mutex);

  return req;
}

int cpu_execute_request(sb_request_t *r, int thread_id)
{
  unsigned long long c;
  unsigned long long l,t;
  unsigned long long n=0;
  log_msg_t           msg;
  log_msg_oper_t      op_msg;
  
  (void)r; /* unused */

  /* Prepare log message */
  msg.type = LOG_MSG_TYPE_OPER;
  msg.data = &op_msg;
  
  /* So far we're using very simple test prime number tests in 64bit */
  LOG_EVENT_START(msg, thread_id);

  for(c=3; c < max_prime; c++)  
  {
    t = sqrt(c);
    for(l = 2; l <= t; l++)
      if (c % l == 0)
        break;
    if (l > t )
      n++; 
  }

  LOG_EVENT_STOP(msg, thread_id);

  return 0;
}

void cpu_print_mode(void)
{
  log_text(LOG_INFO, "Doing CPU performance benchmark\n");  
}

void cpu_print_stats(void)
{
  log_text(LOG_NOTICE, "Maximum prime number checked in CPU test: %d\n",
           max_prime);
}

int cpu_done(void)
{
  pthread_mutex_destroy(&request_mutex);

  return 0;
}
