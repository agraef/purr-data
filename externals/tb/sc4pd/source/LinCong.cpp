/* sc4pd 
   LinCong, LinCong~

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


/* ------------------------ LinCong~ -------------------------------*/

class LinCong_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(LinCong_ar,sc4pd_dsp);
    
public:
    LinCong_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);

    void m_set_seed(float f)
    {
	m_seed = (int)f;
    }

    void m_set_mul(float f)
    {
	m_mul = (int)f;
    }

    void m_set_add(float f)
    {
	m_add = (int)f;
    }

    void m_set_mod(float f)
    {
	m_mod = (int)f;
	m_scale = 2/f; //output between -1 and 1
    }

private:
    unsigned int m_seed;           //seed
    unsigned int m_mul, m_add, m_mod; //parameters
    
    float m_scale;

    FLEXT_CALLBACK_F(m_set_seed);
    FLEXT_CALLBACK_F(m_set_mul);
    FLEXT_CALLBACK_F(m_set_add);
    FLEXT_CALLBACK_F(m_set_mod);
};

FLEXT_LIB_DSP_V("LinCong~",LinCong_ar);

LinCong_ar::LinCong_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_set_seed);
    FLEXT_ADDMETHOD_(0,"mul",m_set_mul);
    FLEXT_ADDMETHOD_(0,"add",m_set_add);
    FLEXT_ADDMETHOD_(0,"mod",m_set_mod);

    //parse arguments
    AtomList Args(argc,argv);
    if (Args.Count()!=4)
    {
	post("4 arguments needed");
    }
    m_seed = sc_getfloatarg(Args,0);
    m_mul = sc_getfloatarg(Args,1);
    m_add = sc_getfloatarg(Args,2);
    m_set_mod(sc_getfloatarg(Args,3));
    
    AddOutSignal();
}    


void LinCong_ar::m_signal(int n, t_sample *const *in, 
			       t_sample *const *out)
{
    t_sample *xout = *out;


    unsigned int seed = m_seed;
    unsigned int mul = m_mul;
    unsigned int add = m_add;
    unsigned int mod = m_mod;

    float scale = m_scale;
      
    for (int i = 0; i!= n;++i)
    {
	seed=(seed * mul + add) % mod;
	(*(xout)++)=float(seed)*scale - 1;
    }

    m_seed = seed;
}



/* ------------------------ LinCong ---------------------------------*/

class LinCong_kr:
    public flext_base
{
    FLEXT_HEADER(LinCong_kr,flext_base);

public:
    LinCong_kr(int argc, t_atom *argv);
    
protected:
    void m_perform();

    void m_set_seed(float f)
    {
	m_seed = (int)f;
    }

    void m_set_mul(float f)
    {
	m_mul = (int)f;
    }

    void m_set_add(float f)
    {
	m_add = (int)f;
    }

    void m_set_mod(float f)
    {
	m_mod = (int)f;
	m_scale = 2/f; //output between -1 and 1
    }

private:
    unsigned int m_seed;           //seed
    unsigned int m_mul, m_add, m_mod; //parameters
    
    float m_scale;

    FLEXT_CALLBACK_F(m_set_seed);
    FLEXT_CALLBACK_F(m_set_mul);
    FLEXT_CALLBACK_F(m_set_add);
    FLEXT_CALLBACK_F(m_set_mod);
    FLEXT_CALLBACK(m_perform);
};


FLEXT_LIB_V("LinCong",LinCong_kr);

LinCong_kr::LinCong_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_set_seed);
    FLEXT_ADDMETHOD_(0,"mul",m_set_mul);
    FLEXT_ADDMETHOD_(0,"add",m_set_add);
    FLEXT_ADDMETHOD_(0,"mod",m_set_mod);
    FLEXT_ADDBANG(0,m_perform);

    //parse arguments
    AtomList Args(argc,argv);
    if (Args.Count()!=4)
    {
	post("4 arguments needed");
    }
    m_seed = sc_getfloatarg(Args,0);
    m_mul = sc_getfloatarg(Args,1);
    m_add = sc_getfloatarg(Args,2);
    m_set_mod(sc_getfloatarg(Args,3));

    AddOutFloat();
}

void LinCong_kr::m_perform()
{
    m_seed=(m_seed * m_mul + m_add) % m_mod;
    ToOutFloat(0,float(m_seed) * m_scale - 1);
}
