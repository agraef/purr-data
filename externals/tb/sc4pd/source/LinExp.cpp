/* sc4pd 
   LinExp~, LinExp

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
     
   Coded while listening to: The Ex & Tom Cora - And The Weathermen Shrug 
                                                 Their Shoulders
   
*/

#include "sc4pd.hpp"


/* ------------------------ LinExp~ -----------------------------*/

class LinExp_ar
    :public sc4pd_dsp
{
    FLEXT_HEADER(LinExp_ar,sc4pd_dsp);

public:
    LinExp_ar(int argc,t_atom * argv);

protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);

    void m_set_srclo(float f)
    {
	m_srclo = f;
	m_reset();
    }

    void m_set_srchi(float f)
    {
	m_srchi = f;
	m_reset();
    }

    void m_set_dstlo(float f)
    {
	m_dstlo = f;
	m_reset();
    }

    void m_set_dsthi(float f)
    {
	m_dsthi = f;
	m_reset();
    }

    void m_reset()
    {
	m_dstratio = m_dsthi/m_dstlo;
	m_rsrcrange = 1. / (m_srchi - m_srclo);	
	m_rrminuslo = m_rsrcrange * -m_srclo;
    }

private:
    FLEXT_CALLBACK_F(m_set_srclo);
    FLEXT_CALLBACK_F(m_set_srchi);
    FLEXT_CALLBACK_F(m_set_dstlo);
    FLEXT_CALLBACK_F(m_set_dsthi);
    float m_dstratio, m_rsrcrange, m_rrminuslo, m_dstlo;
    float m_srclo, m_srchi, m_dsthi; //we will be able to reset the values
};

FLEXT_LIB_DSP_V("LinExp~",LinExp_ar);

LinExp_ar::LinExp_ar(int argc,t_atom * argv)
{
    FLEXT_ADDMETHOD_(0,"srclo",m_set_srclo);
    FLEXT_ADDMETHOD_(0,"srchi",m_set_srchi);
    FLEXT_ADDMETHOD_(0,"dstlo",m_set_dstlo);
    FLEXT_ADDMETHOD_(0,"dsthi",m_set_dsthi);

    AtomList Args(argc,argv);

    if (Args.Count() != 4)
	post("4 arguments are required");
    else
    {
	m_srclo = sc_getfloatarg(Args,0);
	m_srchi = sc_getfloatarg(Args,1);
	m_dstlo = sc_getfloatarg(Args,2);
	m_dsthi = sc_getfloatarg(Args,3);
	
	m_reset();
	AddOutSignal();
    }
}

void LinExp_ar::m_signal(int n, t_sample *const *in, 
				t_sample *const *out)
{
    t_sample *nout = *out;
    t_sample *nin = *in;

    float dstlo =  m_dstlo;
    float dstratio = m_dstratio;
    float rsrcrange = m_rsrcrange;
    float rrminuslo = m_rrminuslo;

    for (int i = 0; i!= n;++i) 
    {
	ZXP(nout) = dstlo * pow(dstratio, ZXP(nin) * rsrcrange + rrminuslo);
    }
}

/* ------------------------ LinExp ------------------------------*/

class LinExp_kr
    :public flext_base
{
    FLEXT_HEADER(LinExp_kr,flext_base);

public:
    LinExp_kr(int argc,t_atom * argv);

protected:
    void m_perform(float f);

    void m_set_srclo(float f)
    {
	m_srclo = f;
	m_reset();
    }

    void m_set_srchi(float f)
    {
	m_srchi = f;
	m_reset();
    }

    void m_set_dstlo(float f)
    {
	m_dstlo = f;
	m_reset();
    }

    void m_set_dsthi(float f)
    {
	m_dsthi = f;
	m_reset();
    }

    void m_reset()
    {
	m_dstratio = m_dsthi/m_dstlo;
	m_rsrcrange = 1. / (m_srchi - m_srclo);	
	m_rrminuslo = m_rsrcrange * -m_srclo;
    }
    
private:
    FLEXT_CALLBACK_F(m_set_srclo);
    FLEXT_CALLBACK_F(m_set_srchi);
    FLEXT_CALLBACK_F(m_set_dstlo);
    FLEXT_CALLBACK_F(m_set_dsthi);
    float m_dstratio, m_rsrcrange, m_rrminuslo, m_dstlo;
    float m_srclo, m_srchi, m_dsthi; //we will be able to reset the values

    FLEXT_CALLBACK_F(m_perform);
};

FLEXT_LIB_V("LinExp",LinExp_kr);

LinExp_kr::LinExp_kr(int argc,t_atom * argv)
{
    FLEXT_ADDMETHOD(0,m_perform);
    FLEXT_ADDMETHOD_(0,"srclo",m_set_srclo);
    FLEXT_ADDMETHOD_(0,"srchi",m_set_srchi);
    FLEXT_ADDMETHOD_(0,"dstlo",m_set_dstlo);
    FLEXT_ADDMETHOD_(0,"dsthi",m_set_dsthi);

    AtomList Args(argc,argv);

    if (Args.Count() != 4)
	post("4 arguments are required");
    else
    {
	m_srclo = sc_getfloatarg(Args,0);
	m_srchi = sc_getfloatarg(Args,1);
	m_dstlo = sc_getfloatarg(Args,2);
	m_dsthi = sc_getfloatarg(Args,3);
	
	m_reset();
    
	AddInFloat();
	AddOutFloat();
    }
}

void LinExp_kr::m_perform(float f)
{
    ToOutFloat(0,m_dstlo * pow(m_dstratio, f * m_rsrcrange + m_rrminuslo));
}
