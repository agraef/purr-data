/* sc4pd 
   DelayL~

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

class DelayL_ar : private DelayUnit_ar
{
    FLEXT_HEADER(DelayL_ar,DelayUnit_ar);

    DelayL_ar (int argc, t_atom *argv);
    ~DelayL_ar ();
    
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

FLEXT_LIB_DSP_V("DelayL~",DelayL_ar);

DelayL_ar::DelayL_ar (int argc, t_atom *argv)
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

DelayL_ar::~DelayL_ar ()
{
    DelayUnit_Dtor();
}

void DelayL_ar::m_signal_z(int n, t_sample *const *in, t_sample *const *out)
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
	    long idsamp = (long)dsamp;
	    float frac = dsamp - idsamp;
	    long irdphase = iwrphase - idsamp;
	    long irdphaseb = irdphase - 1;

	    dlybuf[iwrphase & mask] = ZXP(nin);
	    if (irdphase < 0) 
	    {
		ZXP(nout) = 0.f;
	    }
	    else if (irdphaseb < 0)
	    {
		float d1 = dlybuf[irdphase & mask];
		ZXP(nout) = d1 - frac * d1;
	    }
	    else
	    {
		float d1 = dlybuf[irdphase & mask];
		float d2 = dlybuf[irdphaseb & mask];
		ZXP(nout) = lininterp(frac, d1, d2);
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
	    long irdphase = iwrphase - idsamp;
	    long irdphaseb = irdphase - 1;
	    
	    dlybuf[iwrphase & mask] = ZXP(nin);
	    if (irdphase < 0) 
	    {
		ZXP(nout) = 0.f;
	    } 
	    else if (irdphaseb < 0) 
	    {
		float d1 = dlybuf[irdphase & mask];
		ZXP(nout) = d1 - frac * d1;
	    } 
	    else 
	    {
		float d1 = dlybuf[irdphase & mask];
		float d2 = dlybuf[irdphaseb & mask];
		ZXP(nout) = lininterp(frac, d1, d2);
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

void DelayL_ar::m_signal_(int n, t_sample *const *in, t_sample *const *out)
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
	    long irdphase = iwrphase - idsamp;
	    long irdphaseb = irdphase - 1;
	    float d1 = dlybuf[irdphase & mask];
	    float d2 = dlybuf[irdphaseb & mask];
	    ZXP(nout) = lininterp(frac, d1, d2);
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
	    long irdphase = iwrphase - idsamp;
	    long irdphaseb = irdphase - 1;
	    float d1 = dlybuf[irdphase & mask];
	    float d2 = dlybuf[irdphaseb & mask];
	    ZXP(nout) = lininterp(frac, d1, d2);
	    iwrphase++;
	}
	
    }
    m_iwrphase = iwrphase;
}

/* todo: DelayL for control rate ? */
