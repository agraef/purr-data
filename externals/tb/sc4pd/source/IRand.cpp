/* sc4pd 
   IRand, IRand~

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

/* ------------------------ IRand~ -------------------------------*/

class IRand_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(IRand_ar,sc4pd_dsp);
    
public:
    IRand_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);

    void m_seed(int i)
    {
	rgen.init(i);
    }
    
private:
    float m_sample;
    int lo;
    int hi;
    RGen rgen;
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_DSP_V("IRand~",IRand_ar);

IRand_ar::IRand_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);

    AtomList Args(argc,argv);

    if (Args.Count() != 2)
    {
	post("not enough arguments");
	return;
    }
    lo=int(sc_getfloatarg(Args,0));
    hi=int(sc_getfloatarg(Args,1));
    
    rgen.init(timeseed());

    AddOutSignal();
}

void IRand_ar::m_dsp(int n, t_sample *const *in, t_sample *const *out)
{
    int range = hi - lo;
    m_sample = float(rgen.irand(range) + lo);
}


void IRand_ar::m_signal(int n, t_sample *const *in, 
		       t_sample *const *out)
{
    t_sample *nout = *out;
    
    float sample = m_sample;
    
    for (int i = 0; i!= n;++i)
    {
	(*(nout)++) = sample;
    }
}


/* ------------------------ IRand ---------------------------------*/

class IRand_kr:
    public flext_base
{
    FLEXT_HEADER(IRand_kr,flext_base);

public:
    IRand_kr(int argc, t_atom *argv);
    
protected:
    void m_loadbang();

    void m_seed(int i)
    {
	rgen.init(i);
    }

private:
    int lo;
    int hi;
    RGen rgen;
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_V("IRand",IRand_kr);

IRand_kr::IRand_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);

    AtomList Args(argc,argv);
    if (Args.Count() != 2)
    {
	post("not enough arguments");
	return;
    }
    lo=int(sc_getfloatarg(Args,0));
    hi=int(sc_getfloatarg(Args,1));
    
    rgen.init(timeseed());
    
    AddOutInt();
}

void IRand_kr::m_loadbang()
{
    int range = hi - lo;
    
    ToOutInt(0,rgen.irand(range) + lo);
}
