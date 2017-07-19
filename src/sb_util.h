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

#ifndef SB_UTIL_H
#define SB_UTIL_H

/*
  General utility macros and functions.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ck_md.h"
#include "ck_cc.h"

#ifdef HAVE_FUNC_ATTRIBUTE_FORMAT
# define SB_ATTRIBUTE_FORMAT(style, m, n) __attribute__((format(style, m, n)))
#else
# define SB_ATTRIBUTE_FORMAT(style, m, n)
#endif

#ifdef HAVE_FUNC_ATTRIBUTE_UNUSED
# define SB_ATTRIBUTE_UNUSED __attribute__((unused))
#else
# define SB_ATTRIBUTE_UNUSED
#endif

#if defined(__MACH__)
# define DLEXT ".dylib"
#else
# define DLEXT ".so"
#endif

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
#define SB_CACHELINE_PAD(n) (SB_PAD((n), CK_MD_CACHELINE))

/* Minimum/maximum values */
#ifdef __GNUC__
#  define SB_MIN(a,b)           \
  ({ __typeof__ (a) _a = (a);   \
    __typeof__ (b) _b = (b);    \
    _a < _b ? _a : _b; })
#  define SB_MAX(a,b)           \
  ({ __typeof__ (a) _a = (a);   \
    __typeof__ (b) _b = (b);    \
    _a > _b ? _a : _b; })
#else
#  define SB_MIN(a,b) (((a) < (b)) ? (a) : (b))
#  define SB_MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif /* __GNUC__ */

#define SB_LIKELY(x) CK_CC_LIKELY(x)
#define SB_UNLIKELY(x) CK_CC_UNLIKELY(x)

/* SB_CONTAINER_OF */
#ifdef __GNUC__
#  define SB_MEMBER_TYPE(type, member) __typeof__ (((type *)0)->member)
#else
#  define SB_MEMBER_TYPE(type, member) const void
#endif /* __GNUC__ */

#define SB_CONTAINER_OF(ptr, type, member) ((type *)(void *)(           \
    (char *)(SB_MEMBER_TYPE(type, member) *){ ptr } - offsetof(type, member)))

/* Compile-time assertion */
#define SB_COMPILE_TIME_ASSERT(expr)                                    \
  do {                                                                  \
    typedef char cta[(expr) ? 1 : -1] SB_ATTRIBUTE_UNUSED;              \
  } while(0)

#ifdef HAVE_ISATTY
# define SB_ISATTY() isatty(0)
#else
# error No isatty() implementation for this platform!
#endif

/*
  Allocate a buffer of a specified size such that the address is a multiple of a
  specified alignment.
*/
void *sb_memalign(size_t size, size_t alignment);

/* Get OS page size */
size_t sb_getpagesize(void);

#endif /* SB_UTIL_H */
