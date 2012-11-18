/* sc4pd 
   LFPulse, LFPulse~

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
     
   Coded while listening to: Keith Rowe & Oren Ambarchi: Flypaper
   
*/

#include "sc4pd.hpp"


/* ------------------------ LFPulse~ -------------------------------*/

class LFPulse_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(LFPulse_ar,sc4pd_dsp);
    
public:
    LFPulse_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out)
    {
	m_signal_fun(n,in,out);
    }
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);

    void m_set(float f)
    {
	m_freq=f;
    }
    
    void m_ar()
    {
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_ar));
    }

    void m_kr()
    {
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_kr));
    }
    
    void m_width (float f)
    {
	nextDuty = f;
    }
    

private:
    double mPhase;
    float mFreqMul;
    float m_freq;      //for kr arguments
    float mDuty;
    float nextDuty;

    DEFSIGCALL (m_signal_fun);
    DEFSIGFUN (m_signal_ar);
    DEFSIGFUN (m_signal_kr);
    
    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK_F(m_width);
    FLEXT_CALLBACK(m_ar);
    FLEXT_CALLBACK(m_kr);
};

FLEXT_LIB_DSP_V("LFPulse~",LFPulse_ar);

LFPulse_ar::LFPulse_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"freq",m_set);
    FLEXT_ADDMETHOD_(0,"width",m_width);
    FLEXT_ADDMETHOD_(0,"ar",m_ar);
    FLEXT_ADDMETHOD_(0,"kr",m_kr);

    //parse arguments
    AtomList Args(argc,argv);
    
    m_freq = sc_getfloatarg(Args,0);
    
    nextDuty = sc_getfloatarg(Args,1);

    if(sc_ar(Args))
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_ar));
    else // if not given, use control rate
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_kr)); 
    
    AddOutSignal();
}    

void LFPulse_ar::m_dsp(int n, t_sample *const *in, t_sample *const *out)
{
    mFreqMul = 1 / Samplerate();
    
}

void LFPulse_ar::m_signal_ar(int n, t_sample *const *in, 
	 		   t_sample *const *out)
{
    t_sample *freq = *in;
    t_sample *xout = *out;
    
    float freqmul = mFreqMul;
    double phase = mPhase;
    float duty = mDuty;

    for (int i = 0; i!= n;++i)
    {
	float z;
	if (phase >= 1.f) 
	{
	    phase -= 1.f;
	    duty = mDuty = nextDuty;
	    // output at least one sample from the opposite polarity
	    z = duty < 0.5 ? 1.f : 0.f;
	}
	else 
	{
	    z = phase < duty ? 1.f : 0.f;
	}
	
	phase += (*(freq)++) * freqmul;
	(*(xout)++) = z;
    }

    mPhase=phase;
}


void LFPulse_ar::m_signal_kr(int n, t_sample *const *in, 
			   t_sample *const *out)
{
    t_sample *xout = *out;
    
    double phase = mPhase;
    float duty = mDuty;
    float freq = m_freq * mFreqMul;
    
    for (int i = 0; i!= n;++i)
    {
	float z;
	if (phase >= 1.f) 
	{
	    phase -= 1.f;
	    duty = mDuty = nextDuty;
	    // output at least one sample from the opposite polarity
	    z = duty < 0.5 ? 1.f : 0.f;
	}
	else 
	{
	    z = phase < duty ? 1.f : 0.f;
	}
	
	phase += freq;
	(*(xout)++) = z;
    }
    mPhase=phase;
}

/* ------------------------ LFPulse ---------------------------------*/

/* todo: remove obsolete messages */

class LFPulse_kr:
    public flext_base
{
    FLEXT_HEADER(LFPulse_kr,flext_base);

public:
    LFPulse_kr(int argc, t_atom *argv);
    
protected:
    void m_perform(void*);

    void m_set(float f)
    {
	m_freq_set = f;
	m_freq = f * mFreqMul;
    }

    void m_set_kr(float f)
    {
	if (f != 0)
	{
	    dt = f * 0.001;
 	    mFreqMul = dt;
	    m_freq = m_freq_set * mFreqMul;
	    m_timer.Reset();
	    m_timer.Periodic(dt);
	}
    }

    void m_set_width(float f)
    {
	nextDuty=f;
    }

private:
    double mPhase;
    float mFreqMul;
    float mDuty;
    float nextDuty;
    float m_freq;
    float dt;
    float m_freq_set;

    Timer m_timer;

    FLEXT_CALLBACK_F(m_set_kr);
    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK_F(m_set_width);
    FLEXT_CALLBACK_T(m_perform);
};


FLEXT_LIB_V("LFPulse",LFPulse_kr);

LFPulse_kr::LFPulse_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD(0,m_set);
    FLEXT_ADDMETHOD_(0,"kr",m_set_kr);
    FLEXT_ADDMETHOD_(0,"width",m_set_width);

    FLEXT_ADDTIMER(m_timer,m_perform);

    AddOutFloat();

    //parse arguments
    AtomList Args(argc,argv);

    m_freq_set = sc_getfloatarg(Args,0);
    
    nextDuty = sc_getfloatarg(Args,1);
    
    dt = sc_getfloatarg(Args,2) * 0.001;
    
    if (dt == 0)
	dt = 0.02;  // 20 ms as default control rate as in line
    mFreqMul = dt; 
    
    m_freq = m_freq_set * mFreqMul;
    
    m_timer.Periodic(dt);
    
}

void LFPulse_kr::m_perform(void*)
{
    float z;
    if (mPhase >= 1.f) 
    {
	mPhase -= 1.f;
	mDuty = nextDuty;
	// output at least one sample from the opposite polarity
	z = mDuty < 0.5 ? 1.f : 0.f;
    } 
    else 
    {
	z = mPhase < mDuty ? 1.f : 0.f;
    }
    mPhase += m_freq;
    ToOutFloat(0,z);
}
