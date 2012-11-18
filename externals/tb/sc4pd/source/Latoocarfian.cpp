/* sc4pd 
   Latoocarfian, Latoocarfian~

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
     
   Coded while listening to: Keith Rowe & John Tilbury: Duos For Doris
   
*/

#include "sc4pd.hpp"


/* ------------------------ Latoocarfian~ -------------------------------*/

class Latoocarfian_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(Latoocarfian_ar,sc4pd_dsp);
    
public:
    Latoocarfian_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);

    void m_set_a(float f)
    {
	m_a = f;
    }

    void m_set_b(float f)
    {
	m_b = f;
    }

    void m_set_c(float f)
    {
	m_c = f;
    }

    void m_set_d(float f)
    {
	m_d = f;
    }

private:
    float m_x, m_y;           //state
    float m_a, m_b, m_c, m_d; //parameters

    FLEXT_CALLBACK_F(m_set_a);
    FLEXT_CALLBACK_F(m_set_b);
    FLEXT_CALLBACK_F(m_set_c);
    FLEXT_CALLBACK_F(m_set_d);
};

FLEXT_LIB_DSP_V("Latoocarfian~",Latoocarfian_ar);

Latoocarfian_ar::Latoocarfian_ar(int argc, t_atom *argv)
    :m_x(4),m_y(1)
{
    FLEXT_ADDMETHOD_(0,"a",m_set_a);
    FLEXT_ADDMETHOD_(0,"b",m_set_b);
    FLEXT_ADDMETHOD_(0,"c",m_set_c);
    FLEXT_ADDMETHOD_(0,"d",m_set_d);

    //parse arguments
    AtomList Args(argc,argv);
    if (Args.Count()!=4)
    {
	post("4 arguments needed");
    }
    m_a = sc_getfloatarg(Args,0);
    m_b = sc_getfloatarg(Args,1);
    m_c = sc_getfloatarg(Args,2);
    m_d = sc_getfloatarg(Args,3);

    AddOutSignal();
    AddOutSignal();  // supercollider is only using the x coordinate
}    


void Latoocarfian_ar::m_signal(int n, t_sample *const *in, 
			       t_sample *const *out)
{
    t_sample *xout = *out;
    t_sample *yout = *(out+1);

    float a = m_a;
    float b = m_b;
    float c = m_c;
    float d = m_d;

    float x = m_x;
    float y = m_y;
        
    for (int i = 0; i!= n;++i)
    {
	float x_new=sin(y*b)+c*sin(x*b);
	float y_new=sin(x*a)+d*sin(y*a);
	(*(xout)++)=x=x_new;
	(*(yout)++)=y=y_new;	 
    }
    m_x = x;
    m_y = y;
}



/* ------------------------ Latoocarfian ---------------------------------*/

class Latoocarfian_kr:
    public flext_base
{
    FLEXT_HEADER(Latoocarfian_kr,flext_base);

public:
    Latoocarfian_kr(int argc, t_atom *argv);
    
protected:
    void m_perform();

    void m_set_a(float f)
    {
	m_a = f;
    }

    void m_set_b(float f)
    {
	m_b = f;
    }

    void m_set_c(float f)
    {
	m_c = f;
    }

    void m_set_d(float f)
    {
	m_d = f;
    }

private:
    float m_x, m_y;           //state
    float m_a, m_b, m_c, m_d; //parameters

    FLEXT_CALLBACK_F(m_set_a);
    FLEXT_CALLBACK_F(m_set_b);
    FLEXT_CALLBACK_F(m_set_c);
    FLEXT_CALLBACK_F(m_set_d);
    FLEXT_CALLBACK(m_perform);
};


FLEXT_LIB_V("Latoocarfian",Latoocarfian_kr);

Latoocarfian_kr::Latoocarfian_kr(int argc, t_atom *argv)
    :m_x(1),m_y(1)
{
    FLEXT_ADDMETHOD_(0,"a",m_set_a);
    FLEXT_ADDMETHOD_(0,"b",m_set_b);
    FLEXT_ADDMETHOD_(0,"c",m_set_c);
    FLEXT_ADDMETHOD_(0,"d",m_set_d);

    FLEXT_ADDBANG(0,m_perform);
    
    //parse arguments
    AtomList Args(argc,argv);
    if (Args.Count()!=4)
    {
	post("4 arguments needed");
    }
    m_a = sc_getfloatarg(Args,0);
    m_b = sc_getfloatarg(Args,1);
    m_c = sc_getfloatarg(Args,2);
    m_d = sc_getfloatarg(Args,3);


    AddOutFloat();
    AddOutFloat(); // one outlet more than sc 
}

void Latoocarfian_kr::m_perform()
{
    m_x=sin(m_y*m_b)+m_c*sin(m_x*m_b);
    m_y=sin(m_x*m_a)+m_d*sin(m_y*m_a);
    
    ToOutFloat(0,m_x);
    ToOutFloat(1,m_y);
}
