/*
   Copyright (C) 2016-2017 Alexey Kopytov <akopytov@gmail.com>

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

#ifndef SB_RAND_H
#define SB_RAND_H

#include <stdlib.h>

#include "xoroshiro128plus.h"

/* Random numbers distributions */
typedef enum
{
  DIST_TYPE_UNIFORM,
  DIST_TYPE_GAUSSIAN,
  DIST_TYPE_SPECIAL,
  DIST_TYPE_PARETO,
  DIST_TYPE_ZIPFIAN
} rand_dist_t;

typedef uint64_t sb_rng_state_t [2];

/* optional seed set on the command line */
extern int sb_rand_seed;

/* Thread-local RNG state */
extern TLS sb_rng_state_t sb_rng_state;

/* Return a uniformly distributed pseudo-random 64-bit unsigned integer */
inline uint64_t sb_rand_uniform_uint64(void)
{
  return xoroshiro_next(sb_rng_state);
}

/* Return a uniformly distributed pseudo-random double in the [0, 1) interval */
inline double sb_rand_uniform_double(void)
{
  const uint64_t x = sb_rand_uniform_uint64();
  const union { uint64_t i; double d; } u = { .i = UINT64_C(0x3FF) << 52 | x >> 12 };

  return u.d - 1.0;
}

int sb_rand_register(void);
void sb_rand_print_help(void);
int sb_rand_init(void);
void sb_rand_done(void);
void sb_rand_thread_init(void);

/* Generator functions */
uint32_t sb_rand_default(uint32_t, uint32_t);
uint32_t sb_rand_uniform(uint32_t, uint32_t);
uint32_t sb_rand_gaussian(uint32_t, uint32_t);
uint32_t sb_rand_special(uint32_t, uint32_t);
uint32_t sb_rand_pareto(uint32_t, uint32_t);
uint32_t sb_rand_zipfian(uint32_t, uint32_t);
uint32_t sb_rand_unique(void);
void sb_rand_str(const char *, char *);

#endif /* SB_RAND_H */
