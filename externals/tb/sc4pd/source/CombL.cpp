/* sc4pd 
   CombL~

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
     
   Coded while listening to: Butcher/Charles/Doerner: The Contest Of Pleasures

*/

#include "sc4pd.hpp"
#include "DelayUnit.hpp"

class CombL_ar : public FeedbackDelay_ar
{
    FLEXT_HEADER(CombL_ar,FeedbackDelay_ar);

    CombL_ar (int argc, t_atom *argv);
    ~CombL_ar ();
    
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

FLEXT_LIB_DSP_V("CombL~",CombL_ar);

CombL_ar::CombL_ar (int argc, t_atom *argv)
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

CombL_ar::~CombL_ar ()
{
    DelayUnit_Dtor();
}

void CombL_ar::m_signal_z(int n, t_sample *const *in, t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    
    float *dlybuf = m_dlybuf;
    long iwrphase = m_iwrphase;
    float dsamp = m_dsamp;
    float feedbk = m_feedbk;
    long mask = m_mask;
    
    if (delay_changed || decay_changed)
    {
	float next_dsamp = CalcDelay(m_delaytime);
	float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);
	
	float next_feedbk = CalcFeedback(m_delaytime, m_decaytime);
	float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
	
	for (int i = 0; i!= n;++i)
	{
	    dsamp += dsamp_slope;
	    long idsamp = (long)dsamp;
	    float frac = dsamp - idsamp;
	    long irdphase = iwrphase - idsamp;
	    long irdphaseb = irdphase - 1;
	    
	    float zin = ZXP(nin);
	    if (irdphase < 0) 
	    {
		dlybuf[iwrphase & mask] = zin;
		ZXP(nout) = 0.f;
	    } 
	    else if (irdphaseb < 0)
	    {
		float d1 = dlybuf[irdphase & mask];
		float value = d1 - frac * d1;
		dlybuf[iwrphase & mask] = zin + feedbk * value;
		ZXP(nout) = value;
	    }
	    else
	    {
		float d1 = dlybuf[irdphase & mask];
		float d2 = dlybuf[irdphaseb & mask];
		float value = lininterp(frac, d1, d2);
		dlybuf[iwrphase & mask] = zin + feedbk * value;
		ZXP(nout) = value;
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
	long idsamp = (long)dsamp;
	float frac = dsamp - idsamp;

	for (int i = 0; i!= n;++i)
	{
	    long irdphase = iwrphase - idsamp;
	    long irdphaseb = irdphase - 1;
	    
	    float zin = ZXP(nin);
	    if (irdphase < 0) 
	    {
		dlybuf[iwrphase & mask] = zin;
		ZXP(nout) = 0.f;
	    }
	    else if (irdphaseb < 0) 
	    {
		float d1 = dlybuf[irdphase & mask];
		float value = d1 - frac * d1;
		dlybuf[iwrphase & mask] = zin + feedbk * value;
		ZXP(nout) = value;
	    } 
	    else
	    {
		float d1 = dlybuf[irdphase & mask];
		float d2 = dlybuf[irdphaseb & mask];
		float value = lininterp(frac, d1, d2);
		dlybuf[iwrphase & mask] = zin + feedbk * value;
		ZXP(nout) = value;
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

void CombL_ar::m_signal_(int n, t_sample *const *in, t_sample *const *out)
{
    t_sample *nin = *in;
    t_sample *nout = *out;
    
    float *dlybuf = m_dlybuf;
    long iwrphase = m_iwrphase;
    float dsamp = m_dsamp;
    float feedbk = m_feedbk;
    long mask = m_mask;

    if(delay_changed || decay_changed)
    {
	float next_dsamp = CalcDelay(m_delaytime);
	float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);
	
	float next_feedbk = CalcFeedback(m_delaytime, m_decaytime);
	float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

	for(int i=0; i!= n;++i)
	{
	    dsamp += dsamp_slope;
	    long idsamp = (long)dsamp;
	    float frac = dsamp - idsamp;
	    long irdphase = iwrphase - idsamp;
	    long irdphaseb = irdphase - 1;
	    float d1 = dlybuf[irdphase & mask];
	    float d2 = dlybuf[irdphaseb & mask];
	    float value = lininterp(frac, d1, d2);
	    dlybuf[iwrphase & mask] = ZXP(nin) + feedbk * value;
	    ZXP(nout) = value;
	    feedbk += feedbk_slope;
	    iwrphase++;
	}
	m_feedbk = feedbk;
	m_dsamp = dsamp;
	delay_changed = decay_changed = false;
    }
    else
    {
	long idsamp = (long)dsamp;
	float frac = dsamp - idsamp;
	
	for(int i=0; i!= n;++i)
	{
	    long irdphase = iwrphase - idsamp;
	    long irdphaseb = irdphase - 1;
	    float d1 = dlybuf[irdphase & mask];
	    float d2 = dlybuf[irdphaseb & mask];
	    float value = lininterp(frac, d1, d2);
	    dlybuf[iwrphase & mask] = ZXP(nin) + feedbk * value;
	    ZXP(nout) = value;
	    iwrphase++;
	    
	}
    }
    m_iwrphase = iwrphase;
}

/* todo: CombL for control rate ? */
