/* sc4pd 
   TExpRand, TExpRand~

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
     
   Coded while listening to: Assif Tsahar: Open Systems
   
*/

#include "sc4pd.hpp"


/* ------------------------ TExpRand~ -------------------------------*/

class TExpRand_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(TExpRand_ar,sc4pd_dsp);
    
public:
    TExpRand_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);
    
    void m_bang();

    void m_sethi(float f)
    {
	hi = f;
	ratio = hi / lo;
    }

    void m_setlo(float f)
    {
	lo = f;
	ratio = hi / lo;
    }

    void m_seed(int i)
    {
	rgen.init(i);
    }
    
private:
    float m_sample;
    float lo;
    float hi;
    float ratio;
    RGen rgen;

    FLEXT_CALLBACK(m_bang);
    FLEXT_CALLBACK_F(m_setlo);
    FLEXT_CALLBACK_F(m_sethi);
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_DSP_V("TExpRand~",TExpRand_ar);

TExpRand_ar::TExpRand_ar(int argc, t_atom *argv)
{
    AtomList Args(argc,argv);

    if (Args.Count() != 2)
    {
	post("not enough arguments");
	return;
    }
    lo=sc_getfloatarg(Args,0);
    hi=sc_getfloatarg(Args,1);
    ratio = hi / lo;
    
    rgen.init(timeseed());

    AddOutSignal();

    FLEXT_ADDBANG(0,m_bang);
    FLEXT_ADDMETHOD_(0,"setlo",m_setlo);
    FLEXT_ADDMETHOD_(0,"sethi",m_sethi);
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
}

void TExpRand_ar::m_dsp(int n, t_sample *const *in, t_sample *const *out)
{
    m_sample = pow(ratio, rgen.frand()) * lo;
}


void TExpRand_ar::m_signal(int n, t_sample *const *in, 
		       t_sample *const *out)
{
    t_sample *nout = *out;
    
    float sample = m_sample;
    
    for (int i = 0; i!= n;++i)
    {
	(*(nout)++) = sample;
    }
}

void TExpRand_ar::m_bang()
{
    m_sample = pow(ratio, rgen.frand()) * lo;
}

/* ------------------------ TExpRand ---------------------------------*/

class TExpRand_kr:
    public flext_base
{
    FLEXT_HEADER(TExpRand_kr,flext_base);

public:
    TExpRand_kr(int argc, t_atom *argv);
    
protected:
    void m_loadbang();
    void m_bang();

    void m_sethi(float f)
    {
	hi = f;
	ratio = hi / lo;
    }

    void m_setlo(float f)
    {
	lo = f;
	ratio = hi / lo;
    }

    void m_seed(int i)
    {
	rgen.init(i);
    }

private:
    float lo;
    float hi;
    float ratio;
    RGen rgen;
    FLEXT_CALLBACK(m_bang);
    FLEXT_CALLBACK_F(m_setlo);
    FLEXT_CALLBACK_F(m_sethi);
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_V("TExpRand",TExpRand_kr);

TExpRand_kr::TExpRand_kr(int argc, t_atom *argv)
{
    AtomList Args(argc,argv);
    if (Args.Count() != 2)
    {
	post("not enough arguments");
	return;
    }
    lo=sc_getfloatarg(Args,0);
    hi=sc_getfloatarg(Args,1);
    ratio = hi / lo;
    
    rgen.init(timeseed());
    
    AddOutFloat();

    FLEXT_ADDBANG(0,m_bang);
    FLEXT_ADDMETHOD_(0,"setlo",m_setlo);
    FLEXT_ADDMETHOD_(0,"sethi",m_sethi);
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
}

void TExpRand_kr::m_loadbang()
{
    ToOutFloat(0,pow(ratio, rgen.frand()) * lo);
}

void TExpRand_kr::m_bang()
{
    ToOutFloat(0,pow(ratio, rgen.frand()) * lo);
}
