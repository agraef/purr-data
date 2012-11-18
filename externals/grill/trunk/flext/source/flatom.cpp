/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3669 $
$LastChangedDate: 2009-03-05 18:34:39 -0500 (Thu, 05 Mar 2009) $
$LastChangedBy: thomas $
*/

/*! \file flatom.cpp
    \brief Definitions for handling the t_atom type and lists thereof.
*/
 
#include "flext.h"
#include <cstring> // for memcpy

#include "flpushns.h"

#if FLEXT_SYS != FLEXT_SYS_JMAX
int flext::CmpAtom(const t_atom &a,const t_atom &b)
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

t_atom *flext::CopyList(int argc,const t_atom *argv)
{
	t_atom *dst = new t_atom[argc];
    memcpy(dst,argv,argc*sizeof(t_atom));
	return dst;
}

void flext::CopyAtoms(int cnt,t_atom *dst,const t_atom *src)
{
    if(dst < src)
        // forward
        memcpy(dst,src,cnt*sizeof(t_atom));
    else
        // backwards
        while(cnt--) dst[cnt] = src[cnt];
}

void flext::AtomList::Alloc(int sz,int keepix,int keeplen,int keepto)
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

flext::AtomList::~AtomList() { Free(); }

void flext::AtomList::Free()
{
    if(lst) { 
        delete[] lst; lst = NULL; 
        cnt = 0;
    }
    else
        FLEXT_ASSERT(cnt == 0);
}

flext::AtomList &flext::AtomList::Set(int argc,const t_atom *argv,int offs,bool resize)
{
	int ncnt = argc+offs;
	if(resize) Alloc(ncnt);

    // argv can be NULL independently from argc
    if(argv) CopyAtoms(argc,lst+offs,argv);

	return *this;
}

int flext::AtomList::Compare(const AtomList &a) const
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

flext::AtomListStaticBase::~AtomListStaticBase() { Free(); }

void flext::AtomListStaticBase::Alloc(int sz,int keepix,int keeplen,int keepto)
{ 
    if(sz <= precnt) {
        // small enough for pre-allocated space

        if(lst != predata && lst) {
            // currently allocated memory is larger than what we need

            if(keepix >= 0) {
                // keep contents
                int c = keeplen >= 0?keeplen:(cnt > sz?sz:cnt);
                FLEXT_ASSERT(c+keepto <= precnt);
                FLEXT_ASSERT(c+keepix <= cnt);
                CopyAtoms(c,predata+keepto,lst+keepix);
            }

            // free allocated memory
            AtomList::Free();
        }
        lst = predata,cnt = sz;
    }
    else 
        AtomList::Alloc(sz,keepix,keeplen,keepto);
}

void flext::AtomListStaticBase::Free() 
{
    if(lst != predata) AtomList::Free();
    else lst = NULL,cnt = 0;
}

#include "flpopns.h"
