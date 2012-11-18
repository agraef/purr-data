/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_carith.h"
#include "ops_assign.h"
#include "opdefs.h"
#include "util.h"


VASP_BINARY("vasp.c+",cadd,true,VASP_ARG_R(0),"adds a complex value or vasp")
VASP_BINARY("vasp.c-",csub,true,VASP_ARG_R(0),"subtracts a complex value or vasp")
VASP_BINARY("vasp.c!-",csubr,true,VASP_ARG_R(0),"reverse subtracts a complex value or vasp")
VASP_BINARY("vasp.c*",cmul,true,VASP_ARG_R(1),"multiplies by a complex value or vasp")
VASP_BINARY("vasp.c/",cdiv,true,VASP_ARG_R(1),"divides by a complex value or vasp")
VASP_BINARY("vasp.c!/",cdivr,true,VASP_ARG_R(1),"reverse divides by a complex value or vasp")


// -----------------------------------------------------


VASP_UNARY("vasp.csqr",csqr,true,"complex square") 

// -----------------------------------------------------

Vasp *VaspOp::m_cpowi(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst) 
{ 
	Vasp *ret = NULL;
	CVecBlock *vecs = GetCVecs(p.opname,src,dst);
	if(vecs) {
		I powi = 1;
		if(arg.IsList() && arg.GetList().Count() >= 1 && flext::CanbeInt(arg.GetList()[0]))
			powi = flext::GetAInt(arg.GetList()[0]);
		else 
			post("%s - power arg is invalid -> set to 1",p.opname);

		if(powi < 0) {
			post("%s - negative integer power is not allowed",p.opname);
		}
		else {
			switch(powi) {
			case 0: {
				p.cbin.rarg = 1,p.cbin.iarg = 0;
				ret = DoOp(vecs,VecOp::d_cset,p);
				break;
			}
			case 1: {
				// set arg to src
				ret = DoOp(vecs,VecOp::d_ccopy,p);
				break;
			}
			case 2: {
				ret = DoOp(vecs,VecOp::d_csqr,p);
				break;
			}
			default: {
				p.ibin.arg = powi;
				ret = DoOp(vecs,VecOp::d_cpowi,p);
				break;
			}
			}
		}

		delete vecs;
	}
	return ret;
}

VASP_ANYOP("vasp.cpowi",cpowi,0,true,VASP_ARG_I(1),"complex integer power") 

// -----------------------------------------------------

VASP_UNARY("vasp.cabs",cabs,true,"set real part to complex absolute value, imaginary part becomes zero") 

