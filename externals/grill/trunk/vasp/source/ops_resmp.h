/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPS_RESMP_H
#define __VASP_OPS_RESMP_H

#include "opbase.h"

// Resampling (around center sample)

namespace VecOp {
	BL d_tilt(OpParam &p);
}

namespace VaspOp {
	Vasp *m_tilt(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst = NULL,BL symm = false); 
	// Symmetric resampling (around center sample)
	inline Vasp *m_xtilt(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst = NULL) { return m_tilt(p,src,arg,dst,true); }
}

#endif
