/* sc4pd 
   DelayN~

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
     
   Coded while listening to:

*/

#include "sc4pd.hpp"
#include "DelayUnit.hpp"

class DelayN_ar : private DelayUnit_ar
{
    FLEXT_HEADER(DelayN_ar,DelayUnit_ar);

    DelayN_ar (int argc, t_atom *argv);
    ~DelayN_ar ();
    
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

FLEXT_LIB_DSP_V("DelayN~",DelayN_ar);

DelayN_ar::DelayN_ar (int argc, t_atom *argv)
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

DelayN_ar::~DelayN_ar ()
{
    DelayUnit_Dtor();
}

void DelayN_ar::m_signal_z(int n, t_sample *const *in, t_sample *const *out)
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
	    dsamp += dsamp_slope;
	    long irdphase = iwrphase - (long)dsamp;
	    
	    dlybuf[iwrphase & mask] = ZXP(nin);
	    if (irdphase < 0) 
	    {
		ZXP(nout) = 0.f;
	    }
	    else
	    {
		ZXP(nout) = dlybuf[irdphase & mask];
	    }
	    iwrphase++;
	}
	m_dsamp = dsamp;
	changed = false;
    }
    else
    {
	long irdphase = iwrphase - (long)dsamp;
	float* dlybuf1 = dlybuf - ZOFF;
	float* dlyN    = dlybuf1 + m_idelaylen;
	long remain = n;

	while (remain) 
	{
	    float* dlywr = dlybuf1 + (iwrphase & mask);
	    float* dlyrd = dlybuf1 + (irdphase & mask);
	    long rdspace = dlyN - dlyrd;
	    long wrspace = dlyN - dlywr;
	    long nsmps = sc_min(rdspace, wrspace);
	    nsmps = sc_min(remain, nsmps);
	    remain -= nsmps;
	    if (irdphase < 0) 
	    {
		for (int i = 0; i!= nsmps;++i)
		{
		    ZXP(dlywr) = ZXP(nin);
		    ZXP(nout) = 0.f;
		}
	    } 
	    else
	    {
		for (int i = 0; i!= nsmps;++i)
		{
		    ZXP(dlywr) = ZXP(nin);
		    ZXP(nout) = ZXP(dlyrd);
		}
	    }
	    iwrphase += nsmps;
	    irdphase += nsmps;
	}
    }
    
    m_iwrphase = iwrphase;

    m_numoutput += n;

    if (m_numoutput >= m_idelaylen) 
    {
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_));
    }
}

void DelayN_ar::m_signal_(int n, t_sample *const *in, t_sample *const *out)
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
	    ++iwrphase;
	    long irdphase = iwrphase - (long)dsamp;
	    ZXP(nout) = dlybuf[irdphase & mask];
	}
	m_dsamp = dsamp;
	changed = false;
    }
    else
    {
	long irdphase = iwrphase - (long)dsamp;
	float* dlybuf1 = dlybuf - ZOFF;
	float* dlyrd   = dlybuf1 + (irdphase & mask);
	float* dlywr   = dlybuf1 + (iwrphase & mask);
	float* dlyN    = dlybuf1 + m_idelaylen;
	long remain = n;
	while (remain) 
	{
	    long rdspace = dlyN - dlyrd;
	    long wrspace = dlyN - dlywr;
	    long nsmps = sc_min(rdspace, wrspace);
	    nsmps = sc_min(remain, nsmps);
	    remain -= nsmps;
	    for (int i = 0; i!= nsmps;++i)
	    {
		ZXP(dlywr) = ZXP(nin);
		ZXP(nout) = ZXP(dlyrd);
	    }
	    if (dlyrd == dlyN) dlyrd = dlybuf1;
	    if (dlywr == dlyN) dlywr = dlybuf1;
	}
	iwrphase += n;
    }
    m_iwrphase = iwrphase;
}

/* todo: DelayN for control rate ? */
