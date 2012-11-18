/* sc4pd 
   LPZ2~

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

/* ------------------------ LPZ2~ -------------------------------*/

class LPZ2_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(LPZ2_ar,sc4pd_dsp);
    
public:
    LPZ2_ar(int argc, t_atom *argv);
    
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

FLEXT_LIB_DSP_V("LPZ2~",LPZ2_ar);

LPZ2_ar::LPZ2_ar(int argc, t_atom *argv)
{
    AddOutSignal();

    m_x1=0;
    m_x2=0;
}

void LPZ2_ar::m_signal(int n, t_sample *const *in, 
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
	ZXP(nout) = (x0 + 2.f * x1 + x2) * 0.25f;
	x2 = ZXP(nin);
	ZXP(nout) = (x2 + 2.f * x0 + x1) * 0.25f;
	x1 = ZXP(nin);
	ZXP(nout) = (x1 + 2.f * x2 + x0) * 0.25f;
    }
    
    for (int i = 0; i!= mFilterRemain;++i)
    {
	x0 = ZXP(nin); 
	ZXP(nout) = (x0 + 2.f * x1 + x2) * 0.25f;
	x2 = x1;
	x1 = x0;
    }
    m_x1 = x1;
    m_x2 = x2;
}

/* ------------------------ LPZ2 -------------------------------*/

class LPZ2_kr:
    public flext_base
{
    FLEXT_HEADER(LPZ2_kr,flext_base);
    
public:
    LPZ2_kr(int argc, t_atom *argv);
    
protected:
    void m_perform (float f);
    
private:
    float m_x1;
    float m_x2;
    FLEXT_CALLBACK_F(m_perform);
};

FLEXT_LIB_V("LPZ2",LPZ2_kr);

LPZ2_kr::LPZ2_kr(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD(0,m_perform);

    AddOutFloat();

    m_x2 = m_x1 = 0;
}

void LPZ2_kr::m_perform(float f)
{
    ToOutFloat(0,(f + 2.f * m_x1 + m_x2) * 0.25f);
    m_x2=m_x1;
    m_x1=f;
}

