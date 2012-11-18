/* sc4pd 
   LFSaw, LFSaw~

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


/* ------------------------ LFSaw~ -------------------------------*/

class LFSaw_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(LFSaw_ar,sc4pd_dsp);
    
public:
    LFSaw_ar(int argc, t_atom *argv);
    
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
    double mPhase;
    float mFreqMul;
    float m_freq;      //for kr arguments

    DEFSIGCALL (m_signal_fun);
    DEFSIGFUN (m_signal_ar);
    DEFSIGFUN (m_signal_kr);
    
    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK(m_ar);
    FLEXT_CALLBACK(m_kr);
};

FLEXT_LIB_DSP_V("LFSaw~",LFSaw_ar);

LFSaw_ar::LFSaw_ar(int argc, t_atom *argv)
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

void LFSaw_ar::m_dsp(int n, t_sample *const *in, t_sample *const *out)
{
    mFreqMul = 2 / Samplerate();
}

void LFSaw_ar::m_signal_ar(int n, t_sample *const *in, 
	 		   t_sample *const *out)
{
    t_sample *freq = *in;
    t_sample *xout = *out;
    
    float freqmul = mFreqMul;
    double phase = mPhase;
    
    for (int i = 0; i!= n;++i)
    {
	float z = phase; // out must be written last for in place operation
	phase += (*(freq)++) * freqmul;
	if (phase >= 1.f) 
	    phase -= 2.f;
	else 
	    if (phase <= -1.f) 
		phase += 2.f;
	(*(xout)++) = z;
    }

    mPhase=phase;
}


void LFSaw_ar::m_signal_kr(int n, t_sample *const *in, 
			   t_sample *const *out)
{
    t_sample *xout = *out;
    
    float freq = m_freq * mFreqMul;
    double phase = mPhase;
    
    if (freq >= 0.f) 
    {
	for (int i = 0; i!= n;++i)
	{
	    (*(xout)++) = phase;
	    phase += freq;
	    if (phase >= 1.f) phase -= 2.f;
	}
    }
    else
    {
	for (int i = 0; i!= n;++i)
	{
	    (*(xout)++) = phase;
	    phase += freq;
	    if (phase <= -1.f) phase += 2.f;
	}
    }

    mPhase=phase;
}

/* ------------------------ LFSaw ---------------------------------*/

class LFSaw_kr:
    public flext_base
{
    FLEXT_HEADER(LFSaw_kr,flext_base);

public:
    LFSaw_kr(int argc, t_atom *argv);
    
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
 	    mFreqMul = 2*dt;
	    m_freq = m_freq_set * mFreqMul;
	    m_timer.Reset();
	    m_timer.Periodic(dt);
	}
    }

private:
    double mPhase;
    float mFreqMul;
    float m_freq;
    float dt;
    float m_freq_set;

    Timer m_timer;

    FLEXT_CALLBACK_F(m_set_kr);
    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK_T(m_perform);
};


FLEXT_LIB_V("LFSaw",LFSaw_kr);

LFSaw_kr::LFSaw_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD(0,m_set);
    FLEXT_ADDMETHOD_(0,"kr",m_set_kr);
    //    FLEXT_ADDBANG(0,m_perform);
    FLEXT_ADDTIMER(m_timer,m_perform);

    AddOutFloat();

    //parse arguments
    AtomList Args(argc,argv);

    m_freq_set = sc_getfloatarg(Args,0);
    
    dt = sc_getfloatarg(Args,1) * 0.001;
    
    if (dt == 0 )
	dt = 0.02;  // 20 ms as default control rate as in line
    mFreqMul = 2 * dt; /* test this !!! */
    
    m_freq = m_freq_set * mFreqMul;
    
    m_timer.Periodic(dt);
    
}

void LFSaw_kr::m_perform(void*)
{
    if (m_freq >= 0.f) 
    {
	ToOutFloat(0,mPhase);
	mPhase += m_freq;
	if (mPhase >= 1.f) mPhase -= 2.f;
    }
    else
    {
	ToOutFloat(0,mPhase);
	mPhase += m_freq;
	if (mPhase <= -1.f) mPhase += 2.f;
    }
}
