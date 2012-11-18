/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_trnsc.h"
#include "opdefs.h"
#include "util.h"

// --------------------------------------------------------------

Vasp *VaspOp::m_rpow(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) 
{ 
	Vasp *ret = NULL;
	CVecBlock *vecs = GetCVecs(p.opname,src,dst);
	if(vecs) {
		if(arg.IsList() && arg.GetList().Count() >= 1 && flext::CanbeFloat(arg.GetList()[0]))
			p.cbin.rarg = flext::GetAFloat(arg.GetList()[0]);
		else {
			post("%s - argument is invalid -> set to 1",p.opname);
			p.cbin.rarg = 1;
		}
		p.cbin.iarg = 0; // not used anyway

		ret = DoOp(vecs,VecOp::d_rpow,p);
		delete vecs;
	}
	return ret;
}

VASP_BINARY("vasp.pow",pow,true,VASP_ARG_R(1),"Real power function") 
VASP_ANYOP("vasp.rpow",rpow,0,true,VASP_ARG_R(1),"Power function acting on complex radius") 


// --------------------------------------------------------------

VASP_UNARY("vasp.sqrt",sqrt,true,"Square root") 
VASP_UNARY("vasp.ssqrt",ssqrt,true,"Square root preserving the sign") 

// --------------------------------------------------------------

VASP_UNARY("vasp.exp",exp,true,"Exponential function") 
VASP_UNARY("vasp.log",log,true,"Natural logarithm") 


