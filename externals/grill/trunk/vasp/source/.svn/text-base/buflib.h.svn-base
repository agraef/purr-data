/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

/*! \file buflib.h
	\brief Routines for buffer management
*/

#ifndef __VASP_BUFLIB_H
#define __VASP_BUFLIB_H

#include "classes.h"
#include "vbuffer.h"

class BufEntry;

namespace BufLib
{
	VBuffer *Get(const VSymbol &s,I chn = 0,I len = -1,I offs = 0);

	BufEntry *NewImm(I fr,BL zero = true);

	V IncRef(const t_symbol *s);
	V DecRef(const t_symbol *s);

	BufEntry *Resize(BufEntry *e,I fr,BL keep = false,BL zero = true); 
}



#endif
