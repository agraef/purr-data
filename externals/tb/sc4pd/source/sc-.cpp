/* sc4pd 
   sc-~

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


/* ------------------------ sc-~ -------------------------------*/

class scsub_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(scsub_ar,sc4pd_dsp);
    
public:
    scsub_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);

    void m_set(float f)
    {
	m_nextsubtrahend = f;
	changed = true;
    }
    
    float m_nextsubtrahend, m_subtrahend;
    bool changed;
    bool invert;
    
private:
    FLEXT_CALLBACK_1(m_set,float);
};

FLEXT_LIB_DSP_V("sc-~",scsub_ar);

scsub_ar::scsub_ar(int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD(1,m_set);

    //parse arguments
    AtomList Args(argc,argv);

    m_subtrahend = sc_getfloatarg(Args,0);

    invert = sc_inv(Args);

    AddInSignal("signal");
    AddInFloat("scalar");
    AddOutSignal();
    
    changed = false;
}    

void scsub_ar::m_signal(int n, t_sample *const *in, 
			    t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;

    if (invert)
    {
	if (changed)
	{
	    float xb = m_nextsubtrahend;
	    float slope =  CALCSLOPE(xb, m_subtrahend);
	    for (int i = 0; i!=n; ++i)
	    {
		ZXP(nout) = xb - ZXP(nin);
		xb += slope;
	    }
	    m_subtrahend = xb;
	    changed = false;
	}
	else
	{
	    float xb = m_subtrahend;
	    
	    for (int i = 0; i!=n; ++i)
	    {
		ZXP(nout) = xb - ZXP(nin);
	    }
	}
    }
    else
    {
	if (changed)
	{
	    float xb = m_nextsubtrahend;
	    float slope =  CALCSLOPE(xb, m_subtrahend);
	    for (int i = 0; i!=n; ++i)
	    {
		ZXP(nout) = ZXP(nin) - xb;
		xb += slope;
	    }
	    m_subtrahend = xb;
	    changed = false;
	}
	else
	{
	    float xb = m_subtrahend;
	    
	    for (int i = 0; i!=n; ++i)
	    {
		ZXP(nout) = ZXP(nin) - xb;
	    }
	}
    }
}

