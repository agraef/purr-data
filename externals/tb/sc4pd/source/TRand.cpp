/* sc4pd 
   TRand, TRand~

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
     
   Coded while listening to: Annette Krebs & Taku Sugimoto: A Duo In Berlin
   
*/

#include "sc4pd.hpp"


/* ------------------------ TRand~ -------------------------------*/

class TRand_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(TRand_ar,sc4pd_dsp);
    
public:
    TRand_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);
    
    void m_bang();
    
    void m_sethi(float f)
    {
	hi = f;
	range = hi - lo;
    }

    void m_setlo(float f)
    {
	lo = f;
	range = hi - lo;
    }

    void m_seed(int i)
    {
	rgen.init(i);
    }

private:
    float m_sample;
    float lo;
    float hi;
    float range;
    RGen rgen;

    FLEXT_CALLBACK(m_bang);
    FLEXT_CALLBACK_F(m_setlo);
    FLEXT_CALLBACK_F(m_sethi);
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_DSP_V("TRand~",TRand_ar);

TRand_ar::TRand_ar(int argc, t_atom *argv)
{
    AtomList Args(argc,argv);

    if (Args.Count() != 2)
    {
	post("not enough arguments");
	return;
    }
    lo=sc_getfloatarg(Args,0);
    hi=sc_getfloatarg(Args,1);
    range = hi - lo;
    
    rgen.init(timeseed());

    AddOutSignal();

    FLEXT_ADDBANG(0,m_bang);
    FLEXT_ADDMETHOD_(0,"setlo",m_setlo);
    FLEXT_ADDMETHOD_(0,"sethi",m_sethi);
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
}

void TRand_ar::m_dsp(int n, t_sample *const *in, t_sample *const *out)
{
    m_sample = rgen.frand() * range + lo;
}


void TRand_ar::m_signal(int n, t_sample *const *in, 
		       t_sample *const *out)
{
    t_sample *nout = *out;
    
    float sample = m_sample;
    
    for (int i = 0; i!= n;++i)
    {
	(*(nout)++) = sample;
    }
}

void TRand_ar::m_bang()
{
    m_sample = rgen.frand() * range + lo;
}

/* ------------------------ TRand ---------------------------------*/

class TRand_kr:
    public flext_base
{
    FLEXT_HEADER(TRand_kr,flext_base);

public:
    TRand_kr(int argc, t_atom *argv);
    
protected:
    void m_loadbang();
    void m_bang();

    void m_sethi(float f)
    {
	hi = f;
	range = hi - lo;
    }

    void m_setlo(float f)
    {
	lo = f;
	range = hi - lo;
    }

    void m_seed(int i)
    {
	rgen.init(i);
    }

private:
    float lo;
    float hi;
    float range;
    RGen rgen;
    FLEXT_CALLBACK(m_bang);
    FLEXT_CALLBACK_F(m_setlo);
    FLEXT_CALLBACK_F(m_sethi);
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_V("TRand",TRand_kr);

TRand_kr::TRand_kr(int argc, t_atom *argv)
{
    AtomList Args(argc,argv);
    if (Args.Count() != 2)
    {
	post("not enough arguments");
	return;
    }
    lo=sc_getfloatarg(Args,0);
    hi=sc_getfloatarg(Args,1);
    range = hi - lo;
    
    rgen.init(timeseed());
    
    AddOutFloat();

    FLEXT_ADDBANG(0,m_bang);
    FLEXT_ADDMETHOD_(0,"setlo",m_setlo);
    FLEXT_ADDMETHOD_(0,"sethi",m_sethi);
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
}

void TRand_kr::m_loadbang()
{
    ToOutFloat(0,rgen.frand() * range + lo);
}

void TRand_kr::m_bang()
{
    ToOutFloat(0,rgen.frand() * range + lo);
}
