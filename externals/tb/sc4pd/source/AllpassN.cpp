/* sc4pd 
   AllpassN~

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
     
   Coded while listening to: Efzeg: Boogie

*/

#include "sc4pd.hpp"
#include "DelayUnit.hpp"

class AllpassN_ar : public FeedbackDelay_ar
{
    FLEXT_HEADER(AllpassN_ar,FeedbackDelay_ar);

    AllpassN_ar (int argc, t_atom *argv);
    ~AllpassN_ar ();
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out)
    {
	m_signal_fun(n,in,out);
    }
    
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out)
    {
	delay_changed = decay_changed = false;
	FeedbackDelay_Reset();
    }

    void m_delay(float f)
    {
	m_delaytime=f;
	delay_changed = true;
    }

    void m_decay(float f)
    {
	m_decaytime=f;
	decay_changed = true;
    }

private:
    bool delay_changed, decay_changed;
    DEFSIGCALL(m_signal_fun);
    DEFSIGFUN(m_signal_);
    DEFSIGFUN(m_signal_z);

    FLEXT_CALLBACK_F(m_delay);
    FLEXT_CALLBACK_F(m_decay);
};

FLEXT_LIB_DSP_V("AllpassN~",AllpassN_ar);

AllpassN_ar::AllpassN_ar (int argc, t_atom *argv)
{
    FLEXT_ADDMETHOD_(0,"delaytime",m_delay);
    FLEXT_ADDMETHOD_(0,"decaytime",m_decay);

    //parse arguments
    AtomList Args(argc,argv);
    
    if (Args.Count() != 3)
	{
	    post("3 arguments are needed");
	    return;
	}
    
    m_maxdelaytime = sc_getfloatarg(Args,0);
    m_delaytime = sc_getfloatarg(Args,1);
    m_decaytime = sc_getfloatarg(Args,2);
    
    SETSIGFUN(m_signal_fun,SIGFUN(m_signal_z));
    
    AddOutSignal();
}

AllpassN_ar::~AllpassN_ar ()
{
    DelayUnit_Dtor();
}

void AllpassN_ar::m_signal_z(int n, t_sample *const *in, t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    
    float *dlybuf = m_dlybuf;
    long iwrphase = m_iwrphase;
    float dsamp = m_dsamp;
    float feedbk = m_feedbk;
    long mask = m_mask;
    
    if (delay_changed)
    {
	float next_dsamp = CalcDelay(m_delaytime);
	float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);
	
	float next_feedbk = CalcFeedback(m_delaytime, m_decaytime);
	float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
	
	for (int i = 0; i!= n;++i)
	{
	    dsamp += dsamp_slope;
	    long irdphase = iwrphase - (long)dsamp;
	    
	    if (irdphase < 0) 
	    {
		float dwr = ZXP(nin);
		dlybuf[iwrphase & mask] = dwr;
		ZXP(nout) = -feedbk * dwr;
	    } 
	    else
	    {
		float value = dlybuf[irdphase & mask];
		float dwr = feedbk * value + ZXP(nin);
		dlybuf[iwrphase & mask] = dwr;
		ZXP(nout) = value - feedbk * dwr;
	    }
	    feedbk += feedbk_slope;
	    iwrphase++;
	}
	m_feedbk = feedbk;
	m_dsamp = dsamp;
	delay_changed = decay_changed = false;
    }
    else
    {
	long irdphase = iwrphase - (long)dsamp;
	float* dlybuf1 = dlybuf - ZOFF;
	float* dlyN    = dlybuf1 + m_idelaylen;
	if (decay_changed)
	{
	    float next_feedbk = CalcFeedback(m_delaytime, m_decaytime);
	    float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
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
		    dlyrd += nsmps;
		    for (int i = 0; i!= nsmps;++i)
		    {
			float dwr = ZXP(nin);
			ZXP(dlywr) = dwr;
			ZXP(nout) = -feedbk * dwr;
			feedbk += feedbk_slope;
		    }
		} 
		else 
		{
		    for (int i = 0; i!= nsmps;++i)
		    {
			float x1 = ZXP(dlyrd);
			float dwr = x1 * feedbk + ZXP(nin);
			ZXP(dlywr) = dwr;
			ZXP(nout) = x1 - feedbk * dwr;
			feedbk += feedbk_slope;
		    }
		}
		iwrphase += nsmps;
		irdphase += nsmps;
	    }
	    decay_changed=false;
	}
	else
	{
	    long remain = n;
	    while (remain) 
	    {
		float* dlyrd = dlybuf1 + (irdphase & mask);
		float* dlywr = dlybuf1 + (iwrphase & mask);
		long rdspace = dlyN - dlyrd;
		long wrspace = dlyN - dlywr;
		long nsmps = sc_min(rdspace, wrspace);
		nsmps = sc_min(remain, nsmps);
		remain -= nsmps;
		
		if (irdphase < 0) 
		{
		    feedbk = -feedbk;
		    for (int i = 0; i!= nsmps;++i)
		    {
			float dwr = ZXP(nin);
			ZXP(dlywr) = dwr;
			ZXP(nout) = feedbk * dwr;
		    }
		    feedbk = -feedbk;
		}
		else 
		{
		    for (int i = 0; i!= nsmps;++i)
		    {
			float x1 = ZXP(dlyrd);
			float dwr = x1 * feedbk + ZXP(nin);
			ZXP(dlywr) = dwr;
			ZXP(nout) = x1 - feedbk * dwr;
		    }
		}
		iwrphase += nsmps;
		irdphase += nsmps;
	    }
	    m_feedbk = feedbk;
	}
    }
    
    m_iwrphase = iwrphase;
    m_numoutput += n;
    if (m_numoutput >= m_idelaylen)
    {
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_));
    }
}

