/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_feature.h"
#include "oploop.h"
#include "util.h"

// --- find peaks

BL higher(S a,S b) { return a > b; }
BL lower(S a,S b) { return a < b; }

/*! \brief Find peaks or valleys (depending on cmp function)
	\param rep repetition count
	
	\remark real peak search is mangled into complex domain

	\todo how to treat <=, >=
	\todo separate real and complex functionality
*/

static BL d_vlpk(OpParam &p,BL cmpf(S a,S b)) 
{ 
	I dpeaks = (I)(p.frames*p.peaks.density);
	if(dpeaks < 1) dpeaks = 1;

	I cnt;
	do {
		cnt = 0;

		I i;
		BS *rdst = p.rddt,*rsrc = p.rsdt;
		BS *idst = p.iddt,*isrc = p.isdt;
		
		if(!p.peaks.cx || !idst) idst = rdst,p.ids = p.rds;
		if(!p.peaks.cx || !isrc) isrc = rsrc,p.iss = p.rss;

		// preset sample values
		S d1 = -1,d0 = -1,dn = -1;

		// search first non-null sample
		_D_LOOP(i,p.frames)
			if((dn = sqabs(rsrc[i*p.rss],isrc[i*p.iss])) != 0)
				break; // non-null -> break!
			else 
				rdst[i*p.rds] = idst[i*p.ids] = 0; // copy null samples to dst
		_E_LOOP

		// i points to first non-null sample

		_D_WHILE(i < p.frames)
			// current samples -> previous samples
			d1 = d0,d0 = dn;

			// save current index 
			I ci = i;

			// search next non-null sample
			dn = -1;
			while(++i < p.frames) 
				if((dn = sqabs(rsrc[i*p.rss],isrc[i*p.iss])) != 0) {
					break; // non-null -> break!
				}
				else 
					rdst[i*p.rds] = idst[i*p.ids] = 0;

			if((d1 < 0 || cmpf(d0,d1)) && (dn < 0 || cmpf(d0,dn))) {
				// is peak/valley
				rdst[ci*p.rds] = rsrc[ci*p.rss];
				idst[ci*p.ids] = isrc[ci*p.iss];
				++cnt;
			}
			else
				rdst[ci*p.rds] = idst[ci*p.ids] = 0;
		_E_WHILE
	} while(cnt > dpeaks);

	p.peaks.density = p.frames?(R)cnt/p.frames:(cnt?1:0);
	return true; 
}

inline BL d_peaks(OpParam &p) { return d_vlpk(p,higher); }
inline BL d_valleys(OpParam &p) { return d_vlpk(p,lower); }
inline BL d_rpeaks(OpParam &p) { return d_vlpk(p,higher); }
inline BL d_rvalleys(OpParam &p) { return d_vlpk(p,lower); }

/*! \brief Finds peaks or valleys in a vasp.

	\param arg argument list 
	\param arg.rep repetition count
	\param dst destination vasp (NULL for in-place operation)
	\param inv true for valley operation
	\return normalized destination vasp
*/
Vasp *VaspOp::m_peaks(OpParam &p,CVasp &src,CVasp *dst,BL inv) 
{ 
	Vasp *ret = NULL;
	RVecBlock *vecs = GetRVecs(p.opname,src,dst);
	if(vecs) {
		p.peaks.cx = false;
		ret = DoOp(vecs,inv?d_valleys:d_peaks,p);
		delete vecs;
	}
	return ret;
}



/*! \brief Finds peaks or valleys by radius in a complex vasp.

	\param arg argument list 
	\param arg.rep repetition count
	\param dst destination vasp (NULL for in-place operation)
	\param inv true for valley operation
	\return normalized destination vasp
*/
Vasp *VaspOp::m_rpeaks(OpParam &p,CVasp &src,CVasp *dst,BL inv) 
{ 
	Vasp *ret = NULL;
	CVecBlock *vecs = GetCVecs(p.opname,src,dst);
	if(vecs) {
		p.peaks.cx = true;
		ret = DoOp(vecs,inv?d_rvalleys:d_rpeaks,p);
		delete vecs;
	}
	return ret;
}


class vasp_peaks:
	public vasp_anyop
{																				
	FLEXT_HEADER(vasp_peaks,vasp_anyop)
public:			
	
	vasp_peaks(I argc,const t_atom *argv): 
		vasp_anyop(argc,argv,VASP_ARG(),true,XletCode(xlet_float,0)) 
	{}

	virtual Vasp *do_peaks(OpParam &p) 
	{ 
		CVasp cdst(dst),cref(ref); 
		return VaspOp::m_peaks(p,cref,&cdst); 
	}
		
	virtual Vasp *tx_work(const Argument &arg) 
	{ 
		OpParam p(thisName(),0);													
		
		if(arg.IsList() && arg.GetList().Count() >= 1 && CanbeFloat(arg.GetList()[0])) {
			p.peaks.density = GetAFloat(arg.GetList()[0]);
		}
		else {
			if(!arg.IsNone()) post("%s - invalid density argument -> set to 1",p.opname);
			p.peaks.density = 1;
		}
		
		Vasp *ret = do_peaks(p);
		ToOutFloat(1,p.peaks.density);
		return ret;
	}

	virtual V m_help() { post("%s - Get non-zero values only for peaks",thisName()); }
};																				
VASP_LIB_V("vasp.peaks",vasp_peaks)


class vasp_valleys:
	public vasp_peaks
{																				
	FLEXT_HEADER(vasp_valleys,vasp_peaks)
public:			
	vasp_valleys(I argc,const t_atom *argv): vasp_peaks(argc,argv) {}
	virtual Vasp *do_peaks(OpParam &p) 
	{ 
		CVasp cdst(dst),cref(ref);
		return VaspOp::m_valleys(p,cref,&cdst); 
	}

	virtual V m_help() { post("%s - Get non-zero values only for values",thisName()); }
};																				
VASP_LIB_V("vasp.valleys",vasp_valleys)


class vasp_rpeaks:
	public vasp_peaks
{																				
	FLEXT_HEADER(vasp_rpeaks,vasp_peaks)
public:			
	vasp_rpeaks(I argc,const t_atom *argv): vasp_peaks(argc,argv) {}
	virtual Vasp *do_peaks(OpParam &p) 
	{ 
		CVasp cdst(dst),cref(ref);
		return VaspOp::m_rpeaks(p,cref,&cdst); 
	}

	virtual V m_help() { post("%s - Get non-zero values only for peaks of the complex radius",thisName()); }
};																				
VASP_LIB_V("vasp.rpeaks",vasp_rpeaks)


class vasp_rvalleys:
	public vasp_peaks
{																				
	FLEXT_HEADER(vasp_rvalleys,vasp_peaks)
public:			
	vasp_rvalleys(I argc,const t_atom *argv): vasp_peaks(argc,argv) {}
	virtual Vasp *do_peaks(OpParam &p) 
	{ 
		CVasp cdst(dst),cref(ref);
		return VaspOp::m_rvalleys(p,cref,&cdst); 
	}

	virtual V m_help() { post("%s - Get non-zero values only for valleys of the complex radius",thisName()); }
};																				
VASP_LIB_V("vasp.rvalleys",vasp_rvalleys)

