/* sc4pd 
   NRand, NRand~

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
     
   Coded while listening to: Jim O'Rourke & Loren Mazzacane Connors: In Bern
   
*/

#include "sc4pd.hpp"


/* ------------------------ NRand~ -------------------------------*/

class NRand_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(NRand_ar,sc4pd_dsp);
    
public:
    NRand_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);

    void m_seed(int i)
    {
	rgen.init(i);
    }
    
private:
    float m_sample;
    float lo;
    float hi;
    int sc_n;
    RGen rgen;
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_DSP_V("NRand~",NRand_ar);

NRand_ar::NRand_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);

    AtomList Args(argc,argv);

    if (Args.Count() != 3)
    {
	post("not enough arguments");
	return;
    }
    lo=sc_getfloatarg(Args,0);
    hi=sc_getfloatarg(Args,1);
    sc_n=sc_getfloatarg(Args,2);
    
    rgen.init(timeseed());

    AddOutSignal();
}

void NRand_ar::m_dsp(int n, t_sample *const *in, t_sample *const *out)
{
    float range = hi - lo;
    float sum = 0;
    for (int i=0; i<sc_n; ++i) 
    {
	sum += rgen.frand();
    }
    m_sample = (sum/sc_n) * range + lo;
}


void NRand_ar::m_signal(int n, t_sample *const *in, 
		       t_sample *const *out)
{
    t_sample *nout = *out;
    
    float sample = m_sample;
    
    for (int i = 0; i!= n;++i)
    {
	(*(nout)++) = sample;
    }
}


/* ------------------------ NRand ---------------------------------*/

class NRand_kr:
    public flext_base
{
    FLEXT_HEADER(NRand_kr,flext_base);

public:
    NRand_kr(int argc, t_atom *argv);
    
protected:
    void m_loadbang();

    void m_seed(int i)
    {
	rgen.init(i);
    }

private:
    float lo;
    float hi;
    int sc_n;
    RGen rgen;
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_V("NRand",NRand_kr);

NRand_kr::NRand_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);

    AtomList Args(argc,argv);
    if (Args.Count() != 3)
    {
	post("not enough arguments");
	return;
    }
    lo=sc_getfloatarg(Args,0);
    hi=sc_getfloatarg(Args,1);
    sc_n=sc_getfloatarg(Args,2);
    
    rgen.init(timeseed());

    AddOutFloat();
}

void NRand_kr::m_loadbang()
{
    float range = hi - lo;
    float sum = 0;
    for (int i=0; i<sc_n; ++i) 
    {
	sum += rgen.frand();
    }
    ToOutFloat(0,(sum/sc_n) * range + lo);
}
