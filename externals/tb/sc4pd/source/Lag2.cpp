/* sc4pd 
   Lag2~, Lag2

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
     
   Coded while listening to: DKV Trio: Baraka
   
*/

#include "sc4pd.hpp"

/* ------------------------ Lag2~ -----------------------------*/

class Lag2_ar
    :public sc4pd_dsp
{
    FLEXT_HEADER(Lag2_ar,sc4pd_dsp);

public:
    Lag2_ar(int argc,t_atom * argv);

protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);
    void m_set(float f);

private:
    FLEXT_CALLBACK_F(m_set);
    float m_b1, n_b1;
    float m_y1a, m_y1b;
    float m_lag;
    bool changed;
};

FLEXT_LIB_DSP_V("Lag2~",Lag2_ar);

Lag2_ar::Lag2_ar(int argc,t_atom * argv)
{
    FLEXT_ADDMETHOD_(0,"lagTime",m_set);

    AtomList Args(argc,argv);

    m_lag = sc_getfloatarg(Args,0);

    AddOutSignal();

    m_b1 = 0.f;
    m_y1a = 0.f;
    m_y1b = 0.f;
}

void Lag2_ar::m_dsp(int n, t_sample *const *in, 
		   t_sample *const *out)
{
    changed = false;
    m_b1 = m_lag == 0.f ? 0.f : exp(log001 / (m_lag * Samplerate()));
}

void Lag2_ar::m_signal(int n, t_sample *const *in, 
		      t_sample *const *out)
{
    t_sample *nout = *out;
    t_sample *nin = *in;

    float y1a = m_y1a;
    float y1b = m_y1b;
    float b1 = m_b1;

    if (changed)
    {
	float b1_slope = CALCSLOPE(m_b1, n_b1);
	m_b1 = n_b1;
	for (int i = 0; i!= n;++i)
	{
	    float y0a = ZXP(nin); 
	    y1a = y0a + b1 * (y1a - y0a);
	    y1b = y1a + b1 * (y1b - y1a);
	    ZXP(nout) = y1b;
	    b1 += b1_slope;
	}
	changed = false;
    }
    else
    {
	for (int i = 0; i!= n;++i)
	{
	    float y0a = ZXP(nin); 
	    y1a = y0a + b1 * (y1a - y0a);
	    y1b = y1a + b1 * (y1b - y1a);
	    ZXP(nout) = y1b;
	}
    }
    m_y1a = zapgremlins(y1a);
    m_y1b = zapgremlins(y1b);
}
    
void Lag2_ar::m_set(float f)
{
    m_lag = f;
    n_b1 = m_lag == 0.f ? 0.f : exp(log001 / (m_lag * Samplerate()));
    changed = true;
}

/* todo: does a control rate Lag2 make sense? */
