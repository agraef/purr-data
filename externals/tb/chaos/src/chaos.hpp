//
//
//  chaos~
//  Copyright (C) 2004  Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef __chaos_hpp

#define FLEXT_ATTRIBUTES 1

#define _USE_MATH_DEFINES /* tg says: define this before including cmath */
#include "flext.h"
#include "assert.h"
#include "chaos_defs.hpp"
#include <cmath>

#include <cstdlib>

#if FLEXT_SYS != FLEXT_SYS_PD
#define PD_BIGORSMALL(f) ((((*(unsigned int*)&(f))&0x60000000)==0) || \
      (((*(unsigned int*)&(f))&0x60000000)==0x60000000))
#endif

/* internal we can work with a higher precision than pd */
#ifdef DOUBLE_PRECISION
typedef double data_t;
#else
typedef float data_t;
#endif

inline data_t rand_range(data_t low, data_t high)
{
    return low + ( (rand() * (high - low)) / RAND_MAX);
}

#define __chaos_hpp
#endif /* __chaos_hpp */
