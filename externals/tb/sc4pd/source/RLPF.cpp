/* sc4pd 
   RLPF~

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

/* ------------------------ RLPF~ -------------------------------*/

class RLPF_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(RLPF_ar,sc4pd_dsp);
    
public:
    RLPF_ar(int argc, t_atom *argv);
    
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

    void m_set_rq(float f)
    {
	m_reson=f;
	changed = true;
    }
    
    
private:
    float m_y1, m_y2, m_a0, m_b1, m_b2, m_freq, m_reson;
    bool changed;
    float mRadiansPerSample, mFilterSlope;
    int mFilterLoops, mFilterRemain; 
    
    FLEXT_CALLBACK_F(m_set_freq);
    FLEXT_CALLBACK_F(m_set_rq);
};

FLEXT_LIB_DSP_V("RLPF~",RLPF_ar);

RLPF_ar::RLPF_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"kfreq",m_set_freq);
    FLEXT_ADDMETHOD_(0,"rq",m_set_rq);

    //parse arguments
    AtomList Args(argc,argv);
    
    m_freq = sc_getfloatarg(Args,0);
    m_reson = sc_getfloatarg(Args,1);
    changed = true;
    
    AddOutSignal();

    m_a0 = 0.f;
    m_b1 = 0.f;
    m_b2 = 0.f;
    m_y1 = 0.f;
    m_y2 = 0.f;
}

void RLPF_ar::m_signal(int n, t_sample *const *in, 
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
	float qres = sc_max(0.001, m_reson);
	float pfreq = m_freq * mRadiansPerSample;
		
	float D = tan(pfreq * qres * 0.5);
	float C = ((1.f-D)/(1.f+D));
	float cosf = cos(pfreq);
	
	float next_b1 = (1.f + C) * cosf;	
	float next_b2 = -C;
	float next_a0 = (1.f + C - next_b1) * .25;

	float a0_slope = (next_a0 - a0) * mFilterSlope;
	float b1_slope = (next_b1 - b1) * mFilterSlope;
	float b2_slope = (next_b2 - b2) * mFilterSlope;
	
	for (int i = 0; i!= mFilterLoops;++i)
	{
	    y0 = a0 * ZXP(nin) + b1 * y1 + b2 * y2;
	    ZXP(nout) = y0 + 2.f * y1 + y2;
	    
	    y2 = a0 * ZXP(nin) + b1 * y0 + b2 * y1;
	    ZXP(nout) = y2 + 2.f * y0 + y1;
	    
	    y1 = a0 * ZXP(nin) + b1 * y2 + b2 * y0;
	    ZXP(nout) = y1 + 2.f * y2 + y0;
	    
	    a0 += a0_slope; 
	    b1 += b1_slope; 
	    b2 += b2_slope;
	}
	
	for (int i = 0; i!= mFilterRemain;++i)
	{
	    y0 = a0 * ZXP(nin) + b1 * y1 + b2 * y2;
	    ZXP(nout) = y0 + 2.f * y1 + y2;
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
	    y0 = a0 * ZXP(nin) + b1 * y1 + b2 * y2;
	    ZXP(nout) = y0 + 2.f * y1 + y2;
	    
	    y2 = a0 * ZXP(nin) + b1 * y0 + b2 * y1;
	    ZXP(nout) = y2 + 2.f * y0 + y1;
	    
	    y1 = a0 * ZXP(nin) + b1 * y2 + b2 * y0;
	    ZXP(nout) = y1 + 2.f * y2 + y0;
	}

	for (int i = 0; i!= mFilterRemain;++i)
	{
	    y0 = a0 * ZXP(nin) + b1 * y1 + b2 * y2;
	    ZXP(nout) = y0 + 2.f * y1 + y2;
	    y2 = y1;
	    y1 = y0;
	}
    }
    m_y1 = zapgremlins(y1);
    m_y2 = zapgremlins(y2);
}

/* no control rate RLPF filter */
