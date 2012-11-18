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

//#include "sc4pd.hpp"

class DelayUnit_ar 
    : public sc4pd_dsp
{
    FLEXT_HEADER(DelayUnit_ar,sc4pd_dsp);
    
public:
    /* functions */
    void DelayUnit_AllocDelayLine();
    void DelayUnit_Reset();  
    float CalcDelay(float delaytime);
    void DelayUnit_Dtor();

    /* data */
    float *m_dlybuf;
    float m_dsamp, m_fdelaylen;
    float m_delaytime, m_maxdelaytime;
    long m_iwrphase, m_idelaylen, m_mask;
    long m_numoutput;
};

/* todo: a delay for control messages? */

class FeedbackDelay_ar : public DelayUnit_ar
{
    FLEXT_HEADER(FeedbackDelay_ar,DelayUnit_ar);
    float m_feedbk, m_decaytime;
    void FeedbackDelay_Reset();
};
