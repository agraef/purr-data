/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flatom_part.cpp
    \brief Definitions for handling the t_atom type and lists thereof.
*/
 
#ifndef __FLEXT_ATOM_PART_CPP
#define __FLEXT_ATOM_PART_CPP

#include "flext.h"

#include "flpushns.h"

FLEXT_TEMPIMPL(int FLEXT_CLASSDEF(flext))::AtomList::Get(t_atom *argv,int mxsz) const
{
    int argc = Count();
    if(mxsz >= 0 && argc > mxsz) argc = mxsz;

    for(int i = 0; i < argc; ++i) SetAtom(argv[i],lst[i]);

    return argc;
}


FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::AtomList::GetPart(int offs,int len,AtomList &ret) const
{
    if(offs+len > Count()) {
        len = Count()-offs;
        if(len < 0) len = 0;
    }

    ret(len,Atoms()+offs);
}

#include "flpopns.h"

#endif // __FLEXT_ATOM_PART_CPP

