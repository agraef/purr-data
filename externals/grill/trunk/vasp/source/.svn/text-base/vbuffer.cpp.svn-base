/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "vbuffer.h"
#include "buflib.h"

V VSymbol::Inc() { if(sym) BufLib::IncRef(sym); }
V VSymbol::Dec() { if(sym) BufLib::DecRef(sym); }

///////////////////////////////////////////////////////////////////////////
// SysBuf class
///////////////////////////////////////////////////////////////////////////

SysBuf &SysBuf::Set(const VSymbol &s,I c,I l,I o)
{
	buf.Set(s.Symbol());

	chn = c;
	if(chn > Channels()) {
		I chn1 = Channels()-1;
		post("vasp - buffer %s: Channel index (%i) is out of range, set to highest (%i)",s.Name(),chn,chn1);
		chn = chn1; // simply correct the channel??
	}
	offs = o;
	if(offs < 0) {
		post("vasp - buffer %s: Offset (%i) is out of range, set to 0",s.Name(),offs);
		offs = 0;
	}
	if(offs > Frames()) {
//		post("vasp - buffer %s: Offset (%i) is out of range, set to %i",s.Name(),offs,Frames());
		offs = Frames();
	}
	len = l >= 0?l:Frames();
	if(offs+len > Frames()) {
		I len1 = Frames()-offs;
		if(l >= 0) post("vasp - buffer %s: Length (%i) is out of range, corrected to %i",s.Name(),len,len1);
		len = len1;
	}

	return *this;
}


