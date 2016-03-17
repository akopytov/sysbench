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

/* Thread barrier implementation. */

#ifndef SB_BARRIER_H
#define SB_BARRIER_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#ifdef _WIN32
#include "sb_win.h"
#endif

#define SB_BARRIER_SERIAL_THREAD 1

typedef int (*sb_barrier_cb_t)(void *);

typedef struct {
  unsigned int     count;
  unsigned int     init_count;
  unsigned int     serial;
  pthread_mutex_t  mutex;
  pthread_cond_t   cond;
  sb_barrier_cb_t  callback;
  void             *arg;
  int              error;
} sb_barrier_t;

int sb_barrier_init(sb_barrier_t *barrier, unsigned int count,
                    sb_barrier_cb_t callback, void *arg);

int sb_barrier_wait(sb_barrier_t *barrier);

void sb_barrier_destroy(sb_barrier_t *barrier);

#endif /* SB_BARRIER_H */
