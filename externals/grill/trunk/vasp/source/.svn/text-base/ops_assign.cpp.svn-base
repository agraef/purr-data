/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_assign.h"
#include "opdefs.h"


VASP_BINARY("vasp.set vasp.=",set,false,VASP_ARG_R(0),"Assigns a value, envelope or vasp")
VASP_BINARY("vasp.cset vasp.c=",cset,false,VASP_ARG_R(0),"Assigns a complex value, real envelope or vasp")



Vasp *VaspOp::m_copy(OpParam &p,CVasp &src,CVasp &arg) 
{ 
	Vasp *s = NULL,*d = NULL;
	RVecBlock *vecs = GetRVecs(p.opname,src,&arg);
	if(vecs) {
		d = DoOp(vecs,VecOp::d_copy,p);
		s = vecs->SrcVasp();
		if(d) arg = *d; else { arg.Clear(); delete d; }
		delete vecs;
	}
	return s;
}

Vasp *VaspOp::m_ccopy(OpParam &p,CVasp &src,CVasp &arg) 
{ 
	Vasp *s = NULL,*d = NULL;
	CVecBlock *vecs = GetCVecs(p.opname,src,&arg);
	if(vecs) {
		d = DoOp(vecs,VecOp::d_ccopy,p);
		s = vecs->SrcVasp();
		if(d) arg = *d; else { arg.Clear(); delete d; }
		delete vecs;
	}
	return s;
}


class vasp_copy:
	public vasp_anyop
{																				
	FLEXT_HEADER(vasp_copy,vasp_anyop)
public:			
	
	vasp_copy(I argc,const t_atom *argv): vasp_anyop(argc,argv,VASP_ARG(),true,XletCode(xlet_any,0)) {}

	virtual V m_to(I,const t_atom *) { post("s - destination vasp is ignored!",thisName()); }

	virtual Vasp *do_copy(OpParam &p,CVasp &dst) { CVasp cref(ref); return VaspOp::m_copy(p,cref,dst); }
		
	virtual Vasp *tx_work(const Argument &arg) 
	{ 
		OpParam p(thisName(),0);													
		
		if(arg.CanbeVasp()) {
			CVasp dst(arg.GetAVasp());
			Vasp *ret = do_copy(p,dst);
			ToOutVasp(1,dst);
			return ret;
		}
		else {
			post("%s - argument is not a valid vasp!",thisName());  // \todo check earlier!
			return NULL;
		}
	}

	virtual V m_help() { post("%s - Copies the triggering vasp to the argument vasp",thisName()); }
};																				
VASP_LIB_V("vasp.copy vasp.->",vasp_copy)


class vasp_ccopy:
	public vasp_copy
{																				
	FLEXT_HEADER(vasp_ccopy,vasp_copy)
public:			
	
	vasp_ccopy(I argc,const t_atom *argv): vasp_copy(argc,argv) {}

	virtual Vasp *do_copy(OpParam &p,CVasp &dst) { CVasp cref(ref); return VaspOp::m_ccopy(p,cref,dst); }

	virtual V m_help() { post("%s - Copies complex pairs of the triggering vasp to the argument vasp",thisName()); }
};																				
VASP_LIB_V("vasp.ccopy vasp.c->",vasp_ccopy)


