/*
   Copyright (C) 2017-2018 Alexey Kopytov <akopytov@gmail.com>

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <inttypes.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "sb_util.h"
#include "sb_logger.h"

/*
  Allocate a buffer of a specified size such that the address is a multiple of a
  specified alignment.
*/

void *sb_memalign(size_t size, size_t alignment)
{
  void *buf;
#if defined(_WIN32)
  buf = _aligned_malloc(size, alignment);
#elif defined(HAVE_POSIX_MEMALIGN)
  int ret= posix_memalign(&buf, alignment, size);
  if (ret != 0)
    buf = NULL;
#elif defined(HAVE_MEMALIGN)
  buf = memalign(alignment, size);
#elif defined(HAVE_VALLOC)
  /* Allocate on page boundary */
  (void) alignment; /* unused */
  buf = valloc(size);
#else
# error Cannot find an aligned allocation library function!
#endif
  return buf;
}
/* Free memory allocated with sb_memalign() */
void sb_free_memaligned(void* p)
{
#if defined(WIN32)
  _aligned_free(p);
#else
  free(p);
#endif
}

/* Get OS page size */

size_t sb_getpagesize(void)
{
#ifdef _WIN32
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwPageSize;
#elif defined _SC_PAGESIZE
  return sysconf(_SC_PAGESIZE);
#else
  return getpagesize();
#endif
}
