/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPS_DFT_H
#define __VASP_OPS_DFT_H

#include "opbase.h"

// Fourier transforms

namespace VaspOp {
	Vasp *m_rfft(OpParam &p,CVasp &src,CVasp *dst = NULL,BL inv = false);  // real forward
	inline Vasp *m_rifft(OpParam &p,CVasp &src,CVasp *dst = NULL) { return m_rfft(p,src,dst,true); } // real inverse
	Vasp *m_cfft(OpParam &p,CVasp &src,CVasp *dst = NULL,BL inv = false); // complex forward
	inline Vasp *m_cifft(OpParam &p,CVasp &src,CVasp *dst = NULL) { return m_cfft(p,src,dst,true); } // complex inverse
}

#endif
