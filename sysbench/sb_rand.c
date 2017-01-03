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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_MATH_H
# include <math.h>
#endif

#include "sb_options.h"
#include "sb_rand.h"
#include "sb_logger.h"

/* Large prime number to generate unique random IDs */
#define LARGE_PRIME 2147483647

TLS sb_rng_state_t sb_rng_state CK_CC_CACHELINE;

/* Exported variables */
int sb_rand_seed; /* optional seed set on the command line */

/* Random numbers command line options */

static sb_arg_t rand_args[] =
{
  {"rand-type", "random numbers distribution {uniform,gaussian,special,pareto}",
   SB_ARG_TYPE_STRING, "special"},
  {"rand-spec-iter", "number of iterations used for numbers generation", SB_ARG_TYPE_INT, "12"},
  {"rand-spec-pct", "percentage of values to be treated as 'special' (for special distribution)",
   SB_ARG_TYPE_INT, "1"},
  {"rand-spec-res", "percentage of 'special' values to use (for special distribution)",
   SB_ARG_TYPE_INT, "75"},
  {"rand-seed", "seed for random number generator, ignored when 0", SB_ARG_TYPE_INT, "0"},
  {"rand-pareto-h", "parameter h for pareto distibution", SB_ARG_TYPE_FLOAT,
   "0.2"},
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

static rand_dist_t rand_type;
static int (*rand_func)(int, int); /* pointer to random numbers generator */
static unsigned int rand_iter;
static unsigned int rand_pct;
static unsigned int rand_res;

/* parameters for Pareto distribution */
static double pareto_h; /* parameter h */
static double pareto_power; /* parameter pre-calculated by h */

/* Random seed used to generate unique random numbers */
static unsigned long long rnd_seed;
/* Mutex to protect random seed */
static pthread_mutex_t    rnd_mutex;

int sb_rand_register(void)
{
  sb_register_arg_set(rand_args);

  return 0;
}

/* Initialize random numbers generation */

int sb_rand_init(void)
{
  char     *s;

  sb_rand_seed = sb_get_value_int("rand-seed");

  s = sb_get_value_string("rand-type");
  if (!strcmp(s, "uniform"))
  {
    rand_type = DIST_TYPE_UNIFORM;
    rand_func = &sb_rand_uniform;
  }
  else if (!strcmp(s, "gaussian"))
  {
    rand_type = DIST_TYPE_GAUSSIAN;
    rand_func = &sb_rand_gaussian;
  }
  else if (!strcmp(s, "special"))
  {
    rand_type = DIST_TYPE_SPECIAL;
    rand_func = &sb_rand_special;
  }
  else if (!strcmp(s, "pareto"))
  {
    rand_type = DIST_TYPE_PARETO;
    rand_func = &sb_rand_pareto;
  }
  else
  {
    log_text(LOG_FATAL, "Invalid random numbers distribution: %s.", s);
    return 1;
  }

  rand_iter = sb_get_value_int("rand-spec-iter");
  rand_pct = sb_get_value_int("rand-spec-pct");
  rand_res = sb_get_value_int("rand-spec-res");

  pareto_h  = sb_get_value_float("rand-pareto-h");
  pareto_power = log(pareto_h) / log(1.0-pareto_h);

  /* Initialize random seed  */
  rnd_seed = LARGE_PRIME;
  pthread_mutex_init(&rnd_mutex, NULL);

  /* Seed PRNG for the main thread. Worker thread do their own seeding */
  sb_rand_thread_init();

  return 0;
}


void sb_rand_print_help(void)
{
  printf("Pseudo-Random Numbers Generator options:\n");

  sb_print_options(rand_args);
}


void sb_rand_done(void)
{
  pthread_mutex_destroy(&rnd_mutex);
}

/* Initialize thread-local RNG state */

void sb_rand_thread_init(void)
{
  /* We use libc PRNG to see xoroshiro128+ */
  sb_rng_state[0] = ((((uint64_t) random()) % UINT32_MAX) << 32) |
    (((uint64_t) random()) % UINT32_MAX);
  sb_rng_state[1] = ((((uint64_t) random()) % UINT32_MAX) << 32) |
    (((uint64_t) random()) % UINT32_MAX);
}

/*
  Return random number in the specified range with distribution specified
  with the --rand-type command line option
*/

int sb_rand_default(int a, int b)
{
  return rand_func(a,b);
}

/* uniform distribution */

int sb_rand_uniform(int a, int b)
{
  return a + sb_rand_uniform_uint64() % (b - a + 1);
}

/* gaussian distribution */

int sb_rand_gaussian(int a, int b)
{
  int          sum;
  unsigned int i, t;

  t = b - a + 1;
  for(i=0, sum=0; i < rand_iter; i++)
    sum += sb_rand_uniform_uint64() % t;

  return a + sum / rand_iter;
}

/* 'special' distribution */

int sb_rand_special(int a, int b)
{
  int          sum = 0;
  unsigned int i;
  unsigned int d;
  unsigned int t;
  unsigned int res;
  unsigned int range_size;

  if (a >= b)
    return 0;

  t = b - a + 1;

  /* Increase range size for special values. */
  range_size = t * (100 / (100 - rand_res));

  /* Generate uniformly distributed one at this stage  */
  res = sb_rand_uniform_uint64() % range_size;

  /* For first part use gaussian distribution */
  if (res < t)
  {
    for(i = 0; i < rand_iter; i++)
      sum += sb_rand_uniform_uint64() % t;
    return a + sum / rand_iter;
  }

  /*
   * For second part use even distribution mapped to few items
   * We shall distribute other values near by the center
   */
  d = t * rand_pct / 100;
  if (d < 1)
    d = 1;
  res %= d;

  /* Now we have res values in SPECIAL_PCT range of the data */
  res += (t / 2 - t * rand_pct / (100 * 2));

  return a + res;
}

/* Pareto distribution */

int sb_rand_pareto(int a, int b)
{
  return a + (int)(b - a + 1) * pow(sb_rand_uniform_double(), pareto_power);
}

/* Generate unique random id */


int sb_rand_uniq(int a, int b)
{
  int res;

  pthread_mutex_lock(&rnd_mutex);
  res = (unsigned int) (rnd_seed % (b - a + 1)) ;
  rnd_seed += LARGE_PRIME;
  pthread_mutex_unlock(&rnd_mutex);

  return res + a;
}


/* Generate random string */


void sb_rand_str(const char *fmt, char *buf)
{
  unsigned int i;

  for (i=0; fmt[i] != '\0'; i++)
  {
    if (fmt[i] == '#')
      buf[i] = sb_rand_uniform('0', '9');
    else if (fmt[i] == '@')
      buf[i] = sb_rand_uniform('a', 'z');
    else
      buf[i] = fmt[i];
  }
}
