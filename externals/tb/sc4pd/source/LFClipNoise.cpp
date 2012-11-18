/* sc4pd 
   LFClipNoise, LFClipNoise~

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
     
   Coded while listening to: Elliott Sharp: Revenge Of The Stuttering Child
   
*/

#include "sc4pd.hpp"


/* ------------------------ LFClipNoise~ -------------------------------*/

class LFClipNoise_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(LFClipNoise_ar,sc4pd_dsp);
    
public:
    LFClipNoise_ar(int argc, t_atom *argv);
    
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
    int m_counter;
    int m_sr;
    FLEXT_CALLBACK_I(m_seed);
    FLEXT_CALLBACK_F(m_set);
};

FLEXT_LIB_DSP_V("LFClipNoise~",LFClipNoise_ar);

LFClipNoise_ar::LFClipNoise_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
    FLEXT_ADDMETHOD_(0,"set",m_set);

    //parse arguments
    AtomList Args(argc,argv);

    m_freq = sc_getfloatarg(Args,0);
    
    m_counter=0;
    m_level=0;

    rgen.init(timeseed());

    AddOutSignal();
}    

void LFClipNoise_ar::m_dsp(int n, t_sample *const *in, 
			   t_sample *const *out)
{
    m_sr = Samplerate();
}


void LFClipNoise_ar::m_signal(int n, t_sample *const *in, 
			       t_sample *const *out)
{
    t_sample *nout = *out;

    float level = m_level;
    int32 counter = m_counter;

    RGET;

    int remain = n;
    do
    {
	if (counter<=0) 
	{
	    counter = (int)(m_sr / sc_max(m_freq, .001f));
	    counter = sc_max(1, counter);
	    level = fcoin(s1,s2,s3);
	}
	int nsmps = sc_min(remain, counter);
	remain -= nsmps;
	counter -= nsmps;
	
	for (int i = 0; i!= nsmps;++i)
	{
	    (*(nout)++)=level;
	}
    }
    while(remain);

    m_level = level;
    m_counter = counter;
    
    RPUT;
}


/* ------------------------ LFClipNoise ---------------------------------*/

class LFClipNoise_kr:
    public flext_base
{
    FLEXT_HEADER(LFClipNoise_kr,flext_base);

public:
    LFClipNoise_kr(int argc, t_atom *argv);
    
protected:
    void m_perform(void*);
    
    void m_seed(int i)
    {
	rgen.init(i);
    }
 
    void m_set(float f)
    {
	double dt = sc_max(1/f, .001f);
	m_timer.Reset();
	m_timer.Periodic(dt);
    }
   
private:
    RGen rgen;
    Timer m_timer;
    FLEXT_CALLBACK_I(m_seed);
    FLEXT_CALLBACK_T(m_perform);
    FLEXT_CALLBACK_F(m_set);
};

FLEXT_LIB_V("LFClipNoise",LFClipNoise_kr);

LFClipNoise_kr::LFClipNoise_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
    FLEXT_ADDMETHOD_(0,"set",m_set);
    FLEXT_ADDTIMER(m_timer,m_perform);
    
    //parse arguments
    AtomList Args(argc,argv);
    
    double dt = sc_max(1/sc_getfloatarg(Args,0), .001f);
    
    rgen.init(timeseed());

    m_timer.Periodic(dt);

    AddOutFloat();
}

void LFClipNoise_kr::m_perform(void*)
{
    ToOutFloat(0,rgen.fcoin());
}
