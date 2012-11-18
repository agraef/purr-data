/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

/*! \file vasp__resmp.cpp
	\brief Routines for resampling
*/

#include "main.h"
#include "ops_resmp.h"


// --- resample ---------------------

/*! \brief Subroutine for resampling.

	\param cnt frame count
	\param src source data
	\param sstr source data stride
	\param dst destination data
	\param dstr destination data stride
	\param factor resampling factor
	\param center resampling center (this will remain untouched by the transformation)
	\param mode interpolation mode
	\return true on success

	\todo implement averaging interpolation methods
	\todo check for operation direction!
	\todo check for src/dst overlap
	\todo support different frame lengths for src and dst
*/

static V do_tilt(OpParam &p)
{
	I mode = p.tilt.mode;
	if(mode == 0&& (p.tilt.factor < 0 || p.tilt.center < 0 || p.tilt.factor >= p.frames)) mode = 1;

	const R center = p.tilt.center;
	const I icenter = (I)center;
	BS fll,flr;
	if(p.tilt.fill) 
		fll = p.rsdt[0],flr = p.rsdt[p.frames-1];
	else
		fll = flr = 0;

	if(mode >= 1 && mode <= 3) {
		BS *tmp;
		I rss;
		if(p.rsdt == p.rddt) {
			tmp = new BS[p.frames],rss = 1;
			for(int i = 0; i < p.frames; ++i) tmp[i] = p.rsdt[i*p.rss];
		}
		else tmp = p.rsdt,rss = p.rss;

		switch(mode) {
		case 1: {
			// no interpolation
			for(int i = 0; i < p.frames; ++i) {
				I pi = (I)(center+(i-center)*p.tilt.factor);
				p.rddt[i*p.rds] = pi >= 0?(pi < p.frames?tmp[pi*rss]:flr):fll;
			}
			break;
		}
		case 2: {
			// linear interpolation
			for(int i = 0; i < p.frames; ++i) {
				R pr = center+(i-center)*p.tilt.factor,v;
				I ip = (I)pr;
				if(ip >= 0)
					if(ip < p.frames-1) {
						R r = pr-ip;
						v = (1-r)*tmp[ip*rss]+r*tmp[(ip+1)*rss];
					}
					else
						v = ip == p.frames-1?tmp[ip*rss]:flr;
				else
					v = fll;
				p.rddt[i*p.rds] = v;
			}
			break;
		}
		case 3: {
			// 4-point interpolation
			R f[4];
			for(int i = 0; i < p.frames; ++i) {
				R pr = center+(i-center)*p.tilt.factor;
				const I ip = (I)pr;
				const BS *t = tmp+ip*rss;
				const R r = pr-ip;
				if(ip >= 1) 
					if(ip < p.frames-2) {
						f[0] = t[-rss];
						f[1] = t[0];
						f[2] = t[rss];
						f[3] = t[rss*2];
					}
					else {
						f[0] = ip < p.frames+1?t[-rss]:flr;
						f[1] = ip < p.frames?t[0]:flr;
						f[2] = ip < p.frames-1?t[rss]:flr;
						f[3] = flr;
					}
				else {
					f[0] = fll;
					f[1] = ip >= 0?t[0]:fll;
					f[2] = ip >= -1?t[rss]:fll;
					f[3] = ip >= -2?t[rss*2]:fll;
				}

				const R cmb = f[2]-f[1];
				p.rddt[i*p.rds] = f[1]+r*(cmb-0.5*(r-1.)*((f[0]-f[3]+3.*cmb)*r+(f[1]-f[0]-cmb)));
			}
			break;
		}
		}

		if(p.rsdt == p.rddt) delete[] tmp;
	}
	else {
		const R rem = center-icenter; // 0 <= rem < 1
		// quick and dirty... but in-place!

		if(p.tilt.factor > 1) {
			I i;
			for(i = 0; i <= icenter; ++i) {
				I sp = (I)(center-(i+rem)*p.tilt.factor);
				p.rddt[(icenter-i)*p.rds] = sp >= 0?(sp < p.frames?p.rsdt[sp*p.rss]:flr):fll;
			}
			for(i = 1; i < p.frames-icenter; ++i) {
				I sp = (I)(center+(i-rem)*p.tilt.factor);
				p.rddt[(icenter+i)*p.rds] = sp >= 0?(sp < p.frames?p.rsdt[sp*p.rss]:flr):fll;
			}
		}
		else {
			I i;
			for(i = icenter; i >= 0; --i) {
				I sp = (I)(center-(i+rem)*p.tilt.factor);
				p.rddt[(icenter-i)*p.rds] = p.rsdt[sp*p.rss];
			}
			for(i = p.frames-1-icenter; i > 0; --i) {
				I sp = (I)(center+(i-rem)*p.tilt.factor);
				p.rddt[(icenter+i)*p.rds] = p.rsdt[sp*p.rss];
			}
		}
	}
}


