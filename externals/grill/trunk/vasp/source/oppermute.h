/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPPERMUTE_H
#define __VASP_OPPERMUTE_H

#include "opparam.h"
#include "oploop.h"

#define PERMTMPL
#define MAXPERMDIM 2

template<class T>
inline void permswap(T &a,T &b) { register T t = a; a = b; b = t; }

#ifdef PERMTMPL
template<class T,int origination(int pos, int sz,OpParam &p)>
void permutation1(OpParam &p)
#else
template<class T>
void permutation1(OpParam &p,int (*origination)(int pos, int sz,OpParam &p))
#endif
{
	T *ddt = p.rddt;
	const I ds = p.rds;
	const I sz = p.frames;

	if(ddt != p.rsdt) {
		// not in place
		const T *sdt = p.rsdt;
		const I ss = p.rss;
		I i;
		_DE_LOOP(i,sz, ( ddt[origination(i,sz,p)*ds] = sdt[i*ss] ) )
	}
	else {
		// in place 
		// \todo try to come from both sides!
		I i;
		_D_LOOP(i,sz-1) 
			int cur = i;
			do { cur = origination(cur,sz,p); } while(cur < i);
			if(cur > i) {
				// swap
				permswap(ddt[cur*ds],ddt[i*ds]);
			}
		_E_LOOP
	}
}

#ifdef PERMTMPL
template<class T,int origination(int pos, int sz,OpParam &p)>
void permutation2(OpParam &p)
#else
template<class T>
void permutation2(OpParam &p,int (*origination)(int pos, int sz,OpParam &p))
#endif
{
	T *rddt = p.rddt,*iddt = p.iddt;
	const I rds = p.rds,ids = p.ids;
	const I sz = p.frames;
	bool rinpl = rddt == p.rsdt,iinpl = iddt == p.isdt;

	if(rinpl == iinpl) {
		// re and im both in place
		I i;
		_D_LOOP(i,sz-1)
			int cur = i;
			do { cur = origination(cur,sz,p); } while(cur < i);
			if(cur > i) {
				// swap
				permswap(rddt[cur*rds],rddt[i*rds]);
				permswap(iddt[cur*ids],iddt[i*ids]);
			}
		_E_LOOP
	}
	else {
		if(!rinpl) {
			const T *sdt = p.rsdt;
			const I ss = p.rss;
			I i;
			if(ss == 1 && rds == 1)
				_DE_LOOP(i,sz, ( *(rddt++) = *(sdt++) ) )
			else
				_DE_LOOP(i,sz, ( *rddt = *sdt,rddt += rds,sdt += ss ) )
			rddt = p.rddt;
		}
		else permutation1<T>(p,origination);

		if(!iinpl) {
			const T *sdt = p.isdt;
			const I ss = p.iss;
			I i;
			if(ss == 1 && ids == 1)
				_DE_LOOP(i,sz, ( *(iddt++) = *(sdt++) ) )
			else
				_DE_LOOP(i,sz, ( *iddt = *sdt,iddt += ids,sdt += ss ) )
			iddt = p.iddt;
		}
		else {
			permswap(p.rddt,p.iddt); permswap(p.rds,p.ids);
			permutation1<T>(p,origination);
			permswap(p.rddt,p.iddt); permswap(p.rds,p.ids);
		}
	}
}

#ifdef PERMTMPL
#define PERMUTATION(tp,dim,p,func) permutation ## dim <tp,func>(p)
#else
#define PERMUTATION(tp,dim,p,func) permutation ## dim <tp>(p,func)
#endif

#endif

