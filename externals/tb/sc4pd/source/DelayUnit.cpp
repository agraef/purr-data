/* sc4pd 
   public class for several delay objects

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

        Coded while listening to: 

*/

#include "sc4pd.hpp"
#include "DelayUnit.hpp"

void DelayUnit_ar::DelayUnit_AllocDelayLine()
{
    long delaybufsize = (long)ceil(m_maxdelaytime * Samplerate() + 1.f);
    delaybufsize = delaybufsize + Blocksize();
    delaybufsize = NEXTPOWEROFTWO(delaybufsize);  // round up to next power of two
    m_fdelaylen = m_idelaylen = delaybufsize;
    
    delete[] m_dlybuf;
    m_dlybuf = new float[delaybufsize] ;
    m_mask = delaybufsize - 1;
}

void DelayUnit_ar::DelayUnit_Dtor()
{
    delete[] m_dlybuf;
}

float DelayUnit_ar::CalcDelay(float delaytime)
{
    float next_dsamp = delaytime * Samplerate();
    return sc_clip(next_dsamp, 1.f, m_fdelaylen);
}

void DelayUnit_ar::DelayUnit_Reset()
{
    m_dlybuf = 0;
    
    DelayUnit_AllocDelayLine();
    
    m_dsamp = CalcDelay(m_delaytime);	
    
    m_numoutput = 0;
    m_iwrphase = 0;
}

void FeedbackDelay_ar::FeedbackDelay_Reset()
{
    DelayUnit_Reset();
    
    m_feedbk = CalcFeedback(m_delaytime, m_decaytime);
}
