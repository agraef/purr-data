/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_H
#define __VASP_H

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#include <typeinfo>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <complex>
#include <valarray>
#include <limits>

// enable attributes
#define FLEXT_ATTRIBUTES 1

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 502)
#error You need at least flext version 0.5.2
#endif

#if defined(_MSC_VER) && !defined(FLEXT_DEBUG)
// switch off warnings for the release build
#pragma warning(disable: 4244)
#endif

typedef std::complex<float> complex;
typedef std::valarray<float> vector;

#define I int
#define L long
#define UL unsigned long
#define F float
#define D double
#define C char
#define BL bool
#define V void
#define S t_sample // type for samples
#define BS flext::buffer::Element // type for buffer sample elements
#define R double // type for internal calculations
#define CX complex
#define VX vector

#if FLEXT_SYS == FLEXT_SYS_PD
// buffers are never interleaved - special optimizations may occur
// attention: possibly obsolete when immediate file access is implemented
#define VASP_CHN1  
#endif

#endif
