/* sc4pd 
   TwoPole~

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
     
   Coded while listening to: VA: Insight
   
*/

#include "sc4pd.hpp"

/* ------------------------ TwoPole~ -------------------------------*/

class TwoPole_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(TwoPole_ar,sc4pd_dsp);
    
public:
    TwoPole_ar(int argc, t_atom *argv);
    
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

    void m_set_radius(float f)
    {
	m_reson=f;
	changed = true;
    }
    
    
private:
    float m_b1, m_b2, m_y1, m_y2, m_freq, m_reson;
    bool changed;
    float mRadiansPerSample, mFilterSlope;
    int mFilterLoops, mFilterRemain; 
    
    FLEXT_CALLBACK_F(m_set_freq);
    FLEXT_CALLBACK_F(m_set_radius);
};

FLEXT_LIB_DSP_V("TwoPole~",TwoPole_ar);

TwoPole_ar::TwoPole_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"kfreq",m_set_freq);
    FLEXT_ADDMETHOD_(0,"kradius",m_set_radius);

    //parse arguments
    AtomList Args(argc,argv);
    
    m_freq = sc_getfloatarg(Args,0);
    m_reson = sc_getfloatarg(Args,1);
    changed = false;
    
    AddOutSignal();
    
    m_b1 = 0.f;
    m_b2 = 0.f;
    m_y1 = 0.f;
    m_y2 = 0.f;
}

void TwoPole_ar::m_signal(int n, t_sample *const *in, 
			  t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    
    float y0;
    float y1 = m_y1;
    float y2 = m_y2;

    if (changed)
    {
	float b1 = m_b1;
	float b2 = m_b2;
	float b1_next = 2.f * m_reson * cos(m_freq * mRadiansPerSample);
	float b2_next = -(m_reson * m_reson);
	float b1_slope = (b1_next - b1) * mFilterSlope;
	float b2_slope = (b2_next - b2) * mFilterSlope;
	
	for (int i = 0; i!= mFilterLoops;++i)
	{
	    ZXP(nout) = y0 = ZXP(nin) + b1 * y1 + b2 * y2;
	    ZXP(nout) = y2 = ZXP(nin) + b1 * y0 + b2 * y1;
	    ZXP(nout) = y1 = ZXP(nin) + b1 * y2 + b2 * y0;
	    
	    b1 += b1_slope; 
	    b2 += b2_slope;
	}
	
	for (int i = 0; i!= mFilterRemain;++i)
	{
	    ZXP(nout) = y0 = ZXP(nin) + b1 * y1 + b2 * y2;
	    y2 = y1; 
	    y1 = y0;
	}

	m_b1 = b1;
	m_b2 = b2;
	changed = false;
    }
    else
    {
	float b1 = m_b1;
	float b2 = m_b2;

	for (int i = 0; i!= mFilterLoops;++i)
	{
	    ZXP(nout) = y0 = ZXP(nin) + b1 * y1 + b2 * y2;
	    ZXP(nout) = y2 = ZXP(nin) + b1 * y0 + b2 * y1;
	    ZXP(nout) = y1 = ZXP(nin) + b1 * y2 + b2 * y0;
	}
	for (int i = 0; i!= mFilterRemain;++i)
	{
	    ZXP(nout) = y0 = ZXP(nin) + b1 * y1 + b2 * y2;
	    y2 = y1; 
	    y1 = y0;
	}
    }
    m_y1 = zapgremlins(y1);
    m_y2 = zapgremlins(y2);
}

/* no control rate two pole filter */
