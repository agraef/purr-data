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
     
   Coded while listening to: Morton Feldman: For John Cage
   
*/

#ifndef _SC4PD_HPP

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 406)
#error You need at least FLEXT version 0.4.6
#endif

#include "SC_PlugIn.h"
#include "support.hpp"

/* this macro has to be redefined to work with flext */

// calculate a slope for control rate interpolation to audio rate.
//#define CALCSLOPE(next,prev) ((next - prev) * unit->mRate->mSlopeFactor)
#undef CALCSLOPE
#define CALCSLOPE(next,prev) ((next - prev) * 1/ Blocksize())



//#define SAMPLERATE (unit->mRate->mSampleRate)
#undef SAMPLERATE
#define SAMPLERATE Samplerate()

//#define BUFLENGTH (unit->mBufLength)
#undef BUFLENGTH
#define BUFLENGTH Blocksize()

/* to make sure the behaviour is consistent: */

#undef ZXP
#define ZXP(z) (*(z)++)



class sc4pd_dsp 
    : public flext_dsp
{
    FLEXT_HEADER(sc4pd_dsp,flext_dsp);


/* some initialisation functions, adapted from SC_Rate.cpp*/

    inline float sc_sampledur()
    {
	return 1 / Samplerate();
    }
    
    inline float sc_radianspersample()
    {
	return twopi / Samplerate();
    }
    
    inline float sc_bufduration()
    {
	return Blocksize() / Samplerate();
    }
    
    inline float sc_bufrate()
    {
	return 1 / sc_bufduration();
    }
    
    inline float sc_slopefactor()
    {
	return 1 / Blocksize();
    }
    
    inline int sc_filterloops()
    {
	return Blocksize() / 3;
    }
    
    inline int sc_filterremain()
    {
	return Blocksize() % 3;
    }
    
    inline float sc_filterslope()
    {
	float f = sc_filterloops();
	if (f == 0)
	    return 0;
	else
	    return 1. / f;
    }

};

    
#define _SC4PD_HPP
#endif
