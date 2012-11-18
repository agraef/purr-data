// 
//  
//  direct port of sndobj's StringFlt
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


class FlextStringFlt:
	public flext_sndobj
{
	FLEXT_HEADER(FlextStringFlt,flext_sndobj);

public:
	FlextStringFlt(int argc, t_atom* argv);

	virtual bool NewObjs();
	virtual void FreeObjs();
	virtual void ProcessObjs();

	StringFlt* string;

	float fdbgain;

	float freq;
	
private:
	
	FLEXT_ATTRVAR_F(fdbgain);
	FLEXT_ATTRVAR_F(freq);
};

FLEXT_LIB_DSP_V("StringFlt~",FlextStringFlt)


FlextStringFlt::FlextStringFlt(int argc, t_atom* argv)
{
	AtomList l(argc, argv);
	
	FLEXT_ADDATTR_VAR1("fdbgain", fdbgain);
	FLEXT_ADDATTR_VAR1("frequency", freq);

	AddInSignal(1);
	AddOutSignal(1);
}


bool FlextStringFlt::NewObjs()
{
	string = new StringFlt(freq, fdbgain, &InObj(0), 
		0, Blocksize(), Samplerate());
	
	return true;
}

void FlextStringFlt::FreeObjs()
{
	delete string;
}

void FlextStringFlt::ProcessObjs()
{
	string->SetFreq(freq);
	string->SetFdbgain(fdbgain);

	string->DoProcess();

	*string >> OutObj(0);
}

static void setup()
{
	FLEXT_DSP_SETUP(FlextStringFlt);
}

FLEXT_LIB_SETUP(StringFlt_tilde,setup)
