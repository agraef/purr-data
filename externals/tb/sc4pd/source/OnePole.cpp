/* sc4pd 
   OnePole, OnePole~

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
     
   Coded while listening to: Goh Lee Kwang: Internal Pleasures
   
*/

#include "sc4pd.hpp"

/* ------------------------ OnePole~ -------------------------------*/

class OnePole_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(OnePole_ar,sc4pd_dsp);
    
public:
    OnePole_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out)
    {
	m_signal_fun(n,in,out);
    }

    void m_set(float f)
    {
	n_b1=f;
	changed = true;
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
    float m_b1, m_y1;
    float n_b1;
    bool changed;
    
    DEFSIGCALL (m_signal_fun);
    DEFSIGFUN (m_signal_ar);
    DEFSIGFUN (m_signal_kr);
    
    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK(m_ar);
    FLEXT_CALLBACK(m_kr);
};

FLEXT_LIB_DSP_V("OnePole~",OnePole_ar);

OnePole_ar::OnePole_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"coef",m_set);
    FLEXT_ADDMETHOD_(0,"ar",m_ar);
    FLEXT_ADDMETHOD_(0,"kr",m_kr);

    //parse arguments
    AtomList Args(argc,argv);
    
    m_b1 = sc_getfloatarg(Args,0);
    
    if(sc_ar(Args))
    {
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_ar));
	AddInSignal();
	AddInSignal();
    }
    else // if not given, use control rate
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_kr)); 
    
    AddOutSignal();
    
    m_y1 = 0.f;
}    

void OnePole_ar::m_signal_ar(int n, t_sample *const *in, 
	 		   t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    float *b1p = *(in+1);
    
    float y1 = m_y1;

    for (int i = 0; i!= n;++i)
    {
	float y0 = ZXP(nin); 
	float b1 = ZXP(b1p); 
	ZXP(nout) = y1 = y0 + b1 * (y1 - y0);
    }
    m_y1 = zapgremlins(y1);
}


void OnePole_ar::m_signal_kr(int n, t_sample *const *in, 
			   t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    
    float b1 = m_b1;
    float y1 = m_y1;

    if (changed)
    {
	m_b1=n_b1;
	float b1_slope = CALCSLOPE(m_b1, b1);
	if (b1 >= 0.f && m_b1 >= 0) 
	{
	    for (int i = 0; i!= n;++i)
	    {
		float y0 = ZXP(nin); 
		ZXP(nout) = y1 = y0 + b1 * (y1 - y0);
		b1 += b1_slope;
	    }
	} 
	else if (b1 <= 0.f && m_b1 <= 0) 
	{
	    for (int i = 0; i!= n;++i)
	    {
		float y0 = ZXP(nin); 
		ZXP(nout) = y1 = y0 + b1 * (y1 + y0);
		b1 += b1_slope;
	    }
	}
	else
	{
	    for (int i = 0; i!= n;++i)
	    {
		float y0 = ZXP(nin); 
		ZXP(nout) = y1 = (1.f - fabs(b1)) * y0 + b1 * y1;
		b1 += b1_slope;
	    }
	}
	changed = false;
    }
    else
    {
	if (b1 >= 0.f) 
	{
	    for (int i = 0; i!= n;++i)
	    {
		float y0 = ZXP(nin); 
		ZXP(nout) = y1 = y0 + b1 * (y1 - y0);
	    }
	}
	else
	{
	    for (int i = 0; i!= n;++i)
	    {
		float y0 = ZXP(nin); 
		ZXP(nout) = y1 = y0 + b1 * (y1 + y0);
	    }
	}

    }
    m_y1 = zapgremlins(y1);
}

/* ------------------------ OnePole ---------------------------------*/


class OnePole_kr:
    public flext_base
{
    FLEXT_HEADER(OnePole_kr,flext_base);

public:
    OnePole_kr(int argc, t_atom *argv);
    
protected:
    void m_perform(float f);

    void m_set(float f)
    {
	m_b1=f;
    }

private:
    float m_b1, m_y1;

    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK_F(m_perform);
};


FLEXT_LIB_V("OnePole",OnePole_kr);

OnePole_kr::OnePole_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD(0,m_perform);
    FLEXT_ADDMETHOD_(0,"set",m_set);
    
    AddOutFloat();

    //parse arguments
    AtomList Args(argc,argv);

    m_b1 = sc_getfloatarg(Args,0);
    
    m_y1=0;
}

void OnePole_kr::m_perform(float f)
{
    m_y1= ((1-abs(m_b1))*f)+m_b1*m_y1;
    ToOutFloat(0,m_y1);
}
