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
     
   Coded while listening to: Nmperign & Guenter Mueller: More Gloom, More Light
   
*/

#include "sc4pd.hpp"

#include <flsupport.h>


bool sc_add (flext::AtomList& a)
{
    for (int i = 0; i!=a.Count();++i)
    {
	if ( flext::IsSymbol(a[i]) )
	{
	    const char * teststring; 
	    teststring = flext::GetString(a[i]);
	    if((strcmp(teststring,"add"))==0)
		return true;
	}
    }
    return false;
}

float sc_getfloatarg (flext::AtomList& a,int i)
{
    if (a.Count() > 0 && a.Count() > i)
	return flext::GetAFloat(a[i]);
    else 
	return 0;
}

bool sc_ar(flext::AtomList& a)
{
    for (int i = 0; i!=a.Count();++i)
    {
	if ( flext::IsSymbol(a[i]) )
	{
	    const char * teststring; 
	    teststring = flext::GetString(a[i]);
	    if((strcmp(teststring,"ar"))==0)
		return true;
	}
    }
    return false;
}

bool sc_inv(flext::AtomList& a)
{
    for (int i = 0; i!=a.Count();++i)
    {
	if ( flext::IsSymbol(a[i]) )
	{
	    const char * teststring; 
	    teststring = flext::GetString(a[i]);
	    if((strcmp(teststring,"inv"))==0)
		return true;
	}
    }
    return false;
}

int32 timeseed()
{
	static int32 count = 0;

	double time = flext::GetOSTime();
	
	double sec = trunc(time);
	double usec = (time-sec)*1e6;
	
	time_t tsec = sec;
	useconds_t tusec =usec;       /* not exacty the way, it's calculated
					  in SuperCollider, but it's only 
					  the seed */
	
	return (int32)tsec ^ (int32)tusec ^ count--;
}

/* from Convolution.cpp */
extern "C" 
{
    float *cosTable[32];
    float *fftWindow[32];
}


float* create_cosTable(int log2n)
{
	int size = 1 << log2n;
	int size2 = size / 4 + 1;
	float *win = (float*)malloc(size2 * sizeof(float));
	double winc = twopi / size;
	for (int i=0; i<size2; ++i) {
		double w = i * winc;
		win[i] = cos(w);
	}
	return win;
}

float* create_fftwindow(int log2n)
{
	int size = 1 << log2n;
	float *win = (float*)malloc(size * sizeof(float));
	//double winc = twopi / size;
	double winc = pi / size;
	for (int i=0; i<size; ++i) {
		double w = i * winc;
		//win[i] = 0.5 - 0.5 * cos(w);
		win[i] = sin(w);
	}
	return win;
}

void init_ffts()
{
#if __VEC__
	
	for (int i=0; i<32; ++i) {
		fftsetup[i] = 0;
	}
	for (int i=0; i<15; ++i) {
		fftsetup[i] = create_fftsetup(i, kFFTRadix2);
	}
#else
	for (int i=0; i<32; ++i) {
		cosTable[i] = 0;
		fftWindow[i] = 0;
	}
	for (int i=3; i<15; ++i) {
		cosTable[i] = create_cosTable(i);
		fftWindow[i] = create_fftwindow(i);
	}
#endif
}

void DoWindowing(int log2n, float * fftbuf, int bufsize)
{
	float *win = fftWindow[log2n];
	
	//printf("fail? %i %d /n", log2n, win);
	
	if (!win) return;
	float *in = fftbuf - 1;
	win--;
        
	for (int i=0; i<bufsize; ++i) {
		*++in *= *++win;
	}
}

#include "fftlib.c"
