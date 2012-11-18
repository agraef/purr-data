/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPS_CPLX_H
#define __VASP_OPS_CPLX_H

#include "opfuns.h"

// Complex functions

DEFOP(BS,d_polar,polar,cun)
DEFOP(BS,d_rect,rect,cun)

DEFOP(BS,d_radd,radd,cbin)

DEFOP(BS,d_cnorm,norm,cun)

//DEFOP(S,d_cconj,conj,cun)


namespace VaspOp {
	inline Vasp *m_polar(OpParam &p,CVasp &src,CVasp *dst = NULL) { return m_cun(p,src,dst,VecOp::d_polar); } // cartesian -> polar (each two)
	inline Vasp *m_rect(OpParam &p,CVasp &src,CVasp *dst = NULL) { return m_cun(p,src,dst,VecOp::d_rect); } // polar -> cartesian (each two)

	Vasp *m_radd(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst = NULL); // radius offset

	inline Vasp *m_cnorm(OpParam &p,CVasp &src,CVasp *dst = NULL) { return m_cun(p,src,dst,VecOp::d_cnorm); } // complex normalize

//	inline Vasp *m_cswap(OpParam &p,CVasp &src,CVasp *dst = NULL) { return m_cun(p,src,dst,VecOp::d_cswap); }  // swap real and imaginary parts
//	inline Vasp *m_cconj(OpParam &p,CVasp &src,CVasp *dst = NULL) { return m_cun(p,src,dst,VecOp::d_cconj); }  // complex conjugate
}

#endif
