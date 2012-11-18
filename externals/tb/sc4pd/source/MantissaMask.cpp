/* sc4pd 
   MantissaMask~, MantissaMask

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
     
   Coded while listening to: Wolfgang Mitterer: Amusie
   
*/

#include "sc4pd.hpp"

/* ------------------------ MantissaMask~ -----------------------------*/

class MantissaMask_ar
    :public sc4pd_dsp
{
    FLEXT_HEADER(MantissaMask_ar,sc4pd_dsp);

public:
    MantissaMask_ar(int argc,t_atom * argv);

protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    void m_set(float f);

private:
    FLEXT_CALLBACK_F(m_set);
    int32 mask;
    
};

FLEXT_LIB_DSP_V("MantissaMask~",MantissaMask_ar);

MantissaMask_ar::MantissaMask_ar(int argc,t_atom * argv)
{
    FLEXT_ADDMETHOD_(0,"set",m_set);

    AtomList Args(argc,argv);
    int bits = sc_getfloatarg(Args,0);

    AddOutSignal();

    mask = -1 << (23 - bits);
    
}

void MantissaMask_ar::m_signal(int n, t_sample *const *in, 
				t_sample *const *out)
{
    int32 *nout = (int32*)*out;
    int32 *nin = (int32*)*in;

    for (int i = 0; i!= n;++i)
    {
	(*(nout)++) = mask & (*(nin)++);
    }
}

void MantissaMask_ar::m_set(float f)
{
    int bits = (int) f;
    if ( bits < 0 || bits > 23)
    {
	post("value doesn't make sense");
	return;
    }
    mask = -1 << (23 - bits);
}

/* ------------------------ MantissaMask ------------------------------*/

class MantissaMask_kr
    :public flext_base
{
    FLEXT_HEADER(MantissaMask_kr,flext_base);

public:
    MantissaMask_kr(int argc,t_atom * argv);

protected:
    void m_set(float f);
    void m_perform(float f);
    
private:
    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK_F(m_perform);
    int32 mask;
};

FLEXT_LIB_V("MantissaMask",MantissaMask_kr);

MantissaMask_kr::MantissaMask_kr(int argc,t_atom * argv)
{

    AtomList Args(argc,argv);
    int bits = sc_getfloatarg(Args,0);

    AddInFloat();
    AddOutFloat();

    FLEXT_ADDMETHOD(0,m_perform);
    FLEXT_ADDMETHOD_(0,"set",m_set);
    mask = -1 << (23 - bits);
}

void MantissaMask_kr::m_perform(float f)
{
    float g;
    int32 *gp = (int32*) &g;
    
    *(gp) = mask &  *((int32*) &f);

    ToOutFloat(0,g);
}

void MantissaMask_kr::m_set(float f)
{
    int bits = (int) f;
    if ( bits < 0 || bits > 23)
    {
	post("value doesn't make sense");
	return;
    }
    mask = -1 << (23 - bits);
}
