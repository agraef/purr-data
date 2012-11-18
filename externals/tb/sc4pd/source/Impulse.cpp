/* sc4pd 
   Impulse, Impulse~

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

/* todo: implement phase offset as in sc3
/* ------------------------ Impulse~ -------------------------------*/

class Impulse_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(Impulse_ar,sc4pd_dsp);
    
public:
    Impulse_ar(int argc, t_atom *argv);
    
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
    
private:
    double mPhase, mPhaseOffset;
    float mFreqMul;
    float m_freq;      //for kr arguments

    DEFSIGCALL (m_signal_fun);
    DEFSIGFUN (m_signal_ar);
    DEFSIGFUN (m_signal_kr);
    
    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK(m_ar);
    FLEXT_CALLBACK(m_kr);
};

FLEXT_LIB_DSP_V("Impulse~",Impulse_ar);

Impulse_ar::Impulse_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"freq",m_set);
    FLEXT_ADDMETHOD_(0,"ar",m_ar);
    FLEXT_ADDMETHOD_(0,"kr",m_kr);

    //parse arguments
    AtomList Args(argc,argv);
    
    m_freq = sc_getfloatarg(Args,0);
    
    if(sc_ar(Args))
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_ar));
    else // if not given, use control rate
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_kr)); 
    
    AddOutSignal();
}    

void Impulse_ar::m_dsp(int n, t_sample *const *in, t_sample *const *out)
{
    mFreqMul = 1 / Samplerate();
    
}

void Impulse_ar::m_signal_ar(int n, t_sample *const *in, 
	 		   t_sample *const *out)
{
    t_sample *freq = *in;
    t_sample *xout = *out;
    
    float freqmul = mFreqMul;
    double phase = mPhase;
    
    for (int i = 0; i!= n;++i)
    {
	float z;
	if (phase >= 1.f) 
	{
	    phase -= 1.f;
	    z = 1.f;
	} 
	else 
	{
	    z = 0.f;
	}
	phase += (*(freq)++) * freqmul;
	(*(xout)++) = z;
    }

    mPhase=phase;
}


void Impulse_ar::m_signal_kr(int n, t_sample *const *in, 
			   t_sample *const *out)
{
    t_sample *xout = *out;
    
    float freq = m_freq * mFreqMul;
    float freqmul = mFreqMul;
    double phase = mPhase;
    
    for (int i = 0; i!= n;++i)
    {
	float z;
	if (phase >= 1.f) 
	{
	    phase -= 1.f;
	    z = 1.f;
	} 
	else 
	{
	    z = 0.f;
	}
	phase += freq;
	(*(xout)++) = z;
    }

    mPhase=phase;
}

/* ------------------------ Impulse ---------------------------------*/

/* todo: remove obsolete messages */

class Impulse_kr:
    public flext_base
{
    FLEXT_HEADER(Impulse_kr,flext_base);

public:
    Impulse_kr(int argc, t_atom *argv);
    
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

private:

    double mPhase, mPhaseOffset;
    float mFreqMul;
    float m_freq;      //for kr arguments

    float dt;
    float m_freq_set;

    Timer m_timer;

    FLEXT_CALLBACK_F(m_set_kr);
    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK_T(m_perform);
};


FLEXT_LIB_V("Impulse",Impulse_kr);

Impulse_kr::Impulse_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD(0,m_set);
    FLEXT_ADDMETHOD_(0,"kr",m_set_kr);

    FLEXT_ADDTIMER(m_timer,m_perform);

    AddOutFloat();

    //parse arguments
    AtomList Args(argc,argv);

    m_freq_set = sc_getfloatarg(Args,0);
    
    dt = sc_getfloatarg(Args,1) * 0.001;
    
    if (dt == 0)
	dt = 0.02;  // 20 ms as default control rate as in line
    mFreqMul = dt; 
    
    m_freq = m_freq_set * mFreqMul;
    
    m_timer.Periodic(dt);
    
}

void Impulse_kr::m_perform(void*)
{
    float z;
    if (mPhase >= 1.f) 
    {
	mPhase -= 1.f;
	z = 1.f;
    } 
    else 
    {
	z = 0.f;
    }

    mPhase += m_freq;
    ToOutFloat(0,z);
}
