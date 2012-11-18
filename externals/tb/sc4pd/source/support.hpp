/* sc4pd: 
   support functions
   
   Copyright (c) 2004 Tim Blechmann.                                       

   This code is derived from:
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,             
   but WITHOUT ANY WARRANTY; without even the implied warranty of         
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   Based on:
     PureData by Miller Puckette and others.
         http://www.crca.ucsd.edu/~msp/software.html
     FLEXT by Thomas Grill
         http://www.parasitaere-kapazitaeten.net/ext
     SuperCollider by James McCartney
         http://www.audiosynth.com
     
   Coded while listening to: Phosphor
   
*/

#ifndef _SUPPORT_HPP
#define _SUPPORT_HPP

#include <flext.h>
#include "SC_PlugIn.h"


#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 406)
#error You need at least FLEXT version 0.4.6
#endif


/* for argument parsing */
bool sc_add (flext::AtomList& a);
float sc_getfloatarg (flext::AtomList& a,int i);
bool sc_ar(flext::AtomList& a);
bool sc_inv(flext::AtomList& a);


/* for rngs */

// macros to put rgen state in registers
#define RGET \
	uint32 s1 = rgen.s1; \
	uint32 s2 = rgen.s2; \
	uint32 s3 = rgen.s3; 

#define RPUT \
	rgen.s1 = s1; \
	rgen.s2 = s2; \
	rgen.s3 = s3;

int32 timeseed();

/* cubic interpolation from DelayUGens.cpp */
inline float cubicinterp(float x, float y0, float y1, float y2, float y3)
{
	// 4-point, 3rd-order Hermite (x-form)
	float c0 = y1;
	float c1 = 0.5f * (y2 - y0);
	float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
	float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

	return ((c3 * x + c2) * x + c1) * x + c0;
}

/* feedback calculation from DelayUGens.cpp */
inline float CalcFeedback(float delaytime, float decaytime)
{
	if (delaytime == 0.f) {
		return 0.f;
	} else if (decaytime > 0.f) {
		return exp(log001 * delaytime / decaytime);
	} else if (decaytime < 0.f) {
		return -exp(log001 * delaytime / -decaytime);
	} else {
		return 0.f;
	}
}


/* this is adapted from thomas grill's xsample:
xsample - extended sample objects for Max/MSP and pd (pure data)

Copyright (c) 2001-2004 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#define SETSIGFUN(VAR,FUN) v_##VAR = FUN

#define DEFSIGFUN(NAME)	void NAME(int n,t_sample *const *in,t_sample *const *out)

#define DEFSIGCALL(NAME) \
	inline void NAME(int n,t_sample *const *in,t_sample *const *out) \
        { (this->*v_##NAME)(n,in,out); } \
	void (thisType::*v_##NAME)(int n,t_sample *const *invecs,t_sample *const *outvecs)

#define SIGFUN(FUN) &thisType::FUN


/* from Convolution.cpp */
void init_ffts();
float* create_fftwindow(int log2n);
float* create_cosTable(int log2n);
void DoWindowing(int log2n, float * fftbuf, int bufsize);

#endif
