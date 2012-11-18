/* sc4pd 
   LFDNoise2~

   Copyright (c) 2004 Tim Blechmann.

   This code is derived from:
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

 *
 *  DynNoiseUGens.cpp
 *  xSC3plugins
 *
 *  Created by Alberto de Campo, Sekhar Ramacrishnan, Julian Rohrhuber on Sun May 30 2004.
 *  Copyright (c) 2004 HfbK. All rights reserved.
 *
 *

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
     
   Coded while listening to: Phil Minton & Veryan Weston: Ways Past
   
*/

#include "sc4pd.hpp"


/* ------------------------ LFDNoise2~ -------------------------------*/

class LFDNoise2_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(LFDNoise2_ar,sc4pd_dsp);
    
public:
    LFDNoise2_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);

    void m_seed(int i)
    {
	rgen.init(i);
    }

    void m_set(float f)
    {
	m_freq = f;
    }
    
private:
    RGen rgen;
    float m_freq;
    float m_levela, m_levelb, m_levelc, m_leveld;
    float m_phase;
    float m_smpdur;
    
    bool m_ar;

    FLEXT_CALLBACK_I(m_seed);
    FLEXT_CALLBACK_F(m_set);
};

FLEXT_LIB_DSP_V("LFDNoise2~",LFDNoise2_ar);

LFDNoise2_ar::LFDNoise2_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
    FLEXT_ADDMETHOD_(0,"set",m_set);

    //parse arguments
    AtomList Args(argc,argv);

    m_freq = sc_getfloatarg(Args,0);

    rgen.init(timeseed());

    RGET;

    m_phase=0.f;
    m_levela = frand2(s1, s2, s3) * 0.8f;// limits max interpol. overshoot to 1.
    m_levelb = frand2(s1, s2, s3) * 0.8f;
    m_levelc = frand2(s1, s2, s3) * 0.8f;
    m_leveld = frand2(s1, s2, s3) * 0.8f;
    
    RPUT;
    
    m_ar = sc_ar(Args);
    
    if (m_ar)
	AddInSignal("freqency");
    else
	AddInSignal("\"set\" frequency");
    AddOutSignal();
}    

void LFDNoise2_ar::m_dsp(int n, t_sample *const *in, 
			 t_sample *const *out)
{
    m_smpdur = sc_sampledur();
}


void LFDNoise2_ar::m_signal(int n, t_sample *const *in, 
			    t_sample *const *out)
{
    t_sample *nout = *out;

    float a = m_levela;
    float b = m_levelb;
    float c = m_levelc;
    float d = m_leveld;
    
    float phase = m_phase;
    
    RGET;

    if (m_ar)
    {
	t_sample *nin = *in;
	float smpdur = m_smpdur;
	for (int i = 0; i!= n; ++i)
	{
	    phase -= ZXP(nin) * smpdur;
	    if (phase <= 0) 
	    {			
		phase = sc_wrap(phase, 0.f, 1.f);
		a = b;
		b = c;
		c = d; 
		d = frand2(s1,s2,s3) * 0.8f;	// limits max interpol. overshoot to 1.

	    }
	    ZXP(nout) = cubicinterp(1.f - phase, a, b, c, d);
	}
    }
    else 
    {
	float dphase = m_smpdur * m_freq;
	for (int i = 0; i!= n; ++i)
	{
	    phase -= dphase;
	    if (phase <= 0) 
	    {
		phase = sc_wrap(phase, 0.f, 1.f);
		a = b;
		b = c;
		c = d; 
		d = frand2(s1,s2,s3) * 0.8f;	// limits max interpol. overshoot to 1.
	    }
	    ZXP(nout) = cubicinterp(1.f - phase, a, b, c, d);
	}
    }


    m_phase = phase;

    m_levela = a;
    m_levelb = b;
    m_levelc = c;
    m_leveld = d;
    
    RPUT;
}


