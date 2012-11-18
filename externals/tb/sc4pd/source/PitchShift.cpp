/* sc4pd 
   PitchShift~ 

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
     
   Coded while listening to: Bernhard Lang: Differenz / Wiederholung 2
   
*/

#include "sc4pd.hpp"

/* ------------------------ PitchShift~ -----------------------------*/

class PitchShift_ar
    :public sc4pd_dsp
{
    FLEXT_HEADER(PitchShift_ar,sc4pd_dsp);

public:
    PitchShift_ar(int argc,t_atom * argv);

protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out)
    {
	m_signal_fun(n,in,out);
    }
    
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);

    void m_set_pitchratio (float f)
    {
	m_pitchratio = f;
    }

    void m_set_pitchdispersion (float f)
    {
	m_pitchdispersion = f;
    }
    
    void m_set_timedispersion (float f)
    {
	m_timedispersion = f;
    }

private:
    float m_windowsize,m_pitchratio,m_pitchdispersion,m_timedispersion;
    RGen rgen;


    float *m_dlybuf;
    float m_dsamp1, m_dsamp1_slope, m_ramp1, m_ramp1_slope;
    float m_dsamp2, m_dsamp2_slope, m_ramp2, m_ramp2_slope;
    float m_dsamp3, m_dsamp3_slope, m_ramp3, m_ramp3_slope;
    float m_dsamp4, m_dsamp4_slope, m_ramp4, m_ramp4_slope;
    float m_fdelaylen, m_slope;
    long m_iwrphase, m_idelaylen, m_mask;
    long m_counter, m_stage, m_numoutput, m_framesize;

    DEFSIGCALL(m_signal_fun);
    DEFSIGFUN(m_signal_);
    DEFSIGFUN(m_signal_z);
    
    FLEXT_CALLBACK_F(m_set_pitchratio);
    FLEXT_CALLBACK_F(m_set_pitchdispersion);
    FLEXT_CALLBACK_F(m_set_timedispersion);
};

FLEXT_LIB_DSP_V("PitchShift~",PitchShift_ar);

PitchShift_ar::PitchShift_ar(int argc,t_atom * argv)
{
    FLEXT_ADDMETHOD_(0,"pitchRatio",m_set_pitchratio);
    FLEXT_ADDMETHOD_(0,"pitchDispersion",m_set_pitchdispersion);
    FLEXT_ADDMETHOD_(0,"timeDispersion",m_set_timedispersion);

    AtomList Args(argc,argv);

    if (Args.Count() != 4)
    {
	post("4 arguments needed");
	return;
    }

    m_windowsize = sc_getfloatarg(Args,0);
    m_pitchratio = sc_getfloatarg(Args,1);
    m_pitchdispersion = sc_getfloatarg(Args,2);
    m_timedispersion = sc_getfloatarg(Args,3);

    rgen.init(timeseed());
    
    AddOutSignal();
	
    SETSIGFUN(m_signal_fun,SIGFUN(m_signal_z));
}

void PitchShift_ar::m_dsp(int n, t_sample *const *in, t_sample *const *out)
{
    /* initialization from PitchShift_Ctor(PitchShift *unit) */

    long delaybufsize;
    float *dlybuf;
    float pchratio;
    float fdelaylen, slope;
    long framesize, last;
    
    //out = ZOUT(0);
    //in = ZIN(0);
    pchratio = m_pitchratio;
	
    delaybufsize = (long)ceil(m_windowsize * SAMPLERATE * 3.f + 3.f);
    fdelaylen = delaybufsize - 3;

    delaybufsize = delaybufsize + BUFLENGTH;
    delaybufsize = NEXTPOWEROFTWO(delaybufsize);  // round up to next power of two
    dlybuf = new float[delaybufsize];
         //(float*)RTAlloc(unit->mWorld, delaybufsize * sizeof(float));
	
    *dlybuf = 0;

    m_dlybuf = dlybuf;
    m_idelaylen = delaybufsize;
    m_fdelaylen = fdelaylen;
    m_iwrphase = 0;
    m_numoutput = 0;
    m_mask = last = (delaybufsize - 1);
	
    m_framesize = framesize = ((long)(m_windowsize * SAMPLERATE) + 2) & ~3;
    m_slope = slope = 2.f / framesize;
    m_stage = 3;
    m_counter = framesize >> 2;
    m_ramp1 = 0.5;
    m_ramp2 = 1.0;
    m_ramp3 = 0.5;
    m_ramp4 = 0.0;

    m_ramp1_slope = -slope;
    m_ramp2_slope = -slope;
    m_ramp3_slope =  slope;
    m_ramp4_slope =  slope;
    
    dlybuf[last  ] = 0.f; // put a few zeroes where we start the read heads
    dlybuf[last-1] = 0.f;
    dlybuf[last-2] = 0.f;
    
    m_numoutput = 0;

    // start all read heads 2 samples behind the write head
    m_dsamp1 = m_dsamp2 = m_dsamp3 = m_dsamp4 = 2.f;
    // pch ratio is initially zero for the read heads
    m_dsamp1_slope = m_dsamp2_slope = m_dsamp3_slope = m_dsamp4_slope = 1.f;
}


