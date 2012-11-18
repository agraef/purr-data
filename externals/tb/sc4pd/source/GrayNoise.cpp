/* sc4pd 
   GrayNoise, GrayNoise~

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
     
   Coded while listening to: Keith Rowe: Harsh
   
*/

#include "sc4pd.hpp"

/* ------------------------ GrayNoise~ -------------------------------*/

class GrayNoise_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(GrayNoise_ar,sc4pd_dsp);
    
public:
    GrayNoise_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);

    void m_seed(int i)
    {
	rgen.init(i);
    }
    
private:
    int m_counter;
    RGen rgen;
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_DSP_V("GrayNoise~",GrayNoise_ar);

GrayNoise_ar::GrayNoise_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);

    //parse arguments
    AtomList Args(argc,argv);

    rgen.init(timeseed());

    AddOutSignal();
}    


void GrayNoise_ar::m_signal(int n, t_sample *const *in, 
			       t_sample *const *out)
{
    t_sample *nout = *out;

    RGET;
    int counter = m_counter;

    for (int i = 0; i!= n;++i)
    {
	counter ^= 1L << (trand(s1,s2,s3) & 31);
	(*(nout)++) = counter * 4.65661287308e-10f;
    }
    
    m_counter=counter;
    RPUT;
}


/* ------------------------ GrayNoise ---------------------------------*/

class GrayNoise_kr:
    public flext_base
{
    FLEXT_HEADER(GrayNoise_kr,flext_base);

public:
    GrayNoise_kr(int argc, t_atom *argv);
    
protected:
    void m_perform();

    void m_seed(int i)
    {
	rgen.init(i);
    }
    
private:
    int m_counter;
    RGen rgen;
    FLEXT_CALLBACK(m_perform);
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_V("GrayNoise",GrayNoise_kr);

GrayNoise_kr::GrayNoise_kr(int argc, t_atom *argv)
{
    FLEXT_ADDBANG(0,m_perform);
    FLEXT_ADDMETHOD_(0,"seed",m_seed);

    //parse arguments
    AtomList Args(argc,argv);
    
    rgen.init(timeseed());

    AddOutFloat();
}

void GrayNoise_kr::m_perform()
{
    m_counter ^= 1L << (rgen.trand() & 31);
    m_counter * 4.65661287308e-10f;
    ToOutFloat(0,m_counter * 4.65661287308e-10f);
}
