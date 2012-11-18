/* sc4pd 
   CoinGate

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
     
   Coded while listening to: the sounds coming through my open window
   
*/

#include "sc4pd.hpp"

/* ------------------------ CoinGate ---------------------------------*/

class CoinGate_kr:
    public flext_base
{
    FLEXT_HEADER(CoinGate_kr,flext_base);

public:
    CoinGate_kr(int argc, t_atom *argv);
    
protected:
    void m_bang();

    void m_seed(int i)
    {
	rgen.init(i);
    }

private:
    float prob;
    FLEXT_CALLBACK(m_bang);
    FLEXT_CALLBACK_I(m_seed);
    RGen rgen;
};

FLEXT_LIB_V("CoinGate",CoinGate_kr);

CoinGate_kr::CoinGate_kr(int argc, t_atom *argv)
{
    AtomList Args(argc,argv);
    
    rgen.init(timeseed());

    prob = sc_getfloatarg(Args,0);
        
    FLEXT_ADDBANG(0,m_bang);
    FLEXT_ADDMETHOD_(0,"seed",m_seed);
    AddOutBang();
}

void CoinGate_kr::m_bang()
{
    if(rgen.frand() < prob)    
	ToOutBang(0);
}
