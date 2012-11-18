/* sc4pd 
   BrownNoise, BrownNoise~

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
     
   Coded while listening to: AMM: Laminal
   
*/

#include "sc4pd.hpp"

/* ------------------------ BrownNoise~ -------------------------------*/

class BrownNoise_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(BrownNoise_ar,sc4pd_dsp);
    
public:
    BrownNoise_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    
    void m_seed(int i)
    {
	rgen.init(i);
    }
    
private:
    float m_level;
    RGen rgen;
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_DSP_V("BrownNoise~",BrownNoise_ar);

BrownNoise_ar::BrownNoise_ar(int argc, t_atom *argv)
{
    //parse arguments
    AtomList Args(argc,argv);

    rgen.init(timeseed());
    m_level=rgen.frand2();
    
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
    
    AddOutSignal();
}    


void BrownNoise_ar::m_signal(int n, t_sample *const *in, 
			       t_sample *const *out)
{
    t_sample *nout = *out;
    
    RGET;
	
    float z = m_level;

    for (int i = 0; i!= n;++i)
    {
	z += frand8(s1, s2, s3);
	if (z > 1.f) 
	    z = 2.f - z; 
	else 
	    if (z < -1.f) 
		z = -2.f - z; 
	(*(nout)++) = z;
    }
    m_level = z;
    
    RPUT;
}


/* ------------------------ BrownNoise ---------------------------------*/

class BrownNoise_kr:
    public flext_base
{
    FLEXT_HEADER(BrownNoise_kr,flext_base);

public:
    BrownNoise_kr(int argc, t_atom *argv);
    
protected:
    void m_perform();

    void m_seed(int i)
    {
	rgen.init(i);
    }
    
private:
    float m_level;
    RGen rgen;
    FLEXT_CALLBACK(m_perform);
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_V("BrownNoise",BrownNoise_kr);

BrownNoise_kr::BrownNoise_kr(int argc, t_atom *argv)
{
    FLEXT_ADDBANG(0,m_perform);
    FLEXT_ADDMETHOD_(0,"seed",m_seed);

    //parse arguments
    AtomList Args(argc,argv);
    
    rgen.init(timeseed());
    m_level=rgen.frand2(); 

    AddOutFloat();
}

void BrownNoise_kr::m_perform()
{
    float z = m_level + rgen.frand8();
    if (z > 1.f) 
	z = 2.f - z; 
    else 
	if (z < -1.f) 
	    z = -2.f - z; 
    ToOutFloat(0,z);
    m_level = z;
}
