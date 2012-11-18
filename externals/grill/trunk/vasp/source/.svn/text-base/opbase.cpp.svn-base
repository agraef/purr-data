/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "opbase.h"
#include "opdefs.h"

Vasp *VaspOp::m_run(OpParam &p,CVasp &src,CVasp *dst,opfun fun) 
{ 
	Vasp *ret = NULL;
	RVecBlock *vecs = GetRVecs(p.opname,src,dst);
	if(vecs) {
		ret = DoOp(vecs,fun,p);
		delete vecs;
	}

	return ret;
}

Vasp *VaspOp::m_cun(OpParam &p,CVasp &src,CVasp *dst,opfun fun) 
{ 
	Vasp *ret = NULL;
	CVecBlock *vecs = GetCVecs(p.opname,src,dst);
	if(vecs) {
		ret = DoOp(vecs,fun,p);
		delete vecs;
	}

	return ret;
}

Vasp *VaspOp::m_rbin(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,opfun fun) 
{ 
	Vasp *ret = NULL;
	BL argvasp = arg.IsVasp();

	RVecBlock *vecs = argvasp?GetRVecs(p.opname,src,arg.GetVasp(),dst):GetRVecs(p.opname,src,dst);
	if(vecs) {
		if(arg.CanbeDouble()) p.rbin.arg = arg.GetADouble(); 
		else if(arg.IsEnv()) {
			if(p.args != 1)
				ERRINTERNAL();
			else 
				p.arg[0].SetE(&arg.GetEnv());
		}
	
		ret = DoOp(vecs,fun,p);
		delete vecs;
	}

	return ret;
}

Vasp *VaspOp::m_cbin(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,opfun fun) 
{ 
	Vasp *ret = NULL;
	BL argvasp = arg.IsVasp();

	CVecBlock *vecs = argvasp?GetCVecs(p.opname,src,arg.GetVasp(),dst):GetCVecs(p.opname,src,dst);
	if(vecs) {
		if(arg.CanbeComplex()) {
			CX z = arg.GetAComplex();
			p.cbin.rarg = z.real();
			p.cbin.iarg = z.imag();
		}
		else if(arg.IsEnv()) {
			if(p.args != 1)
				ERRINTERNAL();
			else 
				p.arg[0].SetE(&arg.GetEnv());
		}

		ret = DoOp(vecs,fun,p);
		delete vecs;
	}

	return ret;
}
