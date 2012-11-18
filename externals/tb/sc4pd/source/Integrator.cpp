/* sc4pd 
   Integrator~, Integrator

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

/* ------------------------ Integrator~ -----------------------------*/

class Integrator_ar
    :public sc4pd_dsp
{
    FLEXT_HEADER(Integrator_ar,sc4pd_dsp);

public:
    Integrator_ar(int argc,t_atom * argv);

protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    void m_set(float f);

private:
    FLEXT_CALLBACK_F(m_set);
    float m_b1;                       //leak
    float m_y1;                       //z-1
};

FLEXT_LIB_DSP_V("Integrator~",Integrator_ar);

Integrator_ar::Integrator_ar(int argc,t_atom * argv)
{
    FLEXT_ADDMETHOD_(0,"leak",m_set);

    AtomList Args(argc,argv);

    m_b1 = sc_getfloatarg(Args,0);

    AddOutSignal();

    m_y1 = 0.f;
}

void Integrator_ar::m_signal(int n, t_sample *const *in, 
				t_sample *const *out)
{
    t_sample *nout = *out;
    t_sample *nin = *in;

    float b1 = m_b1;
    float y1 = m_y1;

    if (b1 == m_b1) 
    {
	if (b1 == 1.f) 
	{
	    for (int i = 0; i!= n;++i)
	    {
		float y0 = (*(nin)++); 
		(*(nout)++) = y1 = y0 + y1;
	    }
	    
	} 
	else if (b1 == 0.f) 
	{
	    for (int i = 0; i!= n;++i)
	    {
		float y0 = (*(nin)++); 
		(*(nout)++) = y1 = y0;
	    }
	} 
	else 
	{
	    for (int i = 0; i!= n;++i)
	    {
		float y0 = (*(nin)++); 
		(*(nout)++) = y1 = y0 + b1 * y1;
	    }
	}
    } 
    else 
    {
	float b1_slope = CALCSLOPE(m_b1, b1);
	if (b1 >= 0.f && m_b1 >= 0) 
	{
	    for (int i = 0; i!= n;++i) 
	    {
		float y0 = (*(nin)++); 
		(*(nout)++) = y1 = y0 + b1 * (y1 - y0);
		b1 += b1_slope;
	    }
	} 
	else if (b1 <= 0.f && m_b1 <= 0) 
	{
	    for (int i = 0; i!= n;++i) 
	    {
		float y0 = (*(nin)++); 
		(*(nout)++) = y1 = y0 + b1 * (y1 + y0);
		b1 += b1_slope;
	    }
	} 
	else 
	{
	    for (int i = 0; i!= n;++i) 
	    {
		float y0 = (*(nin)++); 
		(*(nout)++) = y1 = (1.f - fabs(b1)) * y0 + b1 * y1;
		b1 += b1_slope;
	    }
	}
    }
    m_y1 = zapgremlins(y1);
}
    
void Integrator_ar::m_set(float f)
{
    m_b1=f;
}

/* ------------------------ Integrator ------------------------------*/

class Integrator_kr
    :public flext_base
{
    FLEXT_HEADER(Integrator_kr,flext_base);

public:
    Integrator_kr(int argc,t_atom * argv);

protected:
    void m_set(float f);
    void m_perform(float f);
    
private:
    float m_b1;                       //leak
    float m_y1;                       //z-1

    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK_F(m_perform);
};

FLEXT_LIB_V("Integrator",Integrator_kr);

Integrator_kr::Integrator_kr(int argc,t_atom * argv)
{
    AtomList Args(argc,argv);

    m_b1 = sc_getfloatarg(Args,0);
    m_y1 = 0.f;

    
    AddInFloat();
    AddOutFloat();

    FLEXT_ADDMETHOD(0,m_perform);
    FLEXT_ADDMETHOD_(0,"leak",m_set);
}

void Integrator_kr::m_perform(float f)
{
    m_y1 = f + m_y1 * m_b1;
    ToOutFloat(0,m_y1);
}

void Integrator_kr::m_set(float f)
{
    m_b1=f;
}
