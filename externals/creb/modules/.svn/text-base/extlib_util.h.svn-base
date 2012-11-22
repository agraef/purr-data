/*
 *   Prototypes for utility functions used in pd externals 
 *   Copyright (c) 2000-2003 by Tom Schouten
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef CREB_EXTLIB_UTIL_H
#define CREB_EXTLIB_UTIL_H

#include <math.h>
#include "m_pd.h"

// __COUNTER__ is a CPP extension enabled in gcc >= 4.3
#if defined __GNUC__ && __GNUC__ == 4 && __GNUC_MINOR__ >= 3
#define HAVE_COUNTER
#endif
#if defined __GNUC__ && __GNUC__ >= 5
#define HAVE_COUNTER
#endif
#ifdef __clang__ // Should we check version here?
#define HAVE_COUNTER
#endif
#ifndef HAVE_COUNTER
#warning CT_ASSERT() disabled
#define CT_ASSERT(...)
#else
#define CT_NAMED_ASSERT(name,x)                         \
    typedef int _GENSYM(name ## _ctassert_)[-((x)==0)]
#define CT_ASSERT(x) CT_NAMED_ASSERT(,x)
#define _GENSYM(x) _CONCAT(x,__COUNTER__)
#define _CONCAT1(x,y) x##y
#define _CONCAT(x,y) _CONCAT1(x,y)
#endif

/* Only defined in pd-extended. */
#ifndef PD_FLOAT_PRECISION
#define PD_FLOAT_PRECISION 32
#endif

/* http://www.unix.org/version2/whatsnew/lp64_wp.html */
typedef unsigned long long u64;
typedef unsigned int u32;
CT_ASSERT(sizeof(u32)==4);
CT_ASSERT(sizeof(u64)==8);

/* envelope stuff */

/* exponential range for envelopes is 60dB */
#define ENVELOPE_RANGE 0.001
#define ENVELOPE_MAX   (1.0 - ENVELOPE_RANGE)
#define ENVELOPE_MIN   ENVELOPE_RANGE

/* convert milliseconds to 1-p, with p a real pole */
static inline t_float milliseconds_2_one_minus_realpole(t_float time)
{
  t_float r;

  if (time < 0.0) time = 0.0;
  r = -expm1(1000.0 * log(ENVELOPE_RANGE) / (sys_getsr() * time));
  if (!(r < 1.0)) r = 1.0;

  //post("%f",r);
  return r;
}

#if defined(__i386__) || defined(__x86_64__) // type punning code:

#if PD_FLOAT_PRECISION == 32

typedef union
{
    unsigned int i;
    t_float f;
} t_flint;

/* check if floating point number is denormal */

//#define IS_DENORMAL(f) (((*(unsigned int *)&(f))&0x7f800000) == 0) 

#define IS_DENORMAL(f) (((((t_flint)(f)).i) & 0x7f800000) == 0)

#elif PD_FLOAT_PRECISION == 64

typedef union
{
    unsigned int i[2];
    t_float f;
} t_flint;

#define IS_DENORMAL(f) (((((t_flint)(f)).i[1]) & 0x7ff00000) == 0)

#endif // endif PD_FLOAT_PRECISION

#else   // if not defined(__i386__) || defined(__x86_64__)
#define IS_DENORMAL(f) 0
#endif // end if defined(__i386__) || defined(__x86_64__)

#endif /* CREB_EXTLIB_UTIL_H */

