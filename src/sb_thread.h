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

#ifndef SB_THREAD_H
#define SB_THREAD_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef _WIN32
#include "sb_win.h"
#endif

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

/* Thread context definition */

typedef struct
{
  pthread_t     thread;
  unsigned int  id;
} sb_thread_ctxt_t;

extern pthread_attr_t sb_thread_attr;

int sb_thread_create(pthread_t *thread, const pthread_attr_t *attr,
                     void *(*start_routine) (void *), void *arg);

int sb_thread_join(pthread_t thread, void **retval);

int sb_thread_cancel(pthread_t thread);

int sb_thread_create_workers(void *(*worker_routine)(void*));

int sb_thread_join_workers(void);

int sb_thread_init(void);

void sb_thread_done(void);

#endif /* SB_THREAD_H */