void PitchShift_ar::m_signal_z(int n, t_sample *const *in, 
		      t_sample *const *out)
{
    
    float *nout, *nin, *dlybuf;
    float disppchratio, pchratio, pchratio1, value;
    float dsamp1, dsamp1_slope, ramp1, ramp1_slope;
    float dsamp2, dsamp2_slope, ramp2, ramp2_slope;
    float dsamp3, dsamp3_slope, ramp3, ramp3_slope;
    float dsamp4, dsamp4_slope, ramp4, ramp4_slope;
    float fdelaylen, d1, d2, frac, slope, samp_slope, startpos, 
	pchdisp, timedisp;
    long remain, nsmps, idelaylen, irdphase, irdphaseb, iwrphase;
    long mask, idsamp;
    long counter, stage, framesize, numoutput;
	
    RGET;
		
    nout = *out;
    nin = *in;
    
    pchratio = m_pitchratio;
    pchdisp = m_pitchdispersion;
    timedisp = m_timedispersion;
    timedisp = sc_clip(timedisp, 0.f, m_windowsize) * SAMPLERATE;
	
    dlybuf = m_dlybuf;
    fdelaylen = m_fdelaylen;
    idelaylen = m_idelaylen;
    iwrphase = m_iwrphase;
    numoutput = m_numoutput;

    counter = m_counter;
    stage = m_stage;
    mask = m_mask;
    framesize = m_framesize;
    
    dsamp1 = m_dsamp1;
    dsamp2 = m_dsamp2;
    dsamp3 = m_dsamp3;
    dsamp4 = m_dsamp4;
	
    dsamp1_slope = m_dsamp1_slope;
    dsamp2_slope = m_dsamp2_slope;
    dsamp3_slope = m_dsamp3_slope;
    dsamp4_slope = m_dsamp4_slope;
	
    ramp1 = m_ramp1;
    ramp2 = m_ramp2;
    ramp3 = m_ramp3;
    ramp4 = m_ramp4;
    
    ramp1_slope = m_ramp1_slope;
    ramp2_slope = m_ramp2_slope;
    ramp3_slope = m_ramp3_slope;
    ramp4_slope = m_ramp4_slope;
    
    slope = m_slope;
	
    remain = n;
    while (remain) 
    {
	if (counter <= 0) 
	{
	    counter = framesize >> 2;
	    m_stage = stage = (stage + 1) & 3;
	    disppchratio = pchratio;
	    if (pchdisp != 0.f) 
	    {
		disppchratio += (pchdisp * frand2(s1,s2,s3));
	    }
	    disppchratio = sc_clip(disppchratio, 0.f, 4.f);
	    pchratio1 = disppchratio - 1.f;
	    samp_slope = -pchratio1;
	    startpos = pchratio1 < 0.f ? 2.f : framesize * pchratio1 + 2.f;
	    startpos += (timedisp * frand(s1,s2,s3));
	    switch(stage) 
	    {
	    case 0 :
		m_dsamp1_slope = dsamp1_slope = samp_slope;
		dsamp1 = startpos;
		ramp1 = 0.0;
		m_ramp1_slope = ramp1_slope =  slope;
		m_ramp3_slope = ramp3_slope = -slope;
		break;
	    case 1 :
		m_dsamp2_slope = dsamp2_slope = samp_slope;
		dsamp2 = startpos;
		ramp2 = 0.0;
		m_ramp2_slope = ramp2_slope =  slope;
		m_ramp4_slope = ramp4_slope = -slope;
		break;
	    case 2 :
		m_dsamp3_slope = dsamp3_slope = samp_slope;
		dsamp3 = startpos;
		ramp3 = 0.0;
		m_ramp3_slope = ramp3_slope =  slope;
		m_ramp1_slope = ramp1_slope = -slope;
		break;
	    case 3 :
		m_dsamp4_slope = dsamp4_slope = samp_slope;
		dsamp4 = startpos;
		ramp4 = 0.0;
		m_ramp2_slope = ramp2_slope = -slope;
		m_ramp4_slope = ramp4_slope =  slope;
		break;
	    }				
	}
	nsmps = sc_min(remain, counter);
	remain -= nsmps;
	counter -= nsmps;
	
	while (nsmps--) {
	    numoutput++;
	    iwrphase = (iwrphase + 1) & mask;
	    
	    dsamp1 += dsamp1_slope;
	    idsamp = (long)dsamp1;
	    frac = dsamp1 - idsamp;
	    irdphase = (iwrphase - idsamp) & mask;
	    irdphaseb = (irdphase - 1) & mask;
	    if (numoutput < idelaylen) 
	    {
		if (irdphase > iwrphase) 
		{
		    value = 0.f;
		} 
		else if (irdphaseb > iwrphase) 
		{
		    d1 = dlybuf[irdphase];
		    value = (d1 - frac * d1) * ramp1;
		} 
		else 
		{
		    d1 = dlybuf[irdphase];
		    d2 = dlybuf[irdphaseb];
		    value = (d1 + frac * (d2 - d1)) * ramp1;
		}
	    }
	    else
	    {
		d1 = dlybuf[irdphase];
		d2 = dlybuf[irdphaseb];
		value = (d1 + frac * (d2 - d1)) * ramp1;
	    }
	    ramp1 += ramp1_slope;
	    
	    dsamp2 += dsamp2_slope;
	    idsamp = (long)dsamp2;
	    frac = dsamp2 - idsamp;
	    irdphase = (iwrphase - idsamp) & mask;
	    irdphaseb = (irdphase - 1) & mask;
	    if (numoutput < idelaylen) 
	    {
		if (irdphase > iwrphase) 
		{
		    //value += 0.f;
		} 
		else if (irdphaseb > iwrphase) 
		{
		    d1 = dlybuf[irdphase];
		    value += (d1 - frac * d1) * ramp2;
		}
		else 
		{
		    d1 = dlybuf[irdphase];
		    d2 = dlybuf[irdphaseb];
		    value += (d1 + frac * (d2 - d1)) * ramp2;
		
		}
	    } 
	    else 
	    {
		d1 = dlybuf[irdphase];
		d2 = dlybuf[irdphaseb];
		value += (d1 + frac * (d2 - d1)) * ramp2;
	    }
	    ramp2 += ramp2_slope;
	    
	    dsamp3 += dsamp3_slope;
	    idsamp = (long)dsamp3;
	    frac = dsamp3 - idsamp;
	    irdphase = (iwrphase - idsamp) & mask;
	    irdphaseb = (irdphase - 1) & mask;
	    if (numoutput < idelaylen) 
	    {
		if (irdphase > iwrphase) 
		{
		    //value += 0.f;
		} 
		else if (irdphaseb > iwrphase) 
		{
		    d1 = dlybuf[irdphase];
		    value += (d1 - frac * d1) * ramp3;
		} 
		else 
		{
		    d1 = dlybuf[irdphase];
		    d2 = dlybuf[irdphaseb];
		    value += (d1 + frac * (d2 - d1)) * ramp3;
		}
	    }
	    else
	    {
		d1 = dlybuf[irdphase];
		d2 = dlybuf[irdphaseb];
		value += (d1 + frac * (d2 - d1)) * ramp3;
	    }
	    ramp3 += ramp3_slope;
			
	    dsamp4 += dsamp4_slope;
	    idsamp = (long)dsamp4;
	    frac = dsamp4 - idsamp;
	    irdphase = (iwrphase - idsamp) & mask;
	    irdphaseb = (irdphase - 1) & mask;
	    
	    if (numoutput < idelaylen) 
	    {
		if (irdphase > iwrphase) 
		{
		    //value += 0.f;
		} else if (irdphaseb > iwrphase) {
		    d1 = dlybuf[irdphase];
		    value += (d1 - frac * d1) * ramp4;
		}
		else 
		{
		    d1 = dlybuf[irdphase];
		    d2 = dlybuf[irdphaseb];
		    value += (d1 + frac * (d2 - d1)) * ramp4;
		}
	    }
	    else 
	    {
		d1 = dlybuf[irdphase];
		d2 = dlybuf[irdphaseb];
		value += (d1 + frac * (d2 - d1)) * ramp4;
	    }
	    ramp4 += ramp4_slope;
			
	    dlybuf[iwrphase] = ZXP(nin);
	    ZXP(nout) = value *= 0.5;
	}
    }
    
    m_counter = counter;
    m_stage = stage;
    m_mask = mask;
    
    m_dsamp1 = dsamp1;
    m_dsamp2 = dsamp2;
    m_dsamp3 = dsamp3;
    m_dsamp4 = dsamp4;
    
    m_ramp1 = ramp1;
    m_ramp2 = ramp2;
    m_ramp3 = ramp3;
    m_ramp4 = ramp4;
	
    m_numoutput = numoutput;
    m_iwrphase = iwrphase;
	
    if (numoutput >= idelaylen) 
    {
	SETSIGFUN(m_signal_fun,SIGFUN(m_signal_z));
    }
    RPUT;
}
    
