/* sc4pd 
   HPF~

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
     
   Coded while listening to: Peter Kowald & Tatsuya Nakatani: 
                             13 Definitions of Truth

*/

#include "sc4pd.hpp"

/* ------------------------ HPF~ -------------------------------*/

class HPF_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(HPF_ar,sc4pd_dsp);
    
public:
    HPF_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out)
    {
	mRadiansPerSample = sc_radianspersample();
	mFilterSlope = sc_filterslope();
	mFilterLoops = sc_filterloops();
	mFilterRemain = sc_filterremain();
    }

    void m_set_freq(float f)
    {
	m_freq=f;
	changed = true;
    }

private:
    float m_y1, m_y2, m_a0, m_b1, m_b2, m_freq;
    bool changed;
    float mRadiansPerSample, mFilterSlope;
    int mFilterLoops, mFilterRemain; 
    
    FLEXT_CALLBACK_F(m_set_freq);
};

FLEXT_LIB_DSP_V("HPF~",HPF_ar);

HPF_ar::HPF_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"kfreq",m_set_freq);

    //parse arguments
    AtomList Args(argc,argv);
    
    m_freq = sc_getfloatarg(Args,0);
    changed = true;
    
    AddOutSignal();

    m_a0 = 0.f;
    m_b1 = 0.f;
    m_b2 = 0.f;
    m_y1 = 0.f;
    m_y2 = 0.f;
}

void HPF_ar::m_signal(int n, t_sample *const *in, 
			  t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    
    float y0;
    float y1 = m_y1;
    float y2 = m_y2;
    float a0 = m_a0;
    float b1 = m_b1;
    float b2 = m_b2;

    if (changed)
    {
	float pfreq = m_freq * mRadiansPerSample * 0.5;
	
	float C = tan(pfreq);
	float C2 = C * C;
	float sqrt2C = C * sqrt2;
	float next_a0 = 1.f / (1.f + sqrt2C + C2);
	float next_b1 = 2.f * (1.f - C2) * next_a0 ;
	float next_b2 = -(1.f - sqrt2C + C2) * next_a0;

	float a0_slope = (next_a0 - a0) * mFilterSlope;
	float b1_slope = (next_b1 - b1) * mFilterSlope;
	float b2_slope = (next_b2 - b2) * mFilterSlope;
	
	for (int i = 0; i!= mFilterLoops;++i)
	{
	    y0 = ZXP(nin) + b1 * y1 + b2 * y2; 
	    ZXP(nout) = a0 * (y0 - 2.f * y1 + y2);
	    
	    y2 = ZXP(nin) + b1 * y0 + b2 * y1; 
	    ZXP(nout) = a0 * (y2 - 2.f * y0 + y1);
	    
	    y1 = ZXP(nin) + b1 * y2 + b2 * y0; 
	    ZXP(nout) = a0 * (y1 - 2.f * y2 + y0);
	    
	    a0 += a0_slope; 
	    b1 += b1_slope; 
	    b2 += b2_slope;
	}
	
	for (int i = 0; i!= mFilterRemain;++i)
	{
	    y0 = ZXP(nin) + b1 * y1 + b2 * y2; 
	    ZXP(nout) = a0 * (y0 - 2.f * y1 + y2);
	    y2 = y1; 
	    y1 = y0;
	}
	
	m_a0 = a0;
	m_b1 = b1;
	m_b2 = b2;
	changed = false;
    }
    else
    {
	for (int i = 0; i!= mFilterLoops;++i)
	{
	    y0 = ZXP(nin) + b1 * y1 + b2 * y2; 
	    ZXP(nout) = a0 * (y0 - 2.f * y1 + y2);
	    
	    y2 = ZXP(nin) + b1 * y0 + b2 * y1; 
	    ZXP(nout) = a0 * (y2 - 2.f * y0 + y1);
			
	    y1 = ZXP(nin) + b1 * y2 + b2 * y0; 
	    ZXP(nout) = a0 * (y1 - 2.f * y2 + y0);
	}

	for (int i = 0; i!= mFilterRemain;++i)
	{
	    y0 = ZXP(nin) + b1 * y1 + b2 * y2; 
	    ZXP(nout) = a0 * (y0 - 2.f * y1 + y2);
	    y2 = y1; 
	    y1 = y0;
	}
    }
    m_y1 = zapgremlins(y1);
    m_y2 = zapgremlins(y2);
}

/* no control rate HPF filter */
