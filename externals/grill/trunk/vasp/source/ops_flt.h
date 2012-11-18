/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPS_FLT_H
#define __VASP_OPS_FLT_H

#include "opfuns.h"

// Filtering functions

namespace VecOp {
	BL d_flp(OpParam &p);
	BL d_fhp(OpParam &p);

	BL d_int(OpParam &p);
	BL d_dif(OpParam &p); 
}

DEFOP(BS,d_fix,fix,run)

namespace VaspOp {
	// passive filters
	Vasp *m_fhp(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst = NULL,BL hp = true); //! hi pass
	inline Vasp *m_flp(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst = NULL) { return m_fhp(p,src,arg,dst,false); } //! lo pass	

	// int/dif functions
	Vasp *m_int(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst = NULL,BL inv = false); //! integrate
	inline Vasp *m_dif(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst = NULL) { return m_int(p,src,arg,dst,true); } //! differentiate

	// fix denormals/NANs
	inline Vasp *m_fix(OpParam &p,CVasp &src,CVasp *dst = NULL) { return m_run(p,src,dst,VecOp::d_fix); }    // ! NAN/denormal filter
}

#endif
