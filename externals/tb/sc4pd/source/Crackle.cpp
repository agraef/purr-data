/* sc4pd 
   Crackle, Crackle~

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
     
   Coded while listening to: David S. Ware: Threads
   
*/

#include "sc4pd.hpp"

/* ------------------------ Crackle~ -------------------------------*/

class Crackle_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(Crackle_ar,sc4pd_dsp);
    
public:
    Crackle_ar(int argc, t_atom *argv);
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    
    void m_set(float f)
    {
	m_paramf = f;
    }

private:
    FLEXT_CALLBACK_F(m_set);
    float m_paramf;
    float m_y1, m_y2;
};

FLEXT_LIB_DSP_V("Crackle~",Crackle_ar);

Crackle_ar::Crackle_ar(int argc, t_atom *argv)
    : m_y1(0.3f),m_y2(0.f)
{
    FLEXT_ADDMETHOD_(0,"parameter",m_set);
    //parse arguments
    AtomList Args(argc,argv);
    m_paramf=sc_getfloatarg(Args,0);
    
    if (m_paramf == 0)
	m_paramf = 1;

    AddOutSignal();
}    


void Crackle_ar::m_signal(int n, t_sample *const *in, 
			       t_sample *const *out)
{
    t_sample *nout = *out;
    
    float paramf = m_paramf;
    float y1 = m_y1;
    float y2 = m_y2;
    float y0;

    for (int i = 0; i!= n;++i)
    {
	(*(nout)++) = y0 = fabs(y1 * paramf - y2 - 0.05f);
	y2 = y1; 
	y1 = y0;
    }

    m_y1=y1;
    m_y2=y2;
}


/* ------------------------ Crackle ---------------------------------*/

class Crackle_kr:
    public flext_base
{
    FLEXT_HEADER(Crackle_kr,flext_base);

public:
    Crackle_kr(int argc, t_atom *argv);
    
protected:
    void m_perform();

    void m_set(float f)
    {
	m_paramf = f;
    }

private:
    float m_paramf;
    float m_y1, m_y2;
    FLEXT_CALLBACK(m_perform);
    FLEXT_CALLBACK_F(m_set);
};

FLEXT_LIB_V("Crackle",Crackle_kr);

Crackle_kr::Crackle_kr(int argc, t_atom *argv)
    : m_y1(0.3f),m_y2(0.f)
{
    FLEXT_ADDBANG(0,m_perform);
    FLEXT_ADDMETHOD_(0,"parameter",m_set);
    
    //parse arguments
    AtomList Args(argc,argv);
    m_paramf=sc_getfloatarg(Args,0);
    
    if (m_paramf == 0)
	m_paramf = 1;
    
    AddOutFloat();
}

void Crackle_kr::m_perform()
{
    float y0 = fabs(m_y1 * m_paramf - m_y2 - 0.05f);
    m_y2 = m_y1; 
    m_y1 = y0;
    
    ToOutFloat(0,y0);
}
