/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_arith.h"
#include "opdefs.h"
#include "util.h"


VASP_BINARY("vasp.+",add,true,VASP_ARG_R(0),"Adds a value, envelope or vasp")
VASP_BINARY("vasp.-",sub,true,VASP_ARG_R(0),"Subtracts a value, envelope or vasp")
VASP_BINARY("vasp.!-",subr,true,VASP_ARG_R(0),"Reverse subtracts a value, envelope or vasp")
VASP_BINARY("vasp.*",mul,true,VASP_ARG_R(1),"Multiplies with a value, envelope or vasp")
VASP_BINARY("vasp./",div,true,VASP_ARG_R(1),"Divides by a value, envelope or vasp")
VASP_BINARY("vasp.!/",divr,true,VASP_ARG_R(1),"Reverse divides by a value, envelope or vasp")
VASP_BINARY("vasp.%",mod,true,VASP_ARG_R(0),"Calculates the remainder of the division by a value, envelope or vasp")

// -----------------------------------------------------

VASP_UNARY("vasp.sqr",sqr,true,"Calculates the square") 
VASP_UNARY("vasp.ssqr",ssqr,true,"Calculates the square with preservation of the sign") 

// -----------------------------------------------------

VASP_UNARY("vasp.sign",sign,true,"Calculates the sign (signum function)") 
VASP_UNARY("vasp.abs",abs,true,"Calculates the absolute value") 



/*! \class vasp_qsum
	\remark \b vasp.sum?
	\brief Get sum of sample values
	\since 0.1.3
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet float - sample value sum

	\todo Should we provide a cmdln default vasp?
	\todo Should we inhibit output for invalid vasps?
	\remark Returns 0 for a vasp with 0 frames
*/
class vasp_qsum:
	public vasp_unop
{
	FLEXT_HEADER(vasp_qsum,vasp_unop)

public:
	vasp_qsum(): vasp_unop(true,XletCode(xlet_float,0)) {}

	virtual Vasp *do_opt(OpParam &p) 
	{ 
		p.norm.minmax = 0;
		CVasp cref(ref);
		Vasp *ret = VaspOp::m_qsum(p,cref); 
		return ret;
	}
		
	virtual Vasp *tx_work() 
	{ 
		OpParam p(thisName(),0);													
		Vasp *ret = do_opt(p);
		ToOutFloat(1,p.norm.minmax);
		return ret;
	}

	virtual V m_help() { post("%s - Get the sum of a vasp's sample values",thisName()); }
};

VASP_LIB("vasp.sum?",vasp_qsum)

