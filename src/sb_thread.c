/*
   Copyright (C) 2016 Alexey Kopytov <akopytov@gmail.com>

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

/*
   Wrappers around pthread_create() and friends to provide necessary
   (de-)initialization.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef _WIN32
#include "sb_win.h"
#endif

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#include "sb_thread.h"
#include "sb_rand.h"
#include "sb_logger.h"
#include "sysbench.h"
#include "sb_ck_pr.h"

pthread_attr_t  sb_thread_attr;

/* Thread descriptors */
static sb_thread_ctxt_t *threads;

/* Stack size for each thread */
static int thread_stack_size;

int sb_thread_init(void)
{
  thread_stack_size = sb_get_value_size("thread-stack-size");
  if (thread_stack_size <= 0)
  {
    log_text(LOG_FATAL, "Invalid value for thread-stack-size: %d.\n", thread_stack_size);
    return 1;
  }

  /* initialize attr */
  pthread_attr_init(&sb_thread_attr);
#ifdef PTHREAD_SCOPE_SYSTEM
  pthread_attr_setscope(&sb_thread_attr,PTHREAD_SCOPE_SYSTEM);
#endif
  pthread_attr_setstacksize(&sb_thread_attr, thread_stack_size);

#ifdef HAVE_THR_SETCONCURRENCY
  /* Set thread concurrency (required on Solaris) */
  thr_setconcurrency(sb_globals.threads);
#endif

  threads = malloc(sb_globals.threads * sizeof(sb_thread_ctxt_t));
  if (threads == NULL)
  {
    log_text(LOG_FATAL, "Memory allocation failure.\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void sb_thread_done(void)
{
  if (threads != NULL)
    free(threads);
}

int sb_thread_create(pthread_t *thread, const pthread_attr_t *attr,
                     void *(*start_routine) (void *), void *arg)
{
  return pthread_create(thread, attr, start_routine, arg);
}

int sb_thread_join(pthread_t thread, void **retval)
{
  return pthread_join(thread, retval);
}

int sb_thread_cancel(pthread_t thread)
{
  return pthread_cancel(thread);
}

int sb_thread_create_workers(void *(*worker_routine)(void*))
{
  unsigned int i;

  log_text(LOG_NOTICE, "Initializing worker threads...\n");

  for(i = 0; i < sb_globals.threads; i++)
  {
    threads[i].id = i;
  }


  for(i = 0; i < sb_globals.threads; i++)
  {
    int err;

    if ((err = sb_thread_create(&(threads[i].thread), &sb_thread_attr,
                                worker_routine, (void*)(threads + i))) != 0)
    {
      log_errno(LOG_FATAL, "sb_thread_create() for thread #%d failed.", i);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}


int sb_thread_join_workers(void)
{
  for(unsigned i = 0; i < sb_globals.threads; i++)
  {
    int err;

    if((err = sb_thread_join(threads[i].thread, NULL)) != 0)
      log_errno(LOG_FATAL, "sb_thread_join() for thread #%d failed.", i);

    ck_pr_dec_uint(&sb_globals.threads_running);
  }

  return EXIT_SUCCESS;
}
