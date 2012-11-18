/* sc4pd 
   sc*~

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
     
   Coded while listening to: Phil Minton & Veryan Weston: Ways
   
*/

#include "sc4pd.hpp"


/* ------------------------ sc*~ -------------------------------*/

class scmul_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(scmul_ar,sc4pd_dsp);
    
public:
    scmul_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);

    void m_set(float f)
    {
	m_nextfactor = f;
	changed = true;
    }
    
    float m_nextfactor, m_factor;
    bool changed;
    
private:
    FLEXT_CALLBACK_1(m_set,float);
};

FLEXT_LIB_DSP_V("sc*~",scmul_ar);

scmul_ar::scmul_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD(1,m_set);

    //parse arguments
    AtomList Args(argc,argv);

    m_factor = sc_getfloatarg(Args,0);

    AddInSignal("signal");
    AddInFloat("scalar");
    AddOutSignal();
    
    changed = false;
}    

void scmul_ar::m_signal(int n, t_sample *const *in, 
			    t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;

    if (changed)
    {
	float xb = m_nextfactor;
	float slope =  CALCSLOPE(xb, m_factor);
	for (int i = 0; i!=n; ++i)
	{
	    ZXP(nout) = ZXP(nin) * xb;
	    xb += slope;
	}
	m_factor = xb;
	changed = false;
    }
    else
    {
	float xb = m_factor;

	for (int i = 0; i!=n; ++i)
	{
	    ZXP(nout) = ZXP(nin) * xb;
	}
    }
}


