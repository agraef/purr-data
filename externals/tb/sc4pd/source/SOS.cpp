/* sc4pd 
   SOS~

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
     
   Coded while listening to: Susie Ibarra & Assif Tsahar: Home Cookin'

*/

#include "sc4pd.hpp"

/* ------------------------ SOS~ -------------------------------*/

class SOS_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(SOS_ar,sc4pd_dsp);
    
public:
    SOS_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out)
    {
	m_signal_fun(n,in,out);
    }

    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out)
    {
	mFilterLoops = sc_filterloops();
	mFilterRemain = sc_filterremain();
	mFilterSlope = sc_filterslope();
    }

    void m_set_a0(float f)
    {
	next_a0=f;
    }

    void m_set_a1(float f)
    {
	next_a1=f;
    }

    void m_set_a2(float f)
    {
	next_a2=f;
    }

    void m_set_b1(float f)
    {
	next_b1=f;
    }

    void m_set_b2(float f)
    {
	next_b2=f;
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
    float next_a0, next_a1, next_a2, next_b1, next_b2;
    float m_y1, m_y2, m_a0, m_a1, m_a2, m_b1, m_b2;

    float mFilterSlope;
    int mFilterLoops, mFilterRemain;
    
    DEFSIGCALL (m_signal_fun);
    DEFSIGFUN (m_signal_ar);
    DEFSIGFUN (m_signal_kr);
    
    FLEXT_CALLBACK_F(m_set_a0);
    FLEXT_CALLBACK_F(m_set_a1);
    FLEXT_CALLBACK_F(m_set_a2);
    FLEXT_CALLBACK_F(m_set_b1);
    FLEXT_CALLBACK_F(m_set_b2);
    FLEXT_CALLBACK(m_ar);
    FLEXT_CALLBACK(m_kr);
};

FLEXT_LIB_DSP_V("SOS~",SOS_ar);

SOS_ar::SOS_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"a0",m_set_a0);
    FLEXT_ADDMETHOD_(0,"a1",m_set_a1);
    FLEXT_ADDMETHOD_(0,"a2",m_set_a2);
    FLEXT_ADDMETHOD_(0,"b1",m_set_b1);
    FLEXT_ADDMETHOD_(0,"b2",m_set_b2);
    FLEXT_ADDMETHOD_(0,"ar",m_ar);
    FLEXT_ADDMETHOD_(0,"kr",m_kr);

    //parse arguments
    AtomList Args(argc,argv);
    
    if (Args.Count()<5)
    {
	post("needs at least 5 arguments");
	return;
    }
    m_a0 = sc_getfloatarg(Args,0);
    m_a1 = sc_getfloatarg(Args,1);
    m_a2 = sc_getfloatarg(Args,2);
    m_b1 = sc_getfloatarg(Args,3);
    m_b2 = sc_getfloatarg(Args,4);
    
    if(sc_ar(Args))
    {
	AddInSignal();
	AddInSignal();
	AddInSignal();
	AddInSignal();
	AddInSignal();
	AddInSignal();
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_ar));
    }
    else // if not given, use control rate
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_kr)); 
    
    AddOutSignal();
    
    m_y1 = 0.f;
    m_a0 = 0.f;
    m_a1 = 0.f;
    m_a2 = 0.f;
    m_b1 = 0.f;
    m_b2 = 0.f;
}

void SOS_ar::m_signal_ar(int n, t_sample *const *in, 
	 		   t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    float *a0 = *(in+1);
    float *a1 = *(in+2);
    float *a2 = *(in+3);
    float *b1 = *(in+4);
    float *b2 = *(in+5);
    
    float y0;
    float y1 = m_y1;
    float y2 = m_y2;

    for (int i = 0; i!= mFilterLoops;++i)
    {
	y0 = ZXP(nin) + ZXP(b1) * y1 + ZXP(b2) * y2; 
	ZXP(nout) = ZXP(a0) * y0 + ZXP(a1) * y1 + ZXP(a2) * y2;
	
	y2 = ZXP(nin) + ZXP(b1) * y0 + ZXP(b2) * y1; 
	ZXP(nout) = ZXP(a0) * y2 + ZXP(a1) * y0 + ZXP(a2) * y1;
	
	y1 = ZXP(nin) + ZXP(b1) * y2 + ZXP(b2) * y0; 
	ZXP(nout) = ZXP(a0) * y1 + ZXP(a1) * y2 + ZXP(a2) * y0;
    }
    
    for (int i = 0; i!= mFilterRemain;++i)
    {
	y0 = ZXP(nin) + ZXP(b1) * y1 + ZXP(b2) * y2; 
	ZXP(nout) = ZXP(a0) * y0 + ZXP(a1) * y1 + ZXP(a2) * y2;
	y2 = y1; 
	y1 = y0;
    }

    m_y1 = zapgremlins(y1);
    m_y2 = zapgremlins(y2);
    
}


void SOS_ar::m_signal_kr(int n, t_sample *const *in, 
			   t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    
    float y0;
    float y1 = m_y1;
    float y2 = m_y2;
    float a0 = m_a0;
    float a1 = m_a1;
    float a2 = m_a2;
    float b1 = m_b1;
    float b2 = m_b2;
    float a0_slope = (next_a0 - a0) * mFilterSlope;
    float a1_slope = (next_a1 - a1) * mFilterSlope;
    float a2_slope = (next_a2 - a2) * mFilterSlope;
    float b1_slope = (next_b1 - b1) * mFilterSlope;
    float b2_slope = (next_b2 - b2) * mFilterSlope;

    for (int i = 0; i!= mFilterLoops;++i)
    {
	y0 = ZXP(nin) + b1 * y1 + b2 * y2; 
	ZXP(nout) = a0 * y0 + a1 * y1 + a2 * y2;
		
	y2 = ZXP(nin) + b1 * y0 + b2 * y1; 
	ZXP(nout) = a0 * y2 + a1 * y0 + a2 * y1;
	
	y1 = ZXP(nin) + b1 * y2 + b2 * y0; 
	ZXP(nout) = a0 * y1 + a1 * y2 + a2 * y0;
	
	a0 += a0_slope;
	a1 += a1_slope;
	a2 += a2_slope;
	b1 += b1_slope;
	b2 += b2_slope;
    }

    for (int i = 0; i!= mFilterRemain;++i)
    {
	y0 = ZXP(nin) + b1 * y1 + b2 * y2; 
	ZXP(nout) = a0 * y0 + a1 * y1 + a2 * y2;
	y2 = y1; 
	y1 = y0;
    }
    
    m_y1 = zapgremlins(y1);
    m_y2 = zapgremlins(y2);
}

/* no kr SOS */
