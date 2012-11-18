/* sc4pd 
   OneZero, OneZero~

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

/* ------------------------ OneZero~ -------------------------------*/

class OneZero_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(OneZero_ar,sc4pd_dsp);
    
public:
    OneZero_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);

    void m_set(float f)
    {
	n_b1=f;
	changed = true;
    }
    
private:
    float m_b1, m_x1;
    float n_b1;
    bool changed;
    
    FLEXT_CALLBACK_F(m_set);
};

FLEXT_LIB_DSP_V("OneZero~",OneZero_ar);

OneZero_ar::OneZero_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"coef",m_set);

    //parse arguments
    AtomList Args(argc,argv);
    
    m_b1 = sc_getfloatarg(Args,0);
    
    AddOutSignal();
    
    m_x1 = 0.f;
}    

void OneZero_ar::m_signal(int n, t_sample *const *in, 
			  t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    
    float b1 = m_b1;
    float x1 = m_x1;

    if (changed)
    {
	m_b1=n_b1;
	float b1_slope = CALCSLOPE(m_b1, b1);
	if (b1 >= 0.f && m_b1 >= 0) 
	{
	    for (int i = 0; i!= n;++i)
	    {
		float x0 = ZXP(nin); 
		ZXP(nout) = x0 + b1 * (x1 - x0);
		x1 = x0;
		b1 += b1_slope;
	    }
	} 
	else if (b1 <= 0.f && m_b1 <= 0) 
	{
	    for (int i = 0; i!= n;++i)
	    {
		float x0 = ZXP(nin); 
		ZXP(nout) = x0 + b1 * (x1 + x0);
		x1 = x0;
		b1 += b1_slope;
	    }
	}
	else
	{
	    for (int i = 0; i!= n;++i)
	    {
		float x0 = ZXP(nin); 
		ZXP(nout) = (1.f - fabs(b1)) * x0 + b1 * x1;
		x1 = x0;
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
		float x0 = ZXP(nin); 
		ZXP(nout) = x0 + b1 * (x1 - x0);
		x1 = x0;
	    }
	}
	else
	{
	    for (int i = 0; i!= n;++i)
	    {
		float x0 = ZXP(nin); 
		ZXP(nout) = x0 + b1 * (x1 + x0);
		x1 = x0;
	    }
	}
    }
    m_x1 = x1;
}

/* ------------------------ OneZero ---------------------------------*/


class OneZero_kr:
    public flext_base
{
    FLEXT_HEADER(OneZero_kr,flext_base);

public:
    OneZero_kr(int argc, t_atom *argv);
    
protected:
    void m_perform(float f);

    void m_set(float f)
    {
	m_b1=f;
    }

private:
    float m_b1, m_x1;

    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK_F(m_perform);
};


FLEXT_LIB_V("OneZero",OneZero_kr);

OneZero_kr::OneZero_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD(0,m_perform);
    FLEXT_ADDMETHOD_(0,"set",m_set);
    
    AddOutFloat();

    //parse arguments
    AtomList Args(argc,argv);

    m_b1 = sc_getfloatarg(Args,0);
    
    m_x1=0;
}

void OneZero_kr::m_perform(float f)
{
    ToOutFloat(0,((1-abs(m_b1))*f)+m_b1*m_x1);
    m_x1=f;
}