void PitchShift_ar::m_signal_(int n, t_sample *const *in, 
		      t_sample *const *out)
{
    float *nout, *nin, *dlybuf;
    float disppchratio, pchratio, pchratio1, value;
    float dsamp1, dsamp1_slope, ramp1, ramp1_slope;
    float dsamp2, dsamp2_slope, ramp2, ramp2_slope;
    float dsamp3, dsamp3_slope, ramp3, ramp3_slope;
    float dsamp4, dsamp4_slope, ramp4, ramp4_slope;
    float fdelaylen, d1, d2, frac, slope, samp_slope, startpos, 
	pchdisp, timedisp;
    long remain, nsmps, idelaylen, irdphase, irdphaseb, iwrphase, mask, idsamp;
    long counter, stage, framesize;
    
    RGET;
    
    nout = *out;
    nin = *in;
    
    pchratio = m_pitchratio;
    pchdisp = m_pitchdispersion;
    timedisp = m_timedispersion;
    
    timedisp = sc_clip(timedisp, 0.f, m_windowsize) * SAMPLERATE;
    
    dlybuf = m_dlybuf;
    fdelaylen = m_fdelaylen;
    idelaylen = m_idelaylen;
    iwrphase = m_iwrphase;
    
    counter = m_counter;
    stage = m_stage;
    mask = m_mask;
    framesize = m_framesize;
    
    dsamp1 = m_dsamp1;
    dsamp2 = m_dsamp2;
    dsamp3 = m_dsamp3;
    dsamp4 = m_dsamp4;
    
    dsamp1_slope = m_dsamp1_slope;
    dsamp2_slope = m_dsamp2_slope;
    dsamp3_slope = m_dsamp3_slope;
    dsamp4_slope = m_dsamp4_slope;
    
    ramp1 = m_ramp1;
    ramp2 = m_ramp2;
    ramp3 = m_ramp3;
    ramp4 = m_ramp4;
    
    ramp1_slope = m_ramp1_slope;
    ramp2_slope = m_ramp2_slope;
    ramp3_slope = m_ramp3_slope;
    ramp4_slope = m_ramp4_slope;
    
    slope = m_slope;
    
    remain = n;
    while (remain) 
    {
	if (counter <= 0) 
	{
	    counter = framesize >> 2;
	    m_stage = stage = (stage + 1) & 3;
	    disppchratio = pchratio;
	    if (pchdisp != 0.f) 
	    {
		disppchratio += (pchdisp * frand2(s1,s2,s3));
	    }
	    disppchratio = sc_clip(disppchratio, 0.f, 4.f);
	    pchratio1 = disppchratio - 1.f;
	    samp_slope = -pchratio1;
	    startpos = pchratio1 < 0.f ? 2.f : framesize * pchratio1 + 2.f;
	    startpos += (timedisp * frand(s1,s2,s3));
	    switch(stage) 
	    {
	    case 0 :
		m_dsamp1_slope = dsamp1_slope = samp_slope;
		dsamp1 = startpos;
		ramp1 = 0.0;
		m_ramp1_slope = ramp1_slope =  slope;
		m_ramp3_slope = ramp3_slope = -slope;
		break;
	    case 1 :
		m_dsamp2_slope = dsamp2_slope = samp_slope;
		dsamp2 = startpos;
		ramp2 = 0.0;
		m_ramp2_slope = ramp2_slope =  slope;
		m_ramp4_slope = ramp4_slope = -slope;
		break;
	    case 2 :
		m_dsamp3_slope = dsamp3_slope = samp_slope;
		dsamp3 = startpos;
		ramp3 = 0.0;
		m_ramp3_slope = ramp3_slope =  slope;
		m_ramp1_slope = ramp1_slope = -slope;
		break;
	    case 3 :
		m_dsamp4_slope = dsamp4_slope = samp_slope;
		dsamp4 = startpos;
		ramp4 = 0.0;
		m_ramp2_slope = ramp2_slope = -slope;
		m_ramp4_slope = ramp4_slope =  slope;
		break;
	    }
	}
	
	nsmps = sc_min(remain, counter);
	remain -= nsmps;
	counter -= nsmps;
	
	for (int i = 0; i!= nsmps;++i)
	{
	    iwrphase = (iwrphase + 1) & mask;
	    
	    dsamp1 += dsamp1_slope;
	    idsamp = (long)dsamp1;
	    frac = dsamp1 - idsamp;
	    irdphase = (iwrphase - idsamp) & mask;
	    irdphaseb = (irdphase - 1) & mask;
	    d1 = dlybuf[irdphase];
	    d2 = dlybuf[irdphaseb];
	    value = (d1 + frac * (d2 - d1)) * ramp1;
	    ramp1 += ramp1_slope;
	    
	    dsamp2 += dsamp2_slope;
	    idsamp = (long)dsamp2;
	    frac = dsamp2 - idsamp;
	    irdphase = (iwrphase - idsamp) & mask;
	    irdphaseb = (irdphase - 1) & mask;
	    d1 = dlybuf[irdphase];
	    d2 = dlybuf[irdphaseb];
	    value += (d1 + frac * (d2 - d1)) * ramp2;
	    ramp2 += ramp2_slope;
	    
	    dsamp3 += dsamp3_slope;
	    idsamp = (long)dsamp3;
	    frac = dsamp3 - idsamp;
	    irdphase = (iwrphase - idsamp) & mask;
	    irdphaseb = (irdphase - 1) & mask;
	    d1 = dlybuf[irdphase];
	    d2 = dlybuf[irdphaseb];
	    value += (d1 + frac * (d2 - d1)) * ramp3;
	    ramp3 += ramp3_slope;
	
	    dsamp4 += dsamp4_slope;
	    idsamp = (long)dsamp4;
	    frac = dsamp4 - idsamp;
	    irdphase = (iwrphase - idsamp) & mask;
	    irdphaseb = (irdphase - 1) & mask;
	    d1 = dlybuf[irdphase];
	    d2 = dlybuf[irdphaseb];
	    value += (d1 + frac * (d2 - d1)) * ramp4;
	    ramp4 += ramp4_slope;
	    
	    dlybuf[iwrphase] = ZXP(nin);
	    ZXP(nout) = value *= 0.5;
	}
    }
    
    m_counter = counter;
    
    m_dsamp1 = dsamp1;
    m_dsamp2 = dsamp2;
    m_dsamp3 = dsamp3;
    m_dsamp4 = dsamp4;
    
    m_ramp1 = ramp1;
    m_ramp2 = ramp2;
    m_ramp3 = ramp3;
    m_ramp4 = ramp4;
    
    m_iwrphase = iwrphase;
    
    RPUT;
}
    

/* a control rate PitchShift doesn't make sense */
