/* sc4pd 
   LFNoise2, LFNoise2~

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
     
   Coded while listening to: Guenther Mueller & Toshimaru Nakamura: Tint
   
*/

#include "sc4pd.hpp"


/* ------------------------ LFNoise2~ -------------------------------*/

class LFNoise2_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(LFNoise2_ar,sc4pd_dsp);
    
public:
    LFNoise2_ar(int argc, t_atom *argv);
    
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
    float m_slope;
    float m_curve;
    int m_counter;
    int m_sr;
    float m_nextvalue;
    float m_nextmidpt;

    FLEXT_CALLBACK_I(m_seed);
    FLEXT_CALLBACK_F(m_set);
};

FLEXT_LIB_DSP_V("LFNoise2~",LFNoise2_ar);

LFNoise2_ar::LFNoise2_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
    FLEXT_ADDMETHOD_(0,"set",m_set);

    //parse arguments
    AtomList Args(argc,argv);

    m_freq = sc_getfloatarg(Args,0);
    
    rgen.init(timeseed());

    m_counter=0;
    m_level=rgen.frand2();
    m_slope=0;
    m_curve=0;
    m_nextvalue=0;
    m_nextmidpt=0;

    AddOutSignal();
}    

void LFNoise2_ar::m_dsp(int n, t_sample *const *in, 
			   t_sample *const *out)
{
    m_sr = Samplerate();
}


void LFNoise2_ar::m_signal(int n, t_sample *const *in, 
			       t_sample *const *out)
{
    t_sample *nout = *out;

    float level = m_level;
    int32 counter = m_counter;
    float slope = m_slope;
    float curve = m_curve;
    
    RGET;

    int remain = n;
    do
    {
	if (counter<=0) 
	{
	    float value = m_nextvalue;
	    m_nextvalue = frand2(s1,s2,s3);
	    level = m_nextmidpt;
	    m_nextmidpt = (m_nextvalue + value) * .5;
	    
	    counter = (int32)(m_sr / sc_max(m_freq, .001f));
	    counter = sc_max(2, counter);
	    float fseglen = (float)counter;
	    curve = 2.f * (m_nextmidpt - level - fseglen * slope) 
		/ (fseglen * fseglen + fseglen);
	}
	int nsmps = sc_min(remain, counter);
	remain -= nsmps;
	counter -= nsmps;
	
	for (int i = 0; i!= nsmps;++i)
	{
	    (*(nout)++)=level;
	    slope+=curve;
	    level+=slope;
	}
    }
    while(remain);

    m_level = level;
    m_counter = counter;
    m_slope = slope;
    m_curve = curve;
    
    RPUT;
}


/* ------------------------ LFNoise2 ---------------------------------*/

class LFNoise2_kr:
    public flext_base
{
    FLEXT_HEADER(LFNoise2_kr,flext_base);

public:
    LFNoise2_kr(int argc, t_atom *argv);
    
protected:
    void m_perform(void*);

    void m_seed(int i)
    {
	rgen.init(i);
    }
 
    void m_set(float);
   
private:
    RGen rgen;
    float m_level;
    float m_slope;
    float m_curve;
    float m_nextvalue;
    float m_nextmidpt;
    
    float dt;                           //in s
    float tick;                        //in s
    int counter;

    Timer m_timer;
    FLEXT_CALLBACK_I(m_seed);
    FLEXT_CALLBACK_T(m_perform);
    FLEXT_CALLBACK_F(m_set);
};

FLEXT_LIB_V("LFNoise2",LFNoise2_kr);

LFNoise2_kr::LFNoise2_kr(int argc, t_atom *argv)
    : tick(0.01)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
    FLEXT_ADDMETHOD_(0,"set",m_set);
    FLEXT_ADDTIMER(m_timer,m_perform);
    
    //parse arguments
    AtomList Args(argc,argv);
    
    rgen.init(timeseed());

    m_level=rgen.frand2();

    AddOutFloat();

    m_set(sc_getfloatarg(Args,0));
}


void LFNoise2_kr::m_set(float f)
{
    dt = sc_max(1/f, .001f);
    counter = (dt/tick);
    counter = sc_max(2, counter);
    
    float value = m_nextvalue;
    m_nextvalue = rgen.frand2();
    m_level = m_nextmidpt;
    m_nextmidpt = (m_nextvalue + value) * .5;
    
    float fseglen = (float)counter;
    m_curve = 2.f * (m_nextmidpt - m_level - fseglen * m_slope) 
	/ (fseglen * fseglen + fseglen);
    
    
    m_timer.Reset();
    m_timer.Delay(tick);
}

void LFNoise2_kr::m_perform(void*)
{
    m_slope+=m_curve;
    m_level+=m_slope;
    ToOutFloat(0,m_level);
    if (--counter)
    {
	m_timer.Reset();
	m_timer.Delay(tick);
    }
    else
    {
	counter = (dt/tick);

	counter = sc_max(2, counter);
	
	float value = m_nextvalue;
	m_nextvalue = rgen.frand2();
	m_level = m_nextmidpt;
	m_nextmidpt = (m_nextvalue + value) * .5;
	
	float fseglen = (float)counter;
	m_curve = 2.f * (m_nextmidpt - m_level - fseglen * m_slope) / 
	    (fseglen * fseglen + fseglen);
	
	m_timer.Reset();
	m_timer.Delay(tick);
	
    }
}

