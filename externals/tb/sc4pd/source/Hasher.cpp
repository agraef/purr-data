/* sc4pd 
   Hasher~, Hasher

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
     
   Coded while listening to: Philip Jeck: Stoke
   
*/

#include "sc4pd.hpp"

/* ------------------------ Hasher~ -----------------------------------*/

class Hasher_ar
    :public sc4pd_dsp
{
    FLEXT_HEADER(Hasher_ar,sc4pd_dsp);

public:
    Hasher_ar();

protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
private:
    
};

FLEXT_LIB_DSP("Hasher~",Hasher_ar);

Hasher_ar::Hasher_ar()
{
    AddOutSignal();
}

void Hasher_ar::m_signal(int n, t_sample *const *in, 
				t_sample *const *out)
{
    int32 *nin = (int32*)*in;
    t_sample *nout = *out;

    for (int i = 0; i!= n;++i)
    {
	union { float f; int i; } u;
	int z = (*(nin)++);
	u.i = 0x40000000 | ((uint32)Hash(z) >> 9);
	(*(nout)++) = u.f -3.f;
    }
}


/* ------------------------ Hasher ------------------------------------*/

class Hasher_kr
    :public flext_base
{
    FLEXT_HEADER(Hasher_kr,flext_base);

public:
    Hasher_kr();

protected:
    void m_perform(float f);
    
private:
    FLEXT_CALLBACK_F(m_perform);
};

FLEXT_LIB("Hasher",Hasher_kr);

Hasher_kr::Hasher_kr()
{
    AddInFloat();
    AddOutFloat();

    FLEXT_ADDMETHOD(0,m_perform);
}

void Hasher_kr::m_perform(float f)
{
    int32 * fi = (int32*) &f;
    
    union { float f; int i; } u;
    int z = *fi;
    u.i = 0x40000000 | ((uint32)Hash(z) >> 9);

    ToOutFloat(0,u.f -3.f);
}
