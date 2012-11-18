/* sc4pd 
   BRZ2~

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
     
   Coded while listening to: William Parker: Compassion Seizes Bed-Stuy

*/

#include "sc4pd.hpp"

/* ------------------------ BRZ2~ -------------------------------*/

class BRZ2_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(BRZ2_ar,sc4pd_dsp);
    
public:
    BRZ2_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out)
    {
	mFilterLoops=sc_filterloops();
	mFilterRemain=sc_filterremain();
    }

private:
    float m_x1, m_x2;
    int mFilterLoops, mFilterRemain;
};

FLEXT_LIB_DSP_V("BRZ2~",BRZ2_ar);

BRZ2_ar::BRZ2_ar(int argc, t_atom *argv)
{
    AddOutSignal();

    m_x1=0;
    m_x2=0;
}

void BRZ2_ar::m_signal(int n, t_sample *const *in, 
			  t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;

    float x0;
    float x1 = m_x1;
    float x2 = m_x2;

    for (int i = 0; i!=mFilterLoops ;++i)
    {
	x0 = ZXP(nin);
	ZXP(nout) = (x0 + x2) * 0.5f;
	x2 = ZXP(nin);
	ZXP(nout) = (x2 + x1) * 0.5f;
	x1 = ZXP(nin);
	ZXP(nout) = (x1 + x0) * 0.5f;
    }
    
    for (int i = 0; i!= mFilterRemain;++i)
    {
	x0 = ZXP(nin); 
	ZXP(nout) = (x0 + x2) * 0.5f;
	x2 = x1;
	x1 = x0;
    }
    m_x1 = x1;
    m_x2 = x2;
}

/* ------------------------ BRZ2 -------------------------------*/

class BRZ2_kr:
    public flext_base
{
    FLEXT_HEADER(BRZ2_kr,flext_base);
    
public:
    BRZ2_kr(int argc, t_atom *argv);
    
protected:
    void m_perform (float f);
    
private:
    float m_x1;
    float m_x2;
    FLEXT_CALLBACK_F(m_perform);
};

FLEXT_LIB_V("BRZ2",BRZ2_kr);

BRZ2_kr::BRZ2_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD(0,m_perform);

    AddOutFloat();

    m_x2 = m_x1 = 0;
}

void BRZ2_kr::m_perform(float f)
{
    ToOutFloat(0,(f + m_x2) * 0.5f);
    m_x2=m_x1;
    m_x1=f;
}

