/* sc4pd 
   DelayC~

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
     
   Coded while listening to: Tom Cora: Halleluja, Anyway
   
*/

#include "sc4pd.hpp"
#include "DelayUnit.hpp"

class DelayC_ar : private DelayUnit_ar
{
    FLEXT_HEADER(DelayC_ar,DelayUnit_ar);

    DelayC_ar (int argc, t_atom *argv);
    ~DelayC_ar ();
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out)
    {
	m_signal_fun(n,in,out);
    }
    
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out)
    {
	changed = false;
	DelayUnit_Reset();
    }

    void m_set(float f)
    {
	m_delaytime=f;
	changed = true;
    }

private:
    bool changed;
    DEFSIGCALL(m_signal_fun);
    DEFSIGFUN(m_signal_);
    DEFSIGFUN(m_signal_z);

    FLEXT_CALLBACK_F(m_set);
};

FLEXT_LIB_DSP_V("DelayC~",DelayC_ar);

DelayC_ar::DelayC_ar (int argc, t_atom *argv)
{

    FLEXT_ADDMETHOD_(0,"delaytime",m_set);


    //parse arguments
    AtomList Args(argc,argv);
    
    if (Args.Count() != 2)
	{
	    post("2 arguments are needed");
	    return;
	}
    
    m_delaytime = sc_getfloatarg(Args,0);
    m_maxdelaytime = sc_getfloatarg(Args,1);
    

    SETSIGFUN(m_signal_fun,SIGFUN(m_signal_z));
    
    AddOutSignal();
}

DelayC_ar::~DelayC_ar ()
{
    DelayUnit_Dtor();
}

void DelayC_ar::m_signal_z(int n, t_sample *const *in, t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    
    float *dlybuf = m_dlybuf;
    long iwrphase = m_iwrphase;
    float dsamp = m_dsamp;
    long mask = m_mask;
    float d0, d1, d2, d3;

    if (changed)
    {
	float next_dsamp = CalcDelay(m_delaytime);
	float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

	for (int i = 0; i!= n;++i)
	{
	    dsamp += dsamp_slope;
	    long idsamp = (long)dsamp;
	    float frac = dsamp - idsamp;
	    long irdphase1 = iwrphase - idsamp;
	    long irdphase2 = irdphase1 - 1;
	    long irdphase3 = irdphase1 - 2;
	    long irdphase0 = irdphase1 + 1;
	    
	    dlybuf[iwrphase & mask] = ZXP(nin);
	    if (irdphase0 < 0) 
	    {
		ZXP(nout) = 0.f;
	    } 
	    else
	    {
		if (irdphase1 < 0) 
		{
		    d1 = d2 = d3 = 0.f;
		    d0 = dlybuf[irdphase0 & mask];
		} 
		else if (irdphase2 < 0) 
		{
		    d1 = d2 = d3 = 0.f;
		    d0 = dlybuf[irdphase0 & mask];
		    d1 = dlybuf[irdphase1 & mask];
		} 
		else if (irdphase3 < 0) 
		{
		    d3 = 0.f;
		    d0 = dlybuf[irdphase0 & mask];
		    d1 = dlybuf[irdphase1 & mask];
		    d2 = dlybuf[irdphase2 & mask];
		} 
		else 
		{
		    d0 = dlybuf[irdphase0 & mask];
		    d1 = dlybuf[irdphase1 & mask];
		    d2 = dlybuf[irdphase2 & mask];
		    d3 = dlybuf[irdphase3 & mask];
		}
		ZXP(nout) = cubicinterp(frac, d0, d1, d2, d3);
	    }
	    iwrphase++;
	}
	m_dsamp = dsamp;
	changed = false;
    }
    else
    {
	long idsamp = (long)dsamp;
	float frac = dsamp - idsamp;
	for (int i = 0; i!= n;++i)
	{
	    long irdphase1 = iwrphase - idsamp;
	    long irdphase2 = irdphase1 - 1;
	    long irdphase3 = irdphase1 - 2;
	    long irdphase0 = irdphase1 + 1;
	    
	    dlybuf[iwrphase & mask] = ZXP(nin);
	    if (irdphase0 < 0) 
	    {
		ZXP(nout) = 0.f;
	    } 
	    else 
	    {
		if (irdphase1 < 0) 
		{
		    d1 = d2 = d3 = 0.f;
		    d0 = dlybuf[irdphase0 & mask];
		} 
		else if (irdphase2 < 0) 
		{
		    d1 = d2 = d3 = 0.f;
		    d0 = dlybuf[irdphase0 & mask];
		    d1 = dlybuf[irdphase1 & mask];
		} 
		else if (irdphase3 < 0) 
		{
		    d3 = 0.f;
		    d0 = dlybuf[irdphase0 & mask];
		    d1 = dlybuf[irdphase1 & mask];
		    d2 = dlybuf[irdphase2 & mask];
		} 
		else
		{
		    d0 = dlybuf[irdphase0 & mask];
		    d1 = dlybuf[irdphase1 & mask];
		    d2 = dlybuf[irdphase2 & mask];
		    d3 = dlybuf[irdphase3 & mask];
		}
		ZXP(nout) = cubicinterp(frac, d0, d1, d2, d3);
	    }
	    iwrphase++;
	}
    }
    
    m_iwrphase = iwrphase;

    m_numoutput += n;

    if (m_numoutput >= m_idelaylen) 
    {
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_));
    }
}

void DelayC_ar::m_signal_(int n, t_sample *const *in, t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;

    float *dlybuf = m_dlybuf;
    long iwrphase = m_iwrphase;
    float dsamp = m_dsamp;
    long mask = m_mask;

    if (changed)
    {
	float next_dsamp = CalcDelay(m_delaytime);
	float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);
	
	for (int i = 0; i!= n;++i)
	{
	    dlybuf[iwrphase & mask] = ZXP(nin);
	    dsamp += dsamp_slope;
	    long idsamp = (long)dsamp;
	    float frac = dsamp - idsamp;
	    long irdphase1 = iwrphase - idsamp;
	    long irdphase2 = irdphase1 - 1;
	    long irdphase3 = irdphase1 - 2;
	    long irdphase0 = irdphase1 + 1;
	    float d0 = dlybuf[irdphase0 & mask];
	    float d1 = dlybuf[irdphase1 & mask];
	    float d2 = dlybuf[irdphase2 & mask];
	    float d3 = dlybuf[irdphase3 & mask];
	    ZXP(nout) = cubicinterp(frac, d0, d1, d2, d3);
	    iwrphase++;
	}
	m_dsamp = dsamp;
	changed = false;
    }
    else
    {
	long idsamp = (long)dsamp;
	float frac = dsamp - idsamp;
	for (int i = 0; i!= n;++i)
	{
	    dlybuf[iwrphase & mask] = ZXP(nin);
	    long irdphase1 = iwrphase - idsamp;
	    long irdphase2 = irdphase1 - 1;
	    long irdphase3 = irdphase1 - 2;
	    long irdphase0 = irdphase1 + 1;
	    float d0 = dlybuf[irdphase0 & mask];
	    float d1 = dlybuf[irdphase1 & mask];
	    float d2 = dlybuf[irdphase2 & mask];
	    float d3 = dlybuf[irdphase3 & mask];
	    ZXP(nout) = cubicinterp(frac, d0, d1, d2, d3);
	    iwrphase++;
	}
    }
    m_iwrphase = iwrphase;
}

/* todo: DelayC for control rate ? */
