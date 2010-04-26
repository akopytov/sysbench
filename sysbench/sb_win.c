/* Copyright (C) 2008 MySQL AB

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

/*
 This file contains Windows port of Posix functionality used in sysbench
 (partial implementation of pthreads, gettimeofday() and random()
*/

#define _CRT_RAND_S /* for rand_s */
#include <stdlib.h>
#include <windows.h>
#include <errno.h>
#include "sb_win.h"

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
  cond->waiting= 0;
  InitializeCriticalSection(&cond->lock_waiting);

  cond->events[SIGNAL]= CreateEvent(NULL, FALSE, FALSE, NULL);
  cond->events[BROADCAST]= CreateEvent(NULL, TRUE, FALSE, NULL);
  cond->broadcast_block_event= CreateEvent(NULL, TRUE, TRUE, NULL);

  if( cond->events[SIGNAL] == NULL ||
    cond->events[BROADCAST] == NULL ||
    cond->broadcast_block_event == NULL )
    return ENOMEM;
  return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
  DeleteCriticalSection(&cond->lock_waiting);

  if (CloseHandle(cond->events[SIGNAL]) == 0 ||
    CloseHandle(cond->events[BROADCAST]) == 0 ||
    CloseHandle(cond->broadcast_block_event) == 0)
    return EINVAL;
  return 0;
}


int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  return pthread_cond_timedwait(cond,mutex,NULL);
}


int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
struct timespec *abstime)
{
  int result;
  long timeout; 
  union {
    FILETIME ft;
    long long i64;
  }now;

  if( abstime != NULL )
  {
    long long stoptime_nanos = (abstime->tv_sec*1000000000 + abstime->tv_nsec);
    long long now_nanos;
    long long timeout_nanos;

    GetSystemTimeAsFileTime(&now.ft);
    now_nanos = now.i64 *100;
    timeout_nanos = stoptime_nanos - now_nanos;
    timeout = (long)(timeout_nanos /1000000);
  }
  else
  {
    /* No time specified; don't expire */
    timeout= INFINITE;
  }

  /* 
  Block access if previous broadcast hasn't finished.
  This is just for safety and should normally not
  affect the total time spent in this function.
  */
  WaitForSingleObject(cond->broadcast_block_event, INFINITE);

  EnterCriticalSection(&cond->lock_waiting);
  cond->waiting++;
  LeaveCriticalSection(&cond->lock_waiting);

  LeaveCriticalSection(mutex);

  result= WaitForMultipleObjects(2, cond->events, FALSE, timeout);

  EnterCriticalSection(&cond->lock_waiting);
  cond->waiting--;

  if (cond->waiting == 0 && result == (WAIT_OBJECT_0+BROADCAST))
  {
    /*
      We're the last waiter to be notified or to stop waiting, so
      reset the manual event.
    */
    /* Close broadcast gate */
    ResetEvent(cond->events[BROADCAST]);
    /* Open block gate */
    SetEvent(cond->broadcast_block_event);
  }
  LeaveCriticalSection(&cond->lock_waiting);

  EnterCriticalSection(mutex);

  return result == WAIT_TIMEOUT ? ETIMEDOUT : 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
  EnterCriticalSection(&cond->lock_waiting);

  if(cond->waiting > 0)
    SetEvent(cond->events[SIGNAL]);

  LeaveCriticalSection(&cond->lock_waiting);

  return 0;
}


int pthread_cond_broadcast(pthread_cond_t *cond)
{
  EnterCriticalSection(&cond->lock_waiting);
  /*
    The mutex protect us from broadcasting if
    there isn't any thread waiting to open the
    block gate after this call has closed it.
  */
  if(cond->waiting > 0)
  {
    /* Close block gate */
    ResetEvent(cond->broadcast_block_event); 
    /* Open broadcast gate */
    SetEvent(cond->events[BROADCAST]);
  }

  LeaveCriticalSection(&cond->lock_waiting);  

  return 0;
}

int pthread_attr_init(pthread_attr_t *attr)
{
  attr->stacksize = 0;
  return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
  InitializeCriticalSection(mutex);
  return 0;
}
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  EnterCriticalSection(mutex);
  return 0;
}
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  LeaveCriticalSection(mutex);
  return 0;
}
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  DeleteCriticalSection(mutex);
  return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void*), void *arg)
{
  DWORD tid;
  *thread = CreateThread(NULL, attr->stacksize,
                         (LPTHREAD_START_ROUTINE) start_routine, arg,
                         STACK_SIZE_PARAM_IS_A_RESERVATION, &tid);
  if (*thread != NULL)
    return 0;
  return -1;
}

/* Minimal size of thread stack on Windows*/
#define PTHREAD_STACK_MIN 65536*2

int pthread_attr_setstacksize( pthread_attr_t *attr, size_t stacksize)
{
  if(stacksize)
    attr->stacksize = max(stacksize, PTHREAD_STACK_MIN);
  return 0;
}

int pthread_join(pthread_t pthread, void **value_ptr)
{
  if (WaitForSingleObject(pthread, INFINITE) != WAIT_OBJECT_0)
    return -1;
  if (value_ptr)
    *value_ptr = 0;
  return 0;

}

/*
 One time initialization. For simplicity, we assume initializer thread
 does not exit within init_routine().
*/
int pthread_once(pthread_once_t *once_control, 
    void (*init_routine)(void))
{
  LONG state = InterlockedCompareExchange(once_control, PTHREAD_ONCE_INPROGRESS,
                                          PTHREAD_ONCE_INIT);
  switch(state)
  {
  case PTHREAD_ONCE_INIT:
    /* This is initializer thread */
    (*init_routine)();
    *once_control = PTHREAD_ONCE_DONE;
    break;

  case PTHREAD_ONCE_INPROGRESS:
    /* init_routine in progress. Wait for its completion */
    while(*once_control == PTHREAD_ONCE_INPROGRESS)
    {
      Sleep(1);
    }
    break;
  case PTHREAD_ONCE_DONE:
    /* Nothing to do */
    break;
  }
  return 0;
}

#include <time.h>
int gettimeofday(struct timeval * tp, void * tzp)
{
  static long long qpf = 0, startup_time = 0;
  long long qpc;
  if(qpf == 0)
  {
    QueryPerformanceFrequency((LARGE_INTEGER *)&qpf);
  }
  if(startup_time == 0)
  {
    QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
    startup_time = time(NULL) - (qpc/qpf);
  }
  QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
  tp->tv_sec  = (long)(startup_time + qpc/qpf);
  tp->tv_usec = (long)((qpc%qpf)*1000000/qpf);
  return 0;
}

int random()
{
  int ret;
  rand_s(&ret);
  return ret;
}
