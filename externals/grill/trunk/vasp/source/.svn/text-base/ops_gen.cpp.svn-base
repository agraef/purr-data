/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_gen.h"
#include "util.h"

// --- osc ---------------------------------------

/*!	\brief Generator for real (cos) oscillations
*/
BL VecOp::d_osc(OpParam &p) 
{ 
	register R ph = p.gen.ph,phinc = p.gen.phinc; 
	if(p.revdir) ph -= (p.frames-1)*(phinc = -phinc);

	for(I i = 0; i < p.frames; ++i,ph += phinc,p.rddt += p.rds) *p.rddt = cos(ph);
	return true;
}

/*!	\brief multiplicative generator for real (cos) oscillations
*/
BL VecOp::d_mosc(OpParam &p) 
{ 
	register R ph = p.gen.ph,phinc = p.gen.phinc; 
	if(p.revdir) ph -= (p.frames-1)*(phinc = -phinc);

	for(I i = 0; i < p.frames; ++i,ph += phinc,p.rsdt += p.rss,p.rddt += p.rds) 
		*p.rddt = *p.rsdt * cos(ph);
	return true;
}

/*! \brief Generator for real (cos) oscillations.

	\param arg argument list 
	\param arg.perlen Period length (in samples)
	\param arg.stph Starting phase
	\param mul true for multiplication to exisiting date
	\return normalized destination vasp

	\todo Replace period length by frequency specification
*/
Vasp *VaspOp::m_osc(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL mul) 
{ 
	Vasp *ret = NULL;
	if(arg.IsList() && arg.GetList().Count() >= 1) {
		RVecBlock *vecs = GetRVecs(p.opname,src,dst);
		if(vecs) {
			// period length
			p.gen.phinc = 2*M_PI/flext::GetAFloat(arg.GetList()[0]); 
			// starting phase
			p.gen.ph = arg.GetList().Count() >= 2?flext::GetAFloat(arg.GetList()[1]):0;

			ret = DoOp(vecs,mul?VecOp::d_mosc:VecOp::d_osc,p);
			delete vecs;
		}
	}
	else
		post("%s - no arguments: no operation",p.opName());

	return ret;
}


/*!	\brief Generator for complex oscillations.
*/
BL VecOp::d_cosc(OpParam &p) 
{ 
	register R ph = p.gen.ph,phinc = p.gen.phinc;
	if(p.revdir) ph -= (p.frames-1)*(phinc = -phinc);

	for(; p.frames--; ph += phinc,p.rddt += p.rds,p.iddt += p.ids) 
		*p.rddt = cos(ph),*p.iddt = sin(ph);
	return true;
}

/*!	\brief Multiplicative generator for complex oscillations.
*/
BL VecOp::d_mcosc(OpParam &p) 
{ 
	register R ph = p.gen.ph,phinc = p.gen.phinc;
	if(p.revdir) ph -= (p.frames-1)*(phinc = -phinc);

	for(; p.frames--; ph += phinc,p.rsdt += p.rss,p.isdt += p.iss,p.rddt += p.rds,p.iddt += p.ids) {
		R zre = cos(ph),zim = sin(ph);

		register const R r = *p.rsdt * zre - *p.isdt * zim;
		*p.iddt = *p.isdt * zre + *p.rsdt * zim;
		*p.rddt = r;
	}
	return true;
}

/*! \brief Generator for complex (cos+i sin) oscillations.

	\param arg argument list 
	\param arg.perlen Period length (in samples)
	\param arg.stph Starting phase
	\param mul true for multiplication to exisiting date
	\return normalized destination vasp

	\todo Replace period length by frequency specification
*/
Vasp *VaspOp::m_cosc(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL mul) 
{ 
	Vasp *ret = NULL;
	if(arg.IsList() && arg.GetList().Count() >= 1) {
		CVecBlock *vecs = GetCVecs(p.opname,src,dst);
		if(vecs) {
			// period length
			p.gen.phinc = 2*M_PI/flext::GetAFloat(arg.GetList()[0]); 
			// starting phase
			p.gen.ph = arg.GetList().Count() >= 2?flext::GetAFloat(arg.GetList()[1]):0;

			ret = DoOp(vecs,mul?VecOp::d_mcosc:VecOp::d_cosc,p);
			delete vecs;
		}
	}
	else
		post("%s - no arguments: no operation",p.opName());

	return ret;
}


