/* sc4pd 
   Decay2~, Decay2

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
     
   Coded while listening to: AMM: AMMMusic 1966
   
*/

#include "sc4pd.hpp"


/* ------------------------ Decay2~ -----------------------------*/

class Decay2_ar
    :public sc4pd_dsp
{
    FLEXT_HEADER(Decay2_ar,sc4pd_dsp);

public:
    Decay2_ar(int argc,t_atom * argv);

protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);
    void m_attack(float f);
    void m_decay(float f);

private:
    FLEXT_CALLBACK_F(m_decay);
    FLEXT_CALLBACK_F(m_attack);
    float m_attackTime, m_y1a, m_b1a, n_b1a;
    float m_decayTime, m_y1b, m_b1b, n_b1b;
    bool changed;
};

FLEXT_LIB_DSP_V("Decay2~",Decay2_ar);

Decay2_ar::Decay2_ar(int argc,t_atom * argv)
{
    FLEXT_ADDMETHOD_(0,"decayTime",m_decay);
    FLEXT_ADDMETHOD_(0,"attackTime",m_attack);

    AtomList Args(argc,argv);

    m_decayTime = sc_getfloatarg(Args,1); //decay
    m_attackTime = sc_getfloatarg(Args,0);//attack
    
    AddOutSignal();

    m_y1a = m_y1b = 0.f; //different than in sc

}

void Decay2_ar::m_signal(int n, t_sample *const *in, 
				t_sample *const *out)
{
    t_sample *nout = *out;
    t_sample *nin = *in;

    float y1a = m_y1a;
    float y1b = m_y1b;
    float b1a = m_b1a;
    float b1b = m_b1b;
    
    if(changed)
    {
	float b1a_slope = CALCSLOPE(n_b1a, b1a);
	float b1b_slope = CALCSLOPE(n_b1b, b1b);
	m_b1a = n_b1a;
	m_b1b = n_b1b;

	for (int i = 0; i!= n;++i)
	{
	    float y0 = ZXP(nin); 
	    y1a = y0 + b1a * y1a;
	    y1b = y0 + b1b * y1b;
	    
	    ZXP (nout) = y1a - y1b;
	    b1a += b1a_slope;
	    b1b += b1b_slope;
	}
	changed = false;
    }
    else
    {
	for (int i = 0; i!= n;++i)
	{
	    float y0 = ZXP(nin); 
	    y1a = y0 + b1a * y1a;
	    y1b = y0 + b1b * y1b;
	    
	    ZXP (nout) = y1a - y1b;
	}
    }

    m_y1a = zapgremlins(y1a);
    m_y1b = zapgremlins(y1b);
}
    
void Decay2_ar::m_decay(float f)
{
    m_decayTime = f;
    n_b1a = f == 0.f ? 0.f : exp(log001 / (f * Samplerate()));
    changed = true;
}

void Decay2_ar::m_attack(float f)
{
    m_attackTime = f;
    n_b1b = f == 0.f ? 0.f : exp(log001 / (f * Samplerate()));
    changed = true;
}

void Decay2_ar::m_dsp(int n, t_sample *const *in, t_sample *const *out)
{
    m_b1a = m_decayTime == 0.f ? 0.f : exp(log001 / 
					   (m_decayTime * Samplerate()));
    m_b1b = m_attackTime == 0.f ? 0.f : exp(log001 / 
					    (m_attackTime * Samplerate()));
    changed = false;
}

/* todo: does it make sense to implement a message-based Decay2? 
   Probably not... */

