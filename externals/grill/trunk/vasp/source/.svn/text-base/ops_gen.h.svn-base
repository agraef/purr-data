/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPS_GEN_H
#define __VASP_OPS_GEN_H

#include "opbase.h"

// Generator functions 

namespace VecOp {
	BL d_osc(OpParam &p);
	BL d_cosc(OpParam &p);
	BL d_mosc(OpParam &p);
	BL d_mcosc(OpParam &p);
	BL d_phasor(OpParam &p);
	BL d_mphasor(OpParam &p);
	BL d_noise(OpParam &p);
	BL d_cnoise(OpParam &p);
}

namespace VaspOp {
	Vasp *m_osc(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL mul = false);  // real osc
	inline Vasp *m_mosc(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) { return m_osc(p,src,arg,dst,true); }   // * real osc
	Vasp *m_cosc(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL mul = false);  // complex osc (phase rotates)
	inline Vasp *m_mcosc(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) { return m_cosc(p,src,arg,dst,true); }  // * complex osc (phase rotates)
	Vasp *m_phasor(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL mul = false);  // phasor
	inline Vasp *m_mphasor(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) { return m_phasor(p,src,arg,dst,true); }  // * phasor
	Vasp *m_noise(OpParam &p,CVasp &src,CVasp *dst);  // real noise
	Vasp *m_cnoise(OpParam &p,CVasp &src,CVasp *dst); // complex noise (arg and abs random)
}

#endif