void AllpassN_ar::m_signal_(int n, t_sample *const *in, t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    
    float *dlybuf = m_dlybuf;
    long iwrphase = m_iwrphase;
    float dsamp = m_dsamp;
    float feedbk = m_feedbk;
    long mask = m_mask;

    if(delay_changed)
    {
	float next_dsamp = CalcDelay(m_delaytime);
	float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);
	
	float next_feedbk = CalcFeedback(m_delaytime, m_decaytime);
	float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

	for(int i=0; i!= n;++i)
	{
	    dsamp += dsamp_slope;
	    ++iwrphase;
	    long irdphase = iwrphase - (long)dsamp;
	    float value = dlybuf[irdphase & mask];
	    float dwr = value * feedbk + ZXP(nin);
	    dlybuf[iwrphase & mask] = dwr;
	    ZXP(nout) = value - feedbk * dwr;
	    feedbk += feedbk_slope;
	}
	m_feedbk = feedbk;
	m_dsamp = dsamp;
	delay_changed = decay_changed = false;
    }
    else
    {
	long irdphase = iwrphase - (long)dsamp;
	float* dlybuf1 = dlybuf - ZOFF;
	float* dlyrd   = dlybuf1 + (irdphase & mask);
	float* dlywr   = dlybuf1 + (iwrphase & mask);
	float* dlyN    = dlybuf1 + m_idelaylen;
	
	if(decay_changed)
	{
	    float next_feedbk = CalcFeedback(m_delaytime, m_decaytime);
	    float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
	    long remain = n;
	    while (remain)
	    {
		long rdspace = dlyN - dlyrd;
		long wrspace = dlyN - dlywr;
		long nsmps = sc_min(rdspace, wrspace);
		nsmps = sc_min(remain, nsmps);
		remain -= nsmps;
		
		for(int i=0; i!= nsmps;++i)
		{
		    float value = ZXP(dlyrd);
		    float dwr = value * feedbk + ZXP(nin);
		    ZXP(dlywr) = dwr;
		    ZXP(nout) = value - feedbk * dwr;
		    feedbk += feedbk_slope;
		}
		if (dlyrd == dlyN) dlyrd = dlybuf1;
		if (dlywr == dlyN) dlywr = dlybuf1;
	    }
	    m_feedbk = feedbk;
	    decay_changed = false;
	}
	else
	{
	    long remain = n;
	    while (remain) 
	    {
		long rdspace = dlyN - dlyrd;
		long wrspace = dlyN - dlywr;
		long nsmps = sc_min(rdspace, wrspace);
		nsmps = sc_min(remain, nsmps);
		remain -= nsmps;
		
		for (int i = 0; i!= nsmps; ++i)
		{
		    float value = ZXP(dlyrd);
		    float dwr = value * feedbk + ZXP(nin);
		    ZXP(dlywr) = dwr;
		    ZXP(nout) = value - feedbk * dwr;
		}
		if (dlyrd == dlyN) dlyrd = dlybuf1;
		if (dlywr == dlyN) dlywr = dlybuf1;
	    }

	}
	iwrphase += n;
    }
    m_iwrphase = iwrphase;
}

/* todo: AllpassN for control rate ? */
