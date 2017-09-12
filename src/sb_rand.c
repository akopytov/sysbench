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

/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#include "sb_ck_pr.h"

TLS sb_rng_state_t sb_rng_state CK_CC_CACHELINE;

/* Exported variables */
int sb_rand_seed; /* optional seed set on the command line */

/* Random numbers command line options */

static sb_arg_t rand_args[] =
{
  SB_OPT("rand-type",
         "random numbers distribution {uniform, gaussian, special, pareto, "
         "zipfian} to use by default", "special", STRING),
  SB_OPT("rand-seed",
         "seed for random number generator. When 0, the current time is "
         "used as an RNG seed.", "0", INT),
  SB_OPT("rand-spec-iter",
         "number of iterations for the special distribution", "12", INT),
  SB_OPT("rand-spec-pct",
         "percentage of the entire range where 'special' values will fall "
         "in the special distribution",
         "1", INT),
  SB_OPT("rand-spec-res",
         "percentage of 'special' values to use for the special distribution",
         "75", INT),
  SB_OPT("rand-pareto-h", "shape parameter for the Pareto distribution",
         "0.2", DOUBLE),
  SB_OPT("rand-zipfian-exp",
         "shape parameter (exponent, theta) for the Zipfian distribution",
         "0.8", DOUBLE),

  SB_OPT_END
};

static rand_dist_t rand_type;
/* pointer to the default PRNG as defined by --rand-type */
static uint32_t (*rand_func)(uint32_t, uint32_t);
static unsigned int rand_iter;
static unsigned int rand_pct;
static unsigned int rand_res;

/*
  Pre-computed FP constants to avoid unnecessary conversions and divisions at
  runtime.
*/
static double rand_iter_mult;
static double rand_pct_mult;
static double rand_pct_2_mult;
static double rand_res_mult;

/* parameters for Pareto distribution */
static double pareto_h; /* parameter h */
static double pareto_power; /* parameter pre-calculated by h */

/* parameter and precomputed values for the Zipfian distribution */
static double zipf_exp;
static double zipf_s;
static double zipf_hIntegralX1;

/* Unique sequence generator state */
static uint32_t rand_unique_index CK_CC_CACHELINE;
static uint32_t rand_unique_offset;

extern inline uint64_t sb_rand_uniform_uint64(void);
extern inline double sb_rand_uniform_double(void);
extern inline uint64_t xoroshiro_rotl(const uint64_t, int);
extern inline uint64_t xoroshiro_next(uint64_t s[2]);

static void rand_unique_seed(uint32_t index, uint32_t offset);

/* Helper functions for the Zipfian distribution */
static double hIntegral(double x, double e);
static double hIntegralInverse(double x, double e);
static double h(double x, double e);
static double helper1(double x);
static double helper2(double x);

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
  else if (!strcmp(s, "zipfian"))
  {
    rand_type = DIST_TYPE_ZIPFIAN;
    rand_func = &sb_rand_zipfian;
  }
  else
  {
    log_text(LOG_FATAL, "Invalid random numbers distribution: %s.", s);
    return 1;
  }

  rand_iter = sb_get_value_int("rand-spec-iter");
  rand_iter_mult = 1.0 / rand_iter;

  rand_pct = sb_get_value_int("rand-spec-pct");
  rand_pct_mult = rand_pct / 100.0;
  rand_pct_2_mult = rand_pct / 200.0;

  rand_res = sb_get_value_int("rand-spec-res");
  rand_res_mult = 100.0 / (100.0 - rand_res);

  pareto_h  = sb_get_value_double("rand-pareto-h");
  pareto_power = log(pareto_h) / log(1.0-pareto_h);

  zipf_exp = sb_get_value_double("rand-zipfian-exp");
  if (zipf_exp < 0)
  {
    log_text(LOG_FATAL, "--rand-zipfian-exp must be >= 0");
    return 1;
  }

  zipf_s = 2 - hIntegralInverse(hIntegral(2.5, zipf_exp) - h(2, zipf_exp),
                                zipf_exp);
  zipf_hIntegralX1 = hIntegral(1.5, zipf_exp) - 1;

  /* Seed PRNG for the main thread. Worker threads do their own seeding */
  sb_rand_thread_init();

  /* Seed the unique sequence generator */
  rand_unique_seed(random(), random());

  return 0;
}


void sb_rand_print_help(void)
{
  printf("Pseudo-Random Numbers Generator options:\n");

  sb_print_options(rand_args);
}


void sb_rand_done(void)
{
}

/* Initialize thread-local RNG state */

void sb_rand_thread_init(void)
{
  /* We use libc PRNG to seed xoroshiro128+ */
  sb_rng_state[0] = (((uint64_t) random()) << 32) |
    (((uint64_t) random()) & UINT32_MAX);
  sb_rng_state[1] = (((uint64_t) random()) << 32) |
    (((uint64_t) random()) & UINT32_MAX);
}

