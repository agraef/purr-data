/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flatom.cpp
    \brief Definitions for handling the t_atom type and lists thereof.
*/
 
#ifndef __FLEXT_ATOM_CPP
#define __FLEXT_ATOM_CPP

#include "flext.h"

#include <cstring> // for memcpy

#include "flpushns.h"

#if FLEXT_SYS != FLEXT_SYS_JMAX
FLEXT_TEMPIMPL(int FLEXT_CLASSDEF(flext))::CmpAtom(const t_atom &a,const t_atom &b)
{
	if(GetType(a) == GetType(b)) {
		switch(GetType(a)) {
			case A_FLOAT: return GetFloat(a) == GetFloat(b)?0:(GetFloat(a) < GetFloat(b)?-1:1);
#if FLEXT_SYS == FLEXT_SYS_MAX
			case A_INT: return GetInt(a) == GetInt(b)?0:(GetInt(a) < GetInt(b)?-1:1);
#endif
			case A_SYMBOL: return GetSymbol(a) == GetSymbol(b)?0:strcmp(GetString(a),GetString(b));
#if FLEXT_SYS == FLEXT_SYS_PD
			case A_POINTER: return GetPointer(a) == GetPointer(b)?0:(GetPointer(a) < GetPointer(b)?-1:1);
#endif
			default:
				// can't be compared.....
				FLEXT_ASSERT(false);
				return 0;
		}
	}
	else
		return GetType(a) < GetType(b)?-1:1;
}
#else
#error Not implemented
#endif

FLEXT_TEMPIMPL(t_atom *FLEXT_CLASSDEF(flext))::CopyList(int argc,const t_atom *argv)
{
	t_atom *dst = new t_atom[argc];
    memcpy(dst,argv,argc*sizeof(t_atom));
	return dst;
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::CopyAtoms(int cnt,t_atom *dst,const t_atom *src)
{
    if(dst < src)
        // forward
        memcpy(dst,src,cnt*sizeof(t_atom));
    else
        // backwards
        while(cnt--) dst[cnt] = src[cnt];
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::AtomList::Alloc(int sz,int keepix,int keeplen,int keepto)
{
    if(lst) {
        if(cnt == sz) {
            if(keepix >= 0 && keepix != keepto) {
                int c = keeplen >= 0?keeplen:cnt;
                FLEXT_ASSERT(c+keepto <= cnt);
                FLEXT_ASSERT(c+keepix <= cnt);
                CopyAtoms(c,lst+keepto,lst+keepix);
            }

            return; // no change
        }

        t_atom *l;
        if(sz) {
            l = new t_atom[sz];
            if(keepix >= 0) {
                // keep contents
                int c = keeplen >= 0?keeplen:(cnt > sz?sz:cnt);
                FLEXT_ASSERT(c+keepto <= sz);
                FLEXT_ASSERT(c+keepix <= cnt);
                CopyAtoms(c,l+keepto,lst+keepix);
            }
        }
        else
            l = NULL;

        Free();
        lst = l,cnt = sz;
    }
    else {
        FLEXT_ASSERT(cnt == 0);
        if(sz) lst = new t_atom[cnt = sz];
    }
}

FLEXT_TEMPIMPL(FLEXT_CLASSDEF(flext))::AtomList::~AtomList() { Free(); }

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::AtomList::Free()
{
    if(lst) { 
        delete[] lst; lst = NULL; 
        cnt = 0;
    }
    else
        FLEXT_ASSERT(cnt == 0);
}

FLEXT_TEMPIMPL(FLEXT_TEMPSUB(FLEXT_CLASSDEF(flext))::AtomList &FLEXT_CLASSDEF(flext))::AtomList::Set(int argc,const t_atom *argv,int offs,bool resize)
{
	int ncnt = argc+offs;
	if(resize) Alloc(ncnt);

    // argv can be NULL independently from argc
    if(argv) CopyAtoms(argc,lst+offs,argv);

	return *this;
}

FLEXT_TEMPIMPL(int FLEXT_CLASSDEF(flext))::AtomList::Compare(const AtomList &a) const
{
	if(Count() == a.Count()) {
		for(int i = 0; i < Count(); ++i) {
			int cmp = CmpAtom(lst[i],a[i]);
			if(cmp) return cmp;
		}
		return 0;
	}
	else 
		return Count() < a.Count()?-1:1;
}

FLEXT_TEMPIMPL(FLEXT_CLASSDEF(flext))::AtomListStaticBase::~AtomListStaticBase() { Free(); }

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::AtomListStaticBase::Alloc(int sz,int keepix,int keeplen,int keepto)
{ 
    if(sz <= precnt) {
        // small enough for pre-allocated space

        if(AtomList::lst != predata && AtomList::lst) {
            // currently allocated memory is larger than what we need

            if(keepix >= 0) {
                // keep contents
                int c = keeplen >= 0?keeplen:(AtomList::cnt > sz?sz:AtomList::cnt);
                FLEXT_ASSERT(c+keepto <= precnt);
                FLEXT_ASSERT(c+keepix <= AtomList::cnt);
                CopyAtoms(c,predata+keepto,AtomList::lst+keepix);
            }

            // free allocated memory
            AtomList::Free();
        }
        AtomList::lst = predata;
        AtomList::cnt = sz;
    }
    else 
        AtomList::Alloc(sz,keepix,keeplen,keepto);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::AtomListStaticBase::Free()
{
    if(AtomList::lst != predata)
        AtomList::Free();
    else {
        AtomList::lst = NULL;
        AtomList::cnt = 0;
    }
}

#include "flpopns.h"

#endif // __FLEXT_ATOM_CPP

