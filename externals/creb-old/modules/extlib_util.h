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

/* envelope stuff */

/* exponential range for envelopes is 60dB */
#define ENVELOPE_RANGE 0.001f
#define ENVELOPE_MAX   (1.0f - ENVELOPE_RANGE)
#define ENVELOPE_MIN   ENVELOPE_RANGE

/* convert milliseconds to 1-p, with p a real pole */
static inline float milliseconds_2_one_minus_realpole(float time)
{
  float r;

  if (time < 0.0f) time = 0.0f;
  r = -expm1(1000.0f * log(ENVELOPE_RANGE) / (sys_getsr() * time));
  if (!(r < 1.0f)) r = 1.0f;

  //post("%f",r);
  return r;
}


typedef union
{
    unsigned int i;
    float f;
} t_flint;

/* check if floating point number is denormal */

//#define IS_DENORMAL(f) (((*(unsigned int *)&(f))&0x7f800000) == 0) 

#define IS_DENORMAL(f) (((((t_flint)(f)).i) & 0x7f800000) == 0)


#endif /* CREB_EXTLIB_UTIL_H */

