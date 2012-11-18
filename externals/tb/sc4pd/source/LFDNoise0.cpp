/* sc4pd 
   LFDNoise0~

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


/* ------------------------ LFDNoise0~ -------------------------------*/

class LFDNoise0_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(LFDNoise0_ar,sc4pd_dsp);
    
public:
    LFDNoise0_ar(int argc, t_atom *argv);
    
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
    float m_level;
    float m_phase;
    float m_smpdur;
    
    bool m_ar;

    FLEXT_CALLBACK_I(m_seed);
    FLEXT_CALLBACK_F(m_set);
};

FLEXT_LIB_DSP_V("LFDNoise0~",LFDNoise0_ar);

LFDNoise0_ar::LFDNoise0_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
    FLEXT_ADDMETHOD_(0,"set",m_set);

    //parse arguments
    AtomList Args(argc,argv);

    m_freq = sc_getfloatarg(Args,0);

    m_phase=0.f;
    m_level=0.f;

    rgen.init(timeseed());
    
    m_ar = sc_ar(Args);
    
    if (m_ar)
	AddInSignal("freqency");
    else
	AddInSignal("\"set\" frequency");
    AddOutSignal();
}    

void LFDNoise0_ar::m_dsp(int n, t_sample *const *in, 
			 t_sample *const *out)
{
    m_smpdur = sc_sampledur();
}


void LFDNoise0_ar::m_signal(int n, t_sample *const *in, 
			    t_sample *const *out)
{
    t_sample *nout = *out;

    float level = m_level;
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
		level = frand2(s1,s2,s3);
	    }
	    ZXP(nout) = level;
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
		level = frand2(s1,s2,s3);
	    }
	    ZXP(nout) = level;
	}
    }


    m_level = level;
    m_phase = phase;
    
    RPUT;
}