BL VecOp::d_tilt(OpParam &p) 
{ 
	if(p.frames <= 1 || p.tilt.factor == 1) return true;

	// symmetric operation
	if(p.symm == 1) 
		p.tilt.center = p.frames-1-p.tilt.center;

	do_tilt(p); 

	return true;
}


/*! \brief Does vasp resampling.

	\param arg argument list 
	\param arg.factor factor for resampling 
	\param arg.center center of resampling
	\param dst destination vasp (NULL for in-place operation)
	\param symm true for symmetric operation
	\param mode interpolation mode
	\return normalized destination vasp
*/
Vasp *VaspOp::m_tilt(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL symm) 
{ 
	Vasp *ret = NULL;
	if(arg.IsList() && arg.GetList().Count() >= 1) {
		RVecBlock *vecs = GetRVecs(p.opname,src,dst);
		if(vecs) {
			p.tilt.factor = flext::GetAFloat(arg.GetList()[0]);
			p.tilt.center = arg.GetList().Count() >= 2?flext::GetAFloat(arg.GetList()[1]):0;

			ret = DoOp(vecs,VecOp::d_tilt,p,symm);

			delete vecs;
		}
	}
	else
		post("%s - no arguments: no operation",p.opName());

	return ret;
}



class vasp_tilt:
	public vasp_anyop
{																				
	FLEXT_HEADER_S(vasp_tilt,vasp_anyop,Setup)
public:			
	
	vasp_tilt(I argc,t_atom *argv): 
		vasp_anyop(argc,argv,VASP_ARG_R(1),true),
		fill(xtf_zero),inter(xti_4p)
	{}

	static V Setup(t_classid c)
	{
		FLEXT_CADDATTR_VAR1_E(c,"fill",fill);
		FLEXT_CADDATTR_VAR1_E(c,"inter",inter);
	}

	enum xt_fill {
		xtf__ = -1,  // don't change
		xtf_zero = 0,xtf_edge
	};	

	enum xt_inter {
		xti__ = -1,  // don't change
		xti_inpl = 0,xti_none,xti_lin,xti_4p
	};	

	virtual Vasp *do_shift(OpParam &p) 
	{ 
		CVasp cdst(dst),cref(ref);
		return VaspOp::m_tilt(p,cref,arg,&cdst); 
	}
		
	virtual Vasp *tx_work(const Argument &arg) 
	{ 
		OpParam p(thisName(),1);													
		p.tilt.fill  = (I)fill;
		p.tilt.mode  = (I)inter;

		Vasp *ret = do_shift(p);
		return ret;
	}

	virtual V m_help() { post("%s - Resamples buffer data",thisName()); }

protected:
	xt_fill fill;
	xt_inter inter;

private:
	FLEXT_ATTRVAR_E(fill,xt_fill)
	FLEXT_ATTRVAR_E(inter,xt_inter)
};																				
VASP_LIB_V("vasp.tilt",vasp_tilt)


class vasp_xtilt:
	public vasp_tilt
{																				
	FLEXT_HEADER(vasp_xtilt,vasp_tilt)
public:			
	
	vasp_xtilt(I argc,t_atom *argv): vasp_tilt(argc,argv) {}

	virtual Vasp *do_shift(OpParam &p) 
	{ 
		CVasp cdst(dst),cref(ref);
		return VaspOp::m_xtilt(p,cref,arg,&cdst); 
	}
		
	virtual V m_help() { post("%s - Resamples buffer data symmetrically (in two halves)",thisName()); }
};																				
VASP_LIB_V("vasp.xtilt",vasp_xtilt)


