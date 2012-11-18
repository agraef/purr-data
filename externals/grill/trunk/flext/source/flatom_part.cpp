/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3657 $
$LastChangedDate: 2009-02-09 17:58:30 -0500 (Mon, 09 Feb 2009) $
$LastChangedBy: thomas $
*/

/*! \file flatom_part.cpp
    \brief Definitions for handling the t_atom type and lists thereof.
*/
 
#include "flext.h"

#include "flpushns.h"

int flext::AtomList::Get(t_atom *argv,int mxsz) const
{
    int argc = Count();
    if(mxsz >= 0 && argc > mxsz) argc = mxsz;

    for(int i = 0; i < argc; ++i) SetAtom(argv[i],lst[i]);

    return argc;
}


void flext::AtomList::GetPart(int offs,int len,AtomList &ret) const
{
    if(offs+len > Count()) {
        len = Count()-offs;
        if(len < 0) len = 0;
    }

    ret(len,Atoms()+offs);
}

#include "flpopns.h"
