/*
   Copyright (C) 2017 Alexey Kopytov <akopytov@gmail.com>

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
  General utility macros and functions.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ck_md.h"

/*
  Calculate the smallest multiple of m that is not smaller than n, when m is a
  power of 2.
*/
#define SB_ALIGN(n, m) (((n) + ((m) - 1)) & ~((m) - 1))

/*
  Calculate padding, i.e. distance from n to SB_ALIGN(n, m), where m is a power
  of 2.
*/
#define SB_PAD(n, m) (SB_ALIGN((n),(m)) - (n))

/* Calculate padding to cache line size. */
#define SB_CACHELINE_PAD(n) (SB_PAD(n, CK_MD_CACHELINE))