/*
  Return random number in the specified range with distribution specified
  with the --rand-type command line option
*/

uint32_t sb_rand_default(uint32_t a, uint32_t b)
{
  return rand_func(a,b);
}

/* uniform distribution */

uint32_t sb_rand_uniform(uint32_t a, uint32_t b)
{
  return a + sb_rand_uniform_double() * (b - a + 1);
}

/* gaussian distribution */

uint32_t sb_rand_gaussian(uint32_t a, uint32_t b)
{
  double       sum;
  double       t;
  unsigned int i;

  t = b - a + 1;
  for(i=0, sum=0; i < rand_iter; i++)
    sum += sb_rand_uniform_double() * t;

  return a + (uint32_t) (sum * rand_iter_mult) ;
}

/* 'special' distribution */

uint32_t sb_rand_special(uint32_t a, uint32_t b)
{
  double       sum;
  double       t;
  double       range_size;
  double       res;
  double       d;
  double       rnd;
  unsigned int i;

  t = b - a;

  /* Increase range size for special values. */
  range_size = t * rand_res_mult;

  /* Generate uniformly distributed one at this stage  */
  rnd = sb_rand_uniform_double(); /* Random double in the [0, 1) interval */
  /* Random integer in the [0, range_size) interval */
  res = rnd * range_size;

  /*
    Use gaussian distribution for (100 - rand_res) percent of all generated
    values.
  */
  if (res < t)
  {
    sum = 0.0;

    for(i = 0; i < rand_iter; i++)
      sum += sb_rand_uniform_double();

    return a + sum * t * rand_iter_mult;
  }

  /*
    For the remaining rand_res percent of values use the uniform
    distribution. We map previously generated random double in the [0, 1)
    interval to the rand_pct percent part of the [a, b] interval. Then we move
    the resulting value in the [0, (b-a) * (rand_pct / 100)] interval to the
    center of the original interval [a, b].
  */
  d = t * rand_pct_mult;
  res = rnd * (d + 1);
  res += t / 2 - t * rand_pct_2_mult;

  return a + (uint32_t) res;
}

/* Pareto distribution */

