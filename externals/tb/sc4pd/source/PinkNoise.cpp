/* sc4pd 
   PinkNoise, PinkNoise~

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
     
   Coded while listening to: Gottfried Michael Koenig: Klangfiguren II
   
*/

#include "sc4pd.hpp"

/* ------------------------ PinkNoise~ -------------------------------*/

class PinkNoise_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(PinkNoise_ar,sc4pd_dsp);
    
public:
    PinkNoise_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);

    void m_seed(int i)
    {
	rgen.init(i);
    }
    
private:
    uint32 m_dice[16];
    int32 m_total;
    RGen rgen;
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_DSP_V("PinkNoise~",PinkNoise_ar);

PinkNoise_ar::PinkNoise_ar(int argc, t_atom *argv)
    : m_total(0)
{
    FLEXT_ADDMETHOD_(0,"seed",m_seed);

    //parse arguments
    AtomList Args(argc,argv);

    rgen.init(timeseed());

    for (int i=0; i<16; ++i) 
    {
	uint32 newrand = rgen.trand() >> 13;
	m_total += newrand;
	m_dice[i] = newrand;
    }

    AddOutSignal();
}    


void PinkNoise_ar::m_signal(int n, t_sample *const *in, 
			       t_sample *const *out)
{
    t_sample *nout = *out;

    RGET;
    uint32 total = m_total;
    uint32 *dice = m_dice;

    for (int i = 0; i!= n;++i)
    {
	uint32 counter = trand(s1,s2,s3); // Magnus Jonsson's suggestion.
	uint32 newrand = counter >> 13;
	int k = (CTZ(counter)) & 15;
	uint32 prevrand = dice[k];
	dice[k] = newrand;
	total += (newrand - prevrand);
	newrand = trand(s1,s2,s3) >> 13;
	uint32 ifval = (total + newrand) | 0x40000000;
	(*(nout)++) = ((*(float*)&ifval) - 3.0f);
    }
    RPUT;
}


/* ------------------------ PinkNoise ---------------------------------*/

class PinkNoise_kr:
    public flext_base
{
    FLEXT_HEADER(PinkNoise_kr,flext_base);

public:
    PinkNoise_kr(int argc, t_atom *argv);
    
protected:
    void m_perform();

    void m_seed(int i)
    {
	rgen.init(i);
    }
    
private:
    uint32 m_dice[16];
    int32 m_total;
    RGen rgen;
    FLEXT_CALLBACK(m_perform);
    FLEXT_CALLBACK_I(m_seed);
};

FLEXT_LIB_V("PinkNoise",PinkNoise_kr);

PinkNoise_kr::PinkNoise_kr(int argc, t_atom *argv)
    : m_total(0)
{
    FLEXT_ADDBANG(0,m_perform);
    FLEXT_ADDMETHOD_(0,"seed",m_seed);

    //parse arguments
    AtomList Args(argc,argv);
    
    rgen.init(timeseed());

    for (int i=0; i<16; ++i) 
    {
	uint32 newrand = rgen.trand() >> 13;
	m_total += newrand;
	m_dice[i] = newrand;
    }

    AddOutFloat();
}

void PinkNoise_kr::m_perform()
{
    uint32 counter = rgen.trand(); // Magnus Jonsson's suggestion.
    uint32 newrand = counter >> 13;
    int k = (CTZ(counter)) & 15; 
    uint32 prevrand = m_dice[k]; 
    m_dice[k] = newrand; 
    m_total += (newrand - prevrand); 
    newrand = rgen.trand() >> 13;
    uint32 ifval = (m_total + newrand) | 0x40000000;
    
    ToOutFloat(0,((*(float*)&ifval) - 3.0f));
}
