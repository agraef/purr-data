/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_ENV_H
#define __VASP_ENV_H

#include "vasp.h"

class Env:
    public flext
{
public:
	Env(): cnt(0),pos(NULL),val(NULL) {}
	Env(I argc,const t_atom *argv);
//	Env(const Env &p);
	~Env();

	static BL ChkArgs(I argc,const t_atom *argv);

	V MakeList(flext::AtomList &ret) const;

	V Clear();

	BL Ok() const { return cnt && pos != NULL && val != NULL; }

//	friend class Iter;

	class Iter 
	{
	public:
		Iter(const Env &e);
		V Init(R p);

		R ValFwd(R p) 
		{
			if(p > npt) UpdateFwd(p);
			return pvl+k*(p-ppt);
		}

		R ValBwd(R p) 
		{
			if(p < ppt) UpdateBwd(p);
			return pvl+k*(p-ppt);
		}

	protected:
		V UpdateFwd(R p);
		V UpdateBwd(R p);

		const Env &bp;
		I ix;
		R ppt,npt;
		R pvl,nvl;
		R k;
	};

	I Count() const { return cnt; }
	const R *Pos() const { return pos; }
	const R *Val() const { return val; }
	R Pos(I ix) const { return pos[ix]; }
	R Val(I ix) const { return val[ix]; }

protected:
	I cnt;
	R *pos,*val;
};

#endif

