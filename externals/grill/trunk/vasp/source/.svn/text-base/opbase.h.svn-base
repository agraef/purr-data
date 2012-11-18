/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPBASE_H
#define __VASP_OPBASE_H

#include "main.h"
#include "classes.h"
#include "vecblk.h"
#include "opparam.h"


namespace VaspOp {
	typedef BL opfun(OpParam &p);

	// -------- prepare vectors and do vector operation -----------
	// in opvecs.cpp

	RVecBlock *GetRVecs(const C *op,CVasp &src,CVasp *dst = NULL);
	CVecBlock *GetCVecs(const C *op,CVasp &src,CVasp *dst = NULL,BL full = false);
	RVecBlock *GetRVecs(const C *op,CVasp &src,const CVasp &arg,CVasp *dst = NULL,I multi = -1,BL ssize = true);
	CVecBlock *GetCVecs(const C *op,CVasp &src,const CVasp &arg,CVasp *dst = NULL,I multi = -1,BL ssize = true,BL full = false);
	
	Vasp *DoOp(RVecBlock *vecs,opfun *fun,OpParam &p,BL symm = false);
	Vasp *DoOp(CVecBlock *vecs,opfun *fun,OpParam &p,BL symm = false);

	// -------- transformations -----------------------------------
	// in opbase.cpp

	// unary functions
	Vasp *m_run(OpParam &p,CVasp &src,CVasp *dst,opfun fun); // real unary (one vec or real)
	Vasp *m_cun(OpParam &p,CVasp &src,CVasp *dst,opfun fun); // complex unary (one vec or complex)

	// binary functions
	Vasp *m_rbin(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,opfun fun); // real binary (one vec or real)
	Vasp *m_cbin(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,opfun fun); // complex binary (one vec or complex)

}

#endif
