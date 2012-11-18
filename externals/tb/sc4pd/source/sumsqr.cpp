/* sc4pd 
   sumsqr, sumsqr~

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
     
   Coded while listening to: Evan Parker & Keith Rowe: Dark Rags
   
*/

#include "sc4pd.hpp"


inline float sc_sumsqr (float a, float b)
{
    return a*a+b*b;
}


/* ------------------------ sumsqr~ -----------------------------*/

class sumsqr_ar
    :public sc4pd_dsp
{
    FLEXT_HEADER(sumsqr_ar,sc4pd_dsp);

public:
    sumsqr_ar(int argc,t_atom * argv);

protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);

private:
    
};

FLEXT_LIB_DSP_V("sumsqr~",sumsqr_ar);

sumsqr_ar::sumsqr_ar(int argc,t_atom * argv)
{
    AddInSignal();
    AddInSignal();
    AddOutSignal();
}

void sumsqr_ar::m_signal(int n, t_sample *const *in, 
				t_sample *const *out)
{
    t_sample *nout = *out;
    t_sample *nin1 = *in;
    t_sample *nin2 = *(in+1);

    for (int i = 0; i!= n;++i)
    {
	if( *nin2 > 0)
	    (*(nout)++) = sc_sumsqr( (*(nin1)++), (*(nin2)++) );
    }
}



/* ------------------------ sumsqr ------------------------------*/

class sumsqr_kr
    :public flext_base
{
    FLEXT_HEADER(sumsqr_kr,flext_base);

public:
    sumsqr_kr(int argc,t_atom * argv);

protected:
    void m_perform(float f);
    void m_set(float f);
    
private:
    float b;
    FLEXT_CALLBACK_F(m_perform);
    FLEXT_CALLBACK_F(m_set);
};

FLEXT_LIB_V("sumsqr",sumsqr_kr);

sumsqr_kr::sumsqr_kr(int argc,t_atom * argv)
    :b(0)
{

    AddInFloat();
    AddInFloat();
    AddOutFloat();

    FLEXT_ADDMETHOD(0,m_perform);
    FLEXT_ADDMETHOD(1,m_set);
}

void sumsqr_kr::m_perform(float f)
{
    ToOutFloat(0,sc_sumsqr(f,b));
}

void sumsqr_kr::m_set(float f)
{
    b=f;
}
