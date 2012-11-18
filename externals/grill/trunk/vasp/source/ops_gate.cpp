/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_cmp.h"
#include "opdefs.h"
#include "util.h"

// --------------------------------------------------------------


Vasp *VaspOp::m_gate(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) 
{ 
	Vasp *ret = NULL;
	RVecBlock *vecs = GetRVecs(p.opname,src,dst);
	if(vecs) {
		if(arg.IsList() && arg.GetList().Count() >= 1 && flext::CanbeFloat(arg.GetList()[0]))
			p.rbin.arg = flext::GetAFloat(arg.GetList()[0]);
		else {
			post("%s - argument is invalid -> set to 1",p.opname);
			p.rbin.arg = 1;
		}

		ret = DoOp(vecs,VecOp::d_gate,p);
		delete vecs;
	}
	return ret;
}


Vasp *VaspOp::m_rgate(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) 
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

		ret = DoOp(vecs,VecOp::d_rgate,p);
		delete vecs;
	}
	return ret;
}

VASP_ANYOP("vasp.gate",gate,1,true,VASP_ARG_R(1),"set destination to 0 if source < argument") 
VASP_ANYOP("vasp.rgate",rgate,1,true,VASP_ARG_R(1),"complex radius gate: set destination to 0 if rad(complex source) < rad(complex argument)") 


// --------------------------------------------------------------