VASP_ANYOP("vasp.osc",osc,1,false,VASP_ARG(),"Calculates a cosine wave")
VASP_ANYOP("vasp.*osc",mosc,1,true,VASP_ARG(),"Multiplies with a cosine wave")
VASP_ANYOP("vasp.cosc",cosc,1,false,VASP_ARG(),"Calculates a complex oscillation (cos + i sin)")
VASP_ANYOP("vasp.*cosc",mcosc,1,true,VASP_ARG(),"Multiplies with a complex oscillation (cos + i sin)")

// --- phasor ---------------------------------------

// ! look up Hˆldrich's pd phasor code

/*!	\brief Generator for saw wave oscillations.
*/
BL VecOp::d_phasor(OpParam &p) 
{ 
	register R ph = p.gen.ph,phinc = p.gen.phinc;
	if(p.revdir) ph -= (p.frames-1)*(phinc = -phinc);

	for(; p.frames--; ph += phinc,p.rddt += p.rds) *p.rddt = fmod((F)ph,1.F);
	return true;
}

/*!	\brief Multiplicative generator for saw wave oscillations.
*/
BL VecOp::d_mphasor(OpParam &p) 
{ 
	register R ph = p.gen.ph,phinc = p.gen.phinc;
	if(p.revdir) ph -= (p.frames-1)*(phinc = -phinc);

	for(; p.frames--; ph += phinc,p.rddt += p.rds,p.rsdt += p.rss) *p.rddt = *p.rsdt * fmod((F)ph,1.F);
	return true;
}

/*! \brief Generator for sawtooth oscillations.

	\param arg argument list 
	\param arg.perlen Period length (in samples)
	\param arg.stph Starting phase
	\param mul true for multiplication to exisiting date
	\return normalized destination vasp

	\todo Replace period length by frequency specification
*/
Vasp *VaspOp::m_phasor(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL mul) 
{ 
	Vasp *ret = NULL;
	if(arg.IsList() && arg.GetList().Count() >= 1) {
		RVecBlock *vecs = GetRVecs(p.opname,src,dst);
		if(vecs) {
			// period length
			p.gen.phinc = 1./flext::GetAFloat(arg.GetList()[0]); 
			// starting phase
			p.gen.ph = arg.GetList().Count() >= 2?flext::GetAFloat(arg.GetList()[1]):0;
		
			ret = DoOp(vecs,mul?VecOp::d_mphasor:VecOp::d_phasor,p);
			delete vecs;
		}
	}
	else
		post("%s - no arguments: no operation",p.opName());

	return ret;
}


VASP_ANYOP("vasp.phasor",phasor,1,false,VASP_ARG(),"Calculates a sawtooth wave")
VASP_ANYOP("vasp.*phasor",mphasor,1,true,VASP_ARG(),"Multiplies with a sawtooth wave")

// --- noise --------------------------------

static F rnd() 
{
    static I val = 307*1319;
    F ret = ((F)((val&0x7fffffff)-0x40000000))*(F)(1.0/0x40000000);
    val = val * 435898247 + 382842987;
	return ret;
}

/*!	\brief Vector function for pseudorandom noise.
*/
BL VecOp::d_noise(OpParam &p) 
{ 
	for(; p.frames--; p.rddt += p.rds) *p.rddt = rnd();
	return true;
}


/*! \brief Generator for real valued noise.

	\return normalized destination vasp
*/
Vasp *VaspOp::m_noise(OpParam &p,CVasp &src,CVasp *dst) 
{ 
	Vasp *ret = NULL;
	RVecBlock *vecs = GetRVecs(p.opname,src,dst);
	if(vecs) {
		ret = DoOp(vecs,VecOp::d_noise,p);
		delete vecs;
	}
	return ret;
}

/*!	\brief Vector function for pseudorandom complex noise.
*/
BL VecOp::d_cnoise(OpParam &p) 
{ 
	for(; p.frames--; p.rddt += p.rds,p.iddt += p.ids) {
		R amp = rnd();
		R arg = rnd()*(2.*M_PI);
		*p.rddt = amp*cos(arg);
		*p.iddt = amp*sin(arg);
	}
	return true;
}

/*! \brief Generator for complex noise (complex abs, complex arg).

	\return normalized destination vasp

	\todo Replace period length by frequency specification
*/
Vasp *VaspOp::m_cnoise(OpParam &p,CVasp &src,CVasp *dst) 
{ 
	Vasp *ret = NULL;
	CVecBlock *vecs = GetCVecs(p.opname,src,dst);
	if(vecs) {
		ret = DoOp(vecs,VecOp::d_cnoise,p);
		delete vecs;
	}
	return ret;
}

VASP_UNARY("vasp.noise",noise,false,"Fills the vectors with white noise")
VASP_UNARY("vasp.cnoise",cnoise,false,"Fills the vectors with complex white noise (radius and angle are random)")

