/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_UTIL_H
#define __VASP_UTIL_H

#include "main.h"
#include <algorithm>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

using std::min;
using std::max;
using std::swap;

template<class T>
T arg(T re,T im)
{
    if(re) 
	return (T)(fmod(atan(im/re)+(re < 0?2*M_PI:M_PI),2*M_PI)-M_PI);
    else
	if(im || re) return (T)(im > 0?M_PI/2:-M_PI/2);
        else return 0;
}

template<class T>
inline T sgn(T x) { return (T)(x?(x < 0?-1:1):0); }

template<class T>
inline T sqabs(T re,T im) { return re*re+im*im; }

#endif
