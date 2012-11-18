/* sc4pd 
   Resonz~, Resonz

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
     
   Coded while listening to: MIMEO: Electric Chair And Table
   
*/

#include "sc4pd.hpp"

/* ------------------------ Resonz~ -----------------------------*/

class Resonz_ar
    :public sc4pd_dsp
{
    FLEXT_HEADER(Resonz_ar,sc4pd_dsp);

public:
    Resonz_ar(int argc,t_atom * argv);

protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);
    
    void m_set_freq(float f)
    {
	m_freq = f;
	m_ffreq = m_freq * mRadiansPerSample;
	changed = true;
    }
    void m_set_rq(float f)
    {
	m_rq = f;
	changed = true;
    }

private:
    FLEXT_CALLBACK_F(m_set_freq);
    FLEXT_CALLBACK_F(m_set_rq);
    float m_y1, m_y2, m_a0, m_b1, m_b2, m_freq, m_rq, m_ffreq;
    bool changed;
    
    float mRadiansPerSample, mFilterSlope, mFilterLoops, mFilterRemain;
};

FLEXT_LIB_DSP_V("Resonz~",Resonz_ar);

Resonz_ar::Resonz_ar(int argc,t_atom * argv)
{
    FLEXT_ADDMETHOD_(0,"freq",m_set_freq);
    FLEXT_ADDMETHOD_(0,"rq",m_set_rq);

    AtomList Args(argc,argv);
    m_freq = sc_getfloatarg(Args,0);
    m_rq = sc_getfloatarg(Args,1);

    AddOutSignal();

    m_a0 = 0.f;
    m_b1 = 0.f;
    m_b2 = 0.f;
    m_y1 = 0.f;
    m_y2 = 0.f;
    changed = false;
}

void Resonz_ar::m_dsp(int n, t_sample *const *in, 
				t_sample *const *out)
{
    mRadiansPerSample = sc_radianspersample();
    mFilterSlope = sc_filterslope();
    mFilterLoops = sc_filterloops();
    mFilterRemain = sc_filterremain();
    
    m_ffreq = m_freq * mRadiansPerSample;
}

void Resonz_ar::m_signal(int n, t_sample *const *in, 
			 t_sample *const *out)
{
    t_sample *nout = *out;
    t_sample *nin = *in;
    
    float y0;
    float y1 = m_y1;
    float y2 = m_y2;
    float a0 = m_a0;
    float b1 = m_b1;
    float b2 = m_b2;

    if (changed = true)
    {
	float B = m_ffreq * m_rq;
	float R = 1.f - B * 0.5f;
	float twoR = 2.f * R;
	float R2 = R * R;
	float cost = (twoR * cos(m_ffreq)) / (1.f + R2);
	float b1_next = twoR * cost;
	float b2_next = -R2;
	float a0_next = (1.f - R2) * 0.5f;
	float a0_slope = (a0_next - a0) * mFilterSlope;
	float b1_slope = (b1_next - b1) * mFilterSlope;
	float b2_slope = (b2_next - b2) * mFilterSlope;

	for (int i = 0; i!= mFilterLoops;++i)
	{
	    y0 = ZXP(nin) + b1 * y1 + b2 * y2; 
	    ZXP(nout) = a0 * (y0 - y2);
	    
	    y2 = ZXP(nin) + b1 * y0 + b2 * y1; 
	    ZXP(nout) = a0 * (y2 - y1);
	    
	    y1 = ZXP(nin) + b1 * y2 + b2 * y0; 
	    ZXP(nout) = a0 * (y1 - y0);
	    
	    a0 += a0_slope; 
	    b1 += b1_slope; 
	    b2 += b2_slope;
	}

	for (int i = 0; i!= mFilterRemain;++i)
	{
	    y0 = ZXP(nin) + b1 * y1 + b2 * y2; 
	    ZXP(nout) = a0 * (y0 - y2);
	    y2 = y1; 
	    y1 = y0;
	}

	m_a0 = a0_next;
	m_b1 = b1_next;
	m_b2 = b2_next;
	changed = false;
    }
    else
    {
	for (int i = 0; i!= mFilterLoops;++i)
	{
	    y0 = ZXP(nin) + b1 * y1 + b2 * y2; 
	    ZXP(nout) = a0 * (y0 - y2);
	    
	    y2 = ZXP(nin) + b1 * y0 + b2 * y1; 
	    ZXP(nout) = a0 * (y2 - y1);
	    
	    y1 = ZXP(nin) + b1 * y2 + b2 * y0; 
	    ZXP(nout) = a0 * (y1 - y0);
	}
	
	for (int i = 0; i!= mFilterRemain;++i)
	{
	    y0 = ZXP(nin) + b1 * y1 + b2 * y2; 
	    ZXP(nout) = a0 * (y0 - y2);
	    y2 = y1; 
	    y1 = y0;
	}
    }
    m_y1 = zapgremlins(y1);
    m_y2 = zapgremlins(y2);
}


/* no control rate resonz */
