/* 
idelay~ - interpolating delay

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example for usage of flext
It's a simple interpolating delay with signal input with allows for glitchless change of delay times
Watch out for Doppler effects!

*/

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error You need at least flext version 0.4.1 
#endif

// template class for delay line
#include "delay.h"

class idelay:
	public flext_dsp
{
	FLEXT_HEADER_S(idelay,flext_dsp,setup)
 
public:
	idelay(int nsamps)
    { 
    	AddInSignal("Audio In");  // audio in 
    	AddInSignal("Delay time (ms)");  // delay time
    	AddOutSignal("Audio Out");  // audio out

        m_resize(nsamps);
    }

protected:
	virtual void CbSignal()
    {
        int n = Blocksize();
    	t_sample const *in = InSig(0),*del = InSig(1);
    	t_sample *out = OutSig(0);

        for(int i = 0; i < n; ++i) {
    		dline.Put(in[i]);
        	out[i] = dline.Get(del[i]);
        }
    }

    void m_resize(int sz) { if(sz >= 0) dline.SetLen(sz); }

    void m_clear() { dline.Clear(); }
	
private:
    static void setup(t_classid c)
    {
        FLEXT_CADDMETHOD_I(c,0,"resize",m_resize);
        FLEXT_CADDMETHOD_(c,0,"clear",m_clear);
    }
    
    FLEXT_CALLBACK_I(m_resize)
    FLEXT_CALLBACK(m_clear)

	DelayLine<t_sample> dline;
};

// make implementation of a tilde object with one float arg
FLEXT_NEW_DSP_1("idelay~",idelay,float0)