uint32_t sb_rand_pareto(uint32_t a, uint32_t b)
{
  return a + (uint32_t) ((b - a + 1) *
                         pow(sb_rand_uniform_double(), pareto_power));
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

/*
  Unique random sequence generator. This is based on public domain code from
  https://github.com/preshing/RandomSequence
*/

static uint32_t rand_unique_permute(uint32_t x)
{
  static const uint32_t prime = UINT32_C(4294967291);

  if (x >= prime)
    return x; /* The 5 integers out of range are mapped to themselves. */

  uint32_t residue = ((uint64_t) x * x) % prime;
  return (x <= prime / 2) ? residue : prime - residue;
}


static void rand_unique_seed(uint32_t index, uint32_t offset)
{
  rand_unique_index = rand_unique_permute(rand_unique_permute(index) +
                                          0x682f0161);
  rand_unique_offset = rand_unique_permute(rand_unique_permute(offset) +
                                          0x46790905);
}

/* This is safe to be called concurrently from multiple threads */

uint32_t sb_rand_unique(void)
{
  uint32_t index = ck_pr_faa_32(&rand_unique_index, 1);

  return rand_unique_permute((rand_unique_permute(index) + rand_unique_offset) ^
                             0x5bf03635);
}

/*
  Implementation of the Zipf distribution is based on
  RejectionInversionZipfSampler.java from the Apache Commons RNG project
  (https://commons.apache.org/proper/commons-rng/) implementing the rejection
  inversion sampling method for a descrete, bounded Zipf distribution that is in
  turn based on the method described in:

  Wolfgang Hörmann and Gerhard Derflinger. "Rejection-inversion to generate
  variates from monotone discrete distributions", ACM Transactions on Modeling
  and Computer Simulation, (TOMACS) 6.3 (1996): 169-184.
*/

static uint32_t sb_rand_zipfian_int(uint32_t n, double e, double s,
                                    double hIntegralX1)
{
  /*
    The paper describes an algorithm for exponents larger than 1 (Algorithm
    ZRI).

    The original method uses

    H(x) = (v + x)^(1 - q) / (1 - q)

    as the integral of the hat function.  This function is undefined for q = 1,
    which is the reason for the limitation of the exponent.  If instead the
    integral function

    H(x) = ((v + x)^(1 - q) - 1) / (1 - q)

    is used, for which a meaningful limit exists for q = 1, the method works for
    all positive exponents.  The following implementation uses v = 0 and
    generates integral number in the range [1, numberOfElements].  This is
    different to the original method where v is defined to be positive and
    numbers are taken from [0, i_max].  This explains why the implementation
    looks slightly different.
  */
  const double hIntegralNumberOfElements = hIntegral(n + 0.5, e);

  for (;;)
  {
    double u = hIntegralNumberOfElements + sb_rand_uniform_double() *
      (hIntegralX1 - hIntegralNumberOfElements);
    /* u is uniformly distributed in (hIntegralX1, hIntegralNumberOfElements] */

    double x = hIntegralInverse(u, e);
    uint32_t k = (uint32_t) (x + 0.5);

    /*
      Limit k to the range [1, numberOfElements] if it would be outside due to
      numerical inaccuracies.
    */
    if (SB_UNLIKELY(k < 1))
      k = 1;
    else if (SB_UNLIKELY(k > n))
      k = n;

    /*
      Here, the distribution of k is given by:

      P(k = 1) = C * (hIntegral(1.5) - hIntegralX1) = C
      P(k = m) = C * (hIntegral(m + 1/2) - hIntegral(m - 1/2)) for m >= 2

      where C = 1 / (hIntegralNumberOfElements - hIntegralX1)
    */

    if (k - x <= s || u >= hIntegral(k + 0.5, e) - h(k, e))
    {
      /*
        Case k = 1:

        The right inequality is always true, because replacing k by 1 gives u >=
        hIntegral(1.5) - h(1) = hIntegralX1 and u is taken from (hIntegralX1,
        hIntegralNumberOfElements].

        Therefore, the acceptance rate for k = 1 is P(accepted | k = 1) = 1 and
        the probability that 1 is returned as random value is P(k = 1 and
        accepted) = P(accepted | k = 1) * P(k = 1) = C = C / 1^exponent

        Case k >= 2:

        The left inequality (k - x <= s) is just a short cut to avoid the more
        expensive evaluation of the right inequality (u >= hIntegral(k + 0.5) -
        h(k)) in many cases.

        If the left inequality is true, the right inequality is also true:
          Theorem 2 in the paper is valid for all positive exponents, because
          the requirements h'(x) = -exponent/x^(exponent + 1) < 0 and
          (-1/hInverse'(x))'' = (1+1/exponent) * x^(1/exponent-1) >= 0 are both
          fulfilled.  Therefore, f(x) = x - hIntegralInverse(hIntegral(x + 0.5)
          - h(x)) is a non-decreasing function. If k - x <= s holds, k - x <= s
          + f(k) - f(2) is obviously also true which is equivalent to -x <=
          -hIntegralInverse(hIntegral(k + 0.5) - h(k)), -hIntegralInverse(u) <=
          -hIntegralInverse(hIntegral(k + 0.5) - h(k)), and finally u >=
          hIntegral(k + 0.5) - h(k).

        Hence, the right inequality determines the acceptance rate: P(accepted |
        k = m) = h(m) / (hIntegrated(m+1/2) - hIntegrated(m-1/2)) The
        probability that m is returned is given by P(k = m and accepted) =
        P(accepted | k = m) * P(k = m) = C * h(m) = C / m^exponent.

      In both cases the probabilities are proportional to the probability mass
      function of the Zipf distribution.
      */

      return k;
    }
  }
}

uint32_t sb_rand_zipfian(uint32_t a, uint32_t b)
{
  /* sb_rand_zipfian_int() returns a number in the range [1, b - a + 1] */
  return a +
    sb_rand_zipfian_int(b - a + 1, zipf_exp, zipf_s, zipf_hIntegralX1) - 1;
}

/*
  H(x) is defined as

  (x^(1 - exponent) - 1) / (1 - exponent), exponent != 1
  log(x), if exponent == 1

  H(x) is an integral function of h(x), the derivative of H(x) is h(x).
*/

static double hIntegral(double x, double e)
{
  const double logX = log(x);
  return helper2((1 - e) * logX) * logX;
}

/* h(x) = 1 / x^exponent */

static double h(double x, double e)
{
  return exp(-e * log(x));
}

/* The inverse function of H(x) */

static double hIntegralInverse(double x, double e)
{
  double t = x * (1 -e);
  if (t < -1)
  {
    /*
      Limit value to the range [-1, +inf).
      t could be smaller than -1 in some rare cases due to numerical errors.
    */
    t = -1;
  }

  return exp(helper1(t) * x);
}

/*
 Helper function that calculates log(1 + x) / x.

 A Taylor series expansion is used, if x is close to 0.
*/

static double helper1(double x)
{
  if (fabs(x) > 1e-8)
    return log1p(x) / x;
  else
    return 1 - x * (0.5 - x * (0.33333333333333333 - 0.25 * x));
}

/*
 Helper function that calculates (exp(x) - 1) / x.

 A Taylor series expansion is used, if x is close to 0.
*/

static double helper2(double x)
{
  if (fabs(x) > 1e-8)
    return expm1(x) / x;
  else
    return 1 + x * 0.5 * (1 + x * 0.33333333333333333 * (1 + 0.25 * x));
}
