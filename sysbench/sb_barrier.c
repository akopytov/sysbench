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
   Thread barrier implementation. It differs from pthread_barrier_t in two ways:

   - it's more portable (will also work on OS X and Windows with existing
     pthread_* wrappers in sb_win.c).

   - it allows defining a callback function which is called right before
     signaling the participating threads to continue, i.e. as soon as the
     required number of threads reach the barrier. The callback can also signal
     an error to sb_barrier_wait() callers by returning a non-zero value. In
     which case sb_barrier_wait() returns a negative value to all callers.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sb_barrier.h"

int sb_barrier_init(sb_barrier_t *barrier, unsigned int count,
                    sb_barrier_cb_t callback, void *arg)
{
  if (count == 0)
    return 1;

  if (pthread_mutex_init(&barrier->mutex, NULL) ||
      pthread_cond_init(&barrier->cond, NULL))
    return 1;

  barrier->init_count = count;
  barrier->count = count;
  barrier->callback = callback;
  barrier->arg = arg;
  barrier->serial = 0;
  barrier->error = 0;

  return 0;
}


int sb_barrier_wait(sb_barrier_t *barrier)
{
  int res;

  pthread_mutex_lock(&barrier->mutex);

  if (!--barrier->count)
  {
    barrier->serial++;
    barrier->count = barrier->init_count;

    res = SB_BARRIER_SERIAL_THREAD;

    pthread_cond_broadcast(&barrier->cond);

    if (barrier->callback != NULL && barrier->callback(barrier->arg) != 0)
    {
      barrier->error = 1;
      res = -1;
    }

    pthread_mutex_unlock(&barrier->mutex);

  }
  else
  {
    unsigned int serial = barrier->serial;

    do {
      pthread_cond_wait(&barrier->cond, &barrier->mutex);
    } while (serial == barrier->serial);

    res = barrier->error ? -1 : 0;

    pthread_mutex_unlock(&barrier->mutex);
  }

  return res;
}


void sb_barrier_destroy(sb_barrier_t *barrier)
{
  pthread_mutex_destroy(&barrier->mutex);
  pthread_cond_destroy(&barrier->cond);
}
