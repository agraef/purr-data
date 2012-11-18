/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPS_WND_H
#define __VASP_OPS_WND_H

#include "opbase.h"

// Window functions

namespace VecOp {
/*
	BL d_bevel(OpParam &p); 
	BL d_mbevel(OpParam &p); 
*/
	BL d_window(OpParam &p);
}

namespace VaspOp {
/*
	Vasp *m_bevelup(OpParam &p,CVasp &src,CVasp *dst,BL up = true,BL mul = false);  // bevel up 
	inline Vasp *m_mbevelup(OpParam &p,CVasp &src,CVasp *dst) { return m_bevelup(p,src,dst,true,true); }   // * bevel up (fade in)
	inline Vasp *m_beveldn(OpParam &p,CVasp &src,CVasp *dst) { return m_bevelup(p,src,dst,false,false); }  // bevel down
	inline Vasp *m_mbeveldn(OpParam &p,CVasp &src,CVasp *dst) { return m_bevelup(p,src,dst,false,true); }   // * bevel down (fade out)
*/
	Vasp *m_window(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL inv = false,BL mul = false,BL symm = false);  // window curve
	inline Vasp *m_mwindow(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) { return m_window(p,src,arg,dst,false,true,false); }  // * window curve
	inline Vasp *m_iwindow(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) { return m_window(p,src,arg,dst,true,false,false); }  // inverse window curve
	inline Vasp *m_miwindow(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) { return m_window(p,src,arg,dst,true,true,false); }  // * inverse window curve
	inline Vasp *m_xwindow(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) { return m_window(p,src,arg,dst,false,false,true); }  // symmetrical window curve
	inline Vasp *m_mxwindow(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) { return m_window(p,src,arg,dst,false,true,true); }  // * symmetrical window curve
}

#endif
