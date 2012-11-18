/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_cplx.h"
#include "opdefs.h"
#include "util.h"

// -----------------------------------------------------

VASP_UNARY("vasp.polar",polar,true,"convert complex vector pair from rectangular to polar coordinates") 
VASP_UNARY("vasp.rect",rect,true,"convert complex vector pair from polar to rectangular coordinates") 


// -----------------------------------------------------


Vasp *VaspOp::m_radd(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) 
{ 
	Vasp *ret = NULL;
	CVecBlock *vecs = GetCVecs(p.opname,src,dst);
	if(vecs) {
		if(arg.IsList() && arg.GetList().Count() >= 1 && flext::CanbeFloat(arg.GetList()[0]))
			p.cbin.rarg = flext::GetAFloat(arg.GetList()[0]);
		else {
			post("%s - argument is invalid -> set to 0",p.opname);
			p.cbin.rarg = 0;
		}
		p.cbin.iarg = 0; // not used anyway

		ret = DoOp(vecs,VecOp::d_radd,p);
		delete vecs;
	}
	return ret;
}


VASP_ANYOP("vasp.r+",radd,0,true,VASP_ARG_R(0),"add offset to complex radius (of complex vector pair)") 


// -----------------------------------------------------

VASP_UNARY("vasp.cnorm",cnorm,true,"normalize complex radius to 1 (but preserve angle)")

// -----------------------------------------------------

//VASP_UNARY("vasp.cconj",cconj,true,"complex conjugate: multiply imaginary part with -1")  // should be replaced by an abstraction

