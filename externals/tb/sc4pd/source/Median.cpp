/* sc4pd 
   Median, Median~

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
     
   Coded while listening to: Morton Feldman: For John Cage
   
*/

#include "sc4pd.hpp"

/* ------------------------ Median(~) ---------------------------------*/


const int kMAXMEDIANSIZE = 32;

class median_shared
{
public:

    inline void Init(long size, float value);
    inline float Insert(float value);
    
private:
    float m_medianValue[kMAXMEDIANSIZE];
    long m_medianAge[kMAXMEDIANSIZE];
    long m_medianSize, m_medianIndex;
};

void median_shared::Init(long size, float value)
{
    // initialize the arrays with the first value
    m_medianSize = size;
    for (int i=0; i<size; ++i) {
	m_medianValue[i] = value;
	m_medianAge[i] = i;
    }
}

inline float median_shared::Insert(float value)
{
    long i, last, pos=-1;
    
    // keeps a sorted list of the previous n=size values
    // the oldest is removed and the newest is inserted.
    // values between the oldest and the newest are shifted over by one.
    
    // values and ages are both arrays that are 'size' long.
    // the median value is always values[size>>1]
    
    last = m_medianSize - 1;
    // find oldest bin and age the other bins.
    for (i=0; i<m_medianSize; ++i) {
	if (m_medianAge[i] == last) { // is it the oldest bin ?
	    pos = i;	
	} else {
	    m_medianAge[i]++;	// age the bin
	}
    }
    // move values to fill in place of the oldest and make a space for the newest
    // search lower if value is too small for the open space
    while (pos != 0 && value < m_medianValue[pos-1]) {
	m_medianValue[pos] = m_medianValue[pos-1];
	m_medianAge[pos] = m_medianAge[pos-1];
	pos--;
    }
    // search higher if value is too big for the open space
    while (pos != last && value > m_medianValue[pos+1]) {
	m_medianValue[pos] = m_medianValue[pos+1];
	m_medianAge[pos] = m_medianAge[pos+1];
	pos++;
    }
    m_medianValue[pos] = value;
    m_medianAge[pos] = 0;		// this is the newest bin, age = 0
    return m_medianValue[m_medianSize>>1];
}


/* ------------------------ Median~ -----------------------------------*/

class Median_ar
    :public sc4pd_dsp
{
    FLEXT_HEADER(Median_ar,sc4pd_dsp);
    
public:
    Median_ar(int argc, t_atom * argv);

protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp (int n,t_signalvec const * insigs,
			t_signalvec const * outsigs);
    
    void m_set(float f)
    {
	m_size=(int)f;
	Median.Init(m_size,Median.Insert(0)); /* this is not beautiful, but i'm not
						 aware of a nicer way */
    }

private:
    FLEXT_CALLBACK_F(m_set);

    median_shared Median; //median
    int m_size;           //median size
};

FLEXT_LIB_DSP_V("Median~",Median_ar);

Median_ar::Median_ar(int argc,t_atom * argv)
{
    FLEXT_ADDMETHOD_(0,"set",m_set);

    AtomList Args(argc,argv);
    m_size=sc_getfloatarg(Args,0);
    
    AddOutSignal();
}

void Median_ar::m_dsp(int n,t_signalvec const * insigs,
		      t_signalvec const * outsigs)
{
    Median.Init(m_size,0); //how is this done in SuperCollider?
}

void Median_ar::m_signal(int n, t_sample *const *in, t_sample *const *out)
{
    t_sample *nout = *out;
    t_sample *nin = *in;
    
    for (int i = 0; i!= n;++i)
    {
	(*(nout)++) = Median.Insert(*(nin)++);
    }
}


/* ------------------------ Median ------------------------------------*/

class Median_kr
    :public sc4pd_dsp
{
    FLEXT_HEADER(Median_kr,sc4pd_dsp);
    
public:
    Median_kr(int argc, t_atom * argv);

protected:
    void m_set(float f)
    {
	m_size=(int)f;
	Median.Init(m_size,Median.Insert(0)); /* this is not beautiful, but i'm not
						 aware of a nicer way */
    }
    void m_perform(float f);

private:
    FLEXT_CALLBACK_F(m_set);
    FLEXT_CALLBACK_F(m_perform);

    median_shared Median; //median
    int m_size;           //median size
};

FLEXT_LIB_V("Median",Median_kr);

Median_kr::Median_kr(int argc,t_atom * argv)
{
    FLEXT_ADDMETHOD_(0,"set",m_set);
    FLEXT_ADDMETHOD(0,m_perform);


    AtomList Args(argc,argv);
    m_size=(int)sc_getfloatarg(Args,0);

    float m_set=sc_getfloatarg(Args,1);

    Median.Init(m_size,m_set);
    AddOutFloat();
}

void Median_kr::m_perform(float f)
{
    ToOutFloat(0,Median.Insert(f));
}
