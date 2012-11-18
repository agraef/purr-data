/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPS_FEATURE_H
#define __VASP_OPS_FEATURE_H

#include "opbase.h"

// Feature extraction functions

namespace VecOp {
}

namespace VaspOp {
	// extrema functions
	Vasp *m_peaks(OpParam &p,CVasp &src,CVasp *dst = NULL,BL inv = false); //! find peaks
	inline Vasp *m_valleys(OpParam &p,CVasp &src,CVasp *dst = NULL) { return m_peaks(p,src,dst,true); } //! find valleys	
	Vasp *m_rpeaks(OpParam &p,CVasp &src,CVasp *dst = NULL,BL inv = false); //! find radius peaks
	inline Vasp *m_rvalleys(OpParam &p,CVasp &src,CVasp *dst = NULL) { return m_rpeaks(p,src,dst,true); } //! find radius valleys	
}

#endif
