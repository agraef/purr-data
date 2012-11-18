/* sc4pd 
   Logistic, Logistic~

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
     
   Coded while listening to: Fred Frith: Gravity
   
*/

#include "sc4pd.hpp"


/* ------------------------ Logistic~ -------------------------------*/

class Logistic_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(Logistic_ar,sc4pd_dsp);
    
public:
    Logistic_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);

    void m_set(float f)
    {
	m_freq = sc_min(f,m_sr);
    }

    void m_param(float f)
    {
	m_paramf = f;
    }
    
private:
    int m_freq;
    float m_paramf;

    int m_sr;
    double m_y1;
    int m_counter;

    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK_F(m_param);
};

FLEXT_LIB_DSP_V("Logistic~",Logistic_ar);

Logistic_ar::Logistic_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"set",m_set);
    FLEXT_ADDMETHOD_(0,"parameter",m_set);

    //parse arguments
    AtomList Args(argc,argv);

    m_freq = sc_getfloatarg(Args,0);
    m_paramf = sc_getfloatarg(Args,1);
    m_y1 = sc_getfloatarg(Args,2);
    
    m_sr=48000; // this is just a guess (i'll think about this later)
    
    AddOutSignal();
}    

void Logistic_ar::m_dsp(int n, t_sample *const *in, 
			   t_sample *const *out)
{
    m_sr = Samplerate();
    m_paramf = sc_min(m_paramf,m_sr);
}


void Logistic_ar::m_signal(int n, t_sample *const *in, 
			       t_sample *const *out)
{
    t_sample *nout = *out;

    if(m_sr == m_paramf)
    {
	/* it might be possible to implement this without the branch */

	double y1 = m_y1;
	double paramf = m_paramf;
	for (int i = 0; i!= n;++i)
	{
	    (*(nout)++)= y1 = paramf * y1 * (1.0 - y1);
	}
	m_y1 = y1;
    }
    else
    {
	double y1 = m_y1;
	double paramf = m_paramf;
	int counter = m_counter;
	int remain = n;
	do
	{
	    if (counter<=0) 
	    {
		counter = (int)(m_sr / sc_max(m_freq, .001f));
		counter = sc_max(1, counter);
		y1 = paramf * y1 * (1.0 - y1);
	    }
	    
	    int nsmps = sc_min(remain, counter);
	    remain -= nsmps;
	    counter -= nsmps;
	    
	    for (int i = 0; i!= nsmps;++i)
	    {
		(*(nout)++)=y1;
	    }
	}
	while(remain);
	m_y1 = y1;
	m_counter = counter;
    }
}


/* ------------------------ Logistic ---------------------------------*/

class Logistic_kr:
    public flext_base
{
    FLEXT_HEADER(Logistic_kr,flext_base);

public:
    Logistic_kr(int argc, t_atom *argv);
    
protected:
    void m_perform();

    void m_param(float f)
    {
	m_paramf = f; 
    }
   
private:
    float m_paramf;
    double m_y1;

    FLEXT_CALLBACK(m_perform);
    FLEXT_CALLBACK_F(m_param);
};

FLEXT_LIB_V("Logistic",Logistic_kr);

Logistic_kr::Logistic_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"parameter",m_param);
    FLEXT_ADDBANG(0,m_perform);
    
    //parse arguments
    AtomList Args(argc,argv);
    
    m_paramf = sc_getfloatarg(Args,0);
    m_y1 = sc_getfloatarg(Args,1);

    AddOutFloat();
}

void Logistic_kr::m_perform()
{
    m_y1 = m_paramf * m_y1 * (1.0 - m_y1);
    ToOutFloat(0,m_y1);
}
