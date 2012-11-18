// 
//  
//  direct port of sndobj's Buzz
//  Copyright (C) 2005  Tim Blechmann
//  
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.



#define FLEXT_ATTRIBUTES 1

#include <flsndobj.h>
 
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5
#endif


class FlextBuzz:
	public flext_sndobj
{
	FLEXT_HEADER(FlextBuzz,flext_sndobj);

public:
	FlextBuzz(int argc, t_atom* argv);

	virtual bool NewObjs();
	virtual void FreeObjs();
	virtual void ProcessObjs();

	Buzz* buzz;

	float freq;
	float amp;
	int harm;
	
private:
	
	FLEXT_ATTRVAR_F(amp);
	FLEXT_ATTRVAR_F(freq);
	FLEXT_ATTRVAR_I(harm);
};

FLEXT_LIB_DSP_V("Buzz~",FlextBuzz)


FlextBuzz::FlextBuzz(int argc, t_atom* argv)
{
	AtomList l(argc, argv);
	
	FLEXT_ADDATTR_VAR1("frequency", freq);
	FLEXT_ADDATTR_VAR1("amp", amp);
	FLEXT_ADDATTR_VAR1("harm", harm);

	AddOutSignal(1);
}


bool FlextBuzz::NewObjs()
{
	buzz = new Buzz(freq, amp, harm, 0, 
		0, Blocksize(), Samplerate());
	
	return true;
}

void FlextBuzz::FreeObjs()
{
	delete buzz;
}

void FlextBuzz::ProcessObjs()
{
	buzz->SetFreq(freq);
	buzz->SetAmp(amp);
	buzz->SetHarm(harm);
	
	buzz->DoProcess();
	
	*buzz >> OutObj(0);
}

static void setup()
{
	FLEXT_DSP_SETUP(FlextBuzz);
}

FLEXT_LIB_SETUP(Buzz_tilde,setup)
