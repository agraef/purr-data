/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_flt.h"
#include "opdefs.h"
#include "util.h"

// --- highpass ---------------------------------------

//! \todo handle carry
//! \todo handle yield

BL VecOp::d_fhp(OpParam &p) 
{ 
	if(p.revdir)
		post("%s - reversing operation direction due to overlap: opposite sample delay",p.opname);

/*
	R coef = (2*M_PI)/perln;
    if(coef > 1) coef = 1;
*/  
    const R coef = 1-p.flt.coef;
	const I arep = abs(p.flt.rep);
	BS *src = p.rsdt,*dst = p.rddt;

	for(I ti = 0; ti < arep; ++ti) {
		register S v1;
		I i;

		// t+ direction
		for(i = 0,v1 = 0; i < p.frames; ++i) {
			register const S v0 = *src + coef*v1;
			*dst = v0-v1;
			v1 = v0;
			src += p.rss,dst += p.rds;
		}
		
		if(p.flt.rep < 0) {
			if(++ti == arep) break;

			// t- direction
			for(i = p.frames-1,v1 = 0; i >= 0; --i) {
				src -= p.rss,dst -= p.rds;
				register const S v0 = *src + coef*v1;
				*dst = v0-v1;
				v1 = v0;
			}
		}
	}

	return true;
}


// --- lowpass ---------------------------------------

//! \todo handle carry
//! \todo handle yield

BL VecOp::d_flp(OpParam &p) 
{ 
	if(p.revdir)
		post("%s - reversing operation direction due to overlap: opposite sample delay",p.opname);

/*
	R coef = (2*M_PI)/perln;
    if(coef > 1) coef = 1;
*/    
    
	const R coef = p.flt.coef,feed = 1-coef;
	const I arep = abs(p.flt.rep);

	for(I ti = 0; ti < arep; ++ti) {
		register S v1;
		I i;
		BS *src = p.rsdt,*dst = p.rddt;

		// t+ direction
		for(i = 0,v1 = 0; i < p.frames; ++i) {
			v1 = *dst = coef* *src + feed*v1;
			src += p.rss,dst += p.rds;
		}
		
		if(p.flt.rep < 0) {
			if(++ti == arep) break;

			// t- direction
			for(i = p.frames-1,v1 = 0; i >= 0; --i) {
				src -= p.rss,dst -= p.rds;
				v1 = *dst = coef* *src + feed*v1;
			}
		}
	}

	return true;
}



Vasp *VaspOp::m_fhp(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL hp) 
{ 
	Vasp *ret = NULL;
	if(arg.IsList() && arg.GetList().Count() >= 1) {
		RVecBlock *vecs = GetRVecs(p.opname,src,dst);
		if(vecs) {
			p.flt.coef = 2*M_PI/flext::GetAFloat(arg.GetList()[0]);
		    if(p.flt.coef > 1) p.flt.coef = 1;
			p.flt.rep = arg.GetList().Count() >= 2?flext::GetAInt(arg.GetList()[1]):1;
			p.flt.rep = -p.flt.rep;  // fwd/bwd operation
/*
			if(p.SROvr()) {
				p.SDRRev();
				post("%s - reversing operation direction due to overlap: opposite sample delay",opnm);
			}	
*/
			ret = DoOp(vecs,hp?VecOp::d_fhp:VecOp::d_flp,p);

			delete vecs;
		}
	}

	return ret;
}

VASP_ANYOP("vasp.flp",flp,1,true,VASP_ARG(),"Passive low pass filter")
VASP_ANYOP("vasp.fhp",fhp,1,true,VASP_ARG(),"Passive high pass filter")


// --- integrate/differentiate

/*! \brief Integration
	\remark The delay of the result is +/- one sample, depending on the direction of the calculation 
	
	\todo different modes how to initialize first carry?
	\todo repetition count
*/
BL VecOp::d_int(OpParam &p) 
{ 
	if(p.revdir)
		post("%s - reversed operation direction due to overlap: opposite sample delay",p.opname);

	register S d = p.intdif.carry;
	register I i;
	_DE_LOOP(i,p.frames, ( *p.rddt = (d += *p.rsdt), p.rsdt += p.rss,p.rddt += p.rds ) )
	p.intdif.carry = d;
	return true; 
}

/*! \brief Differentiation
	\remark The delay of the result is +/- one sample, depending on the direction of the calculation 

	\todo different modes how to initialize first carry?
	\todo repetition count
*/
BL VecOp::d_dif(OpParam &p) 
{ 
	if(p.revdir)
		post("%s - reversed operation direction due to overlap: opposite sample delay",p.opname);

	register S d = p.intdif.carry,d1;
	register I i;
	_DE_LOOP(i,p.frames, ( d1 = *p.rsdt, *p.rddt = d1-d,d = d1, p.rsdt += p.rss,p.rddt += p.rds ) )
	p.intdif.carry = d;
	return true; 
}

/*! \brief Does vasp integration/differentiation.

	\param arg argument list 
	\param dst destination vasp (NULL for in-place operation)
	\param inv true for differentiation
	\return normalized destination vasp
*/
Vasp *VaspOp::m_int(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL inv) 
{ 
	Vasp *ret = NULL;
	RVecBlock *vecs = GetRVecs(p.opname,src,dst);
	if(vecs) {
		p.intdif.carry = 0,p.intdif.rep = 1;
		if(arg.IsList() && arg.GetList().Count() >= 1) p.intdif.rep = flext::GetAInt(arg.GetList()[0]);
		
		if(p.intdif.rep < 0) {
			post("%s - invalid repetition count (%i) -> set to 1",p.opname,p.intdif.rep);
			p.intdif.rep = 1;
		}
		
		ret = DoOp(vecs,inv?VecOp::d_dif:VecOp::d_int,p);
		delete vecs;
	}
	return ret;
}

VASP_ANYOP("vasp.int",int,0,true,VASP_ARG_I(1),"Integration") 
VASP_ANYOP("vasp.dif",dif,0,true,VASP_ARG_I(1),"Differentiation") 


VASP_UNARY("vasp.fix",fix,true,"Bashes denormals/NANs to zero") 
