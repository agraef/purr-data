/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPS_ASSIGN_H
#define __VASP_OPS_ASSIGN_H

#include "opfuns.h"

// Assignment functions

DEFOP(BS,d_copy,copy,run)
DEFOP(BS,d_ccopy,copy,cun)

DEFOP(BS,d_set,set,rbin)
DEFOP(BS,d_cset,set,cbin)


namespace VaspOp {
	inline Vasp *m_set(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst = NULL) { return m_rbin(p,src,arg,dst,VecOp::d_set); } // copy to (one vec or real)
	inline Vasp *m_cset(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst = NULL) { return m_cbin(p,src,arg,dst,VecOp::d_cset); }  // complex copy (pairs of vecs or complex)

	Vasp *m_copy(OpParam &p,CVasp &src,CVasp &dst);
	Vasp *m_ccopy(OpParam &p,CVasp &src,CVasp &dst);
}

#endif
