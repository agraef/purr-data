/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_search.h"
#include "util.h"
#include "oploop.h"


BL VecOp::d_search(OpParam &p) 
{ 
	const I off = p.srch.offs;
	const S val = p.srch.val;
	const R cur = p.rsdt[off];

	I i,ofl = -1,ofr = -1;
	
    if(p.srch.incl && cur == val) {
        // if @incl attribute is set and current sample matches 
        ofl = ofr = off;
    }
    else {
	    if(p.srch.dir <= 0) {
		    BL y = cur >= val;
		    i = off-1;
		    _D_WHILE(i >= 0)
			    BL y2 = p.rsdt[i] >= val;
			    if(y != y2) {
				    if(p.srch.slope <= 0 && y2) break; 
				    if(p.srch.slope >= 0 && !y2) break;
			    }
			    y = y2;
			    --i;
		    _E_WHILE

		    if(i >= 0) ofl = i;
	    }

	    if(p.srch.dir >= 0) {
		    BL y = cur >= val;
		    i = off+1; 
		    _D_WHILE(i < p.frames)
			    BL y2 = p.rsdt[i] >= val;
			    if(y != y2) {
				    if(p.srch.slope <= 0 && !y2) break;
				    if(p.srch.slope >= 0 && y2) break;
			    }
			    y = y2;
			    ++i;
		    _E_WHILE

		    if(i < p.frames) ofr = i;
	    }
    }

	if(!p.srch.dir) {
		if(ofl >= 0) {
			p.srch.dif = ofl-off;
			if(ofr >= 0 && abs(p.srch.dif) < abs(ofr-off)) p.srch.dif = ofr-off;
		}
		else 
			p.srch.dif = ofr >= 0?ofr-off:0;
	}
	else if(p.srch.dir > 0) 
		p.srch.dif = ofr >= 0?ofr-off:0;
	else
		p.srch.dif = ofl >= 0?ofl-off:0;

	return true;
}

Vasp *VaspOp::m_search(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL st) 
{ 
	Vasp *ret = NULL;
	if(src.Vectors() != 1) 
		post("%s - Need exactly one vector in vasp!",p.opName());
	else if(arg.CanbeFloat() || (arg.IsList() && arg.GetList().Count() >= 1)) {
		I fr = src.Frames();
		I o = src.Vector(0).Offset();

        VBuffer *buf = src.Buffer(0);
        I sz = buf->Frames();
        delete buf;

		CVasp all(src);
		if(st) {
			// search start point
			p.srch.offs = o;

			// set bounds of search buffer
			all.Offset(0);
			all.Frames(fr+o);  // all frames of buffer
		}
		else {
			// search end point
			p.srch.offs = o+fr;
			// check if current offset is past buffer
			if(p.srch.offs >= sz) p.srch.offs = sz-1;

			// set bounds of search buffer
			all.Offset(o);
			all.Frames(sz-o);  // all frames of buffer			
		}

		RVecBlock *vecs = GetRVecs(p.opname,all,dst);
		if(vecs) {
			p.srch.val = arg.IsList()?flext::GetAFloat(arg.GetList()[0]):arg.GetAFloat();

			ret = DoOp(vecs,VecOp::d_search,p);

			if(st) o += p.srch.dif,fr -= p.srch.dif;
			else fr += p.srch.dif;

			if(ret) {
				ret->Offset(o);
				ret->Frames(fr);
				ret->Frames(ret->ChkFrames()); // What's that????
			}

			delete vecs;
		}
	}
	else
		post("%s - no arguments: no operation",p.opName());

	return ret;
}


class vasp_search:
	public vasp_anyop
{																				
	FLEXT_HEADER_S(vasp_search,vasp_anyop,Setup)
public:			
	
	vasp_search(I argc,t_atom *argv): 
		vasp_anyop(argc,argv,VASP_ARG_R(0),false,XletCode(xlet_float,0)),
		slope(0),dir(0),incl(false)
	{}

	static V Setup(t_classid c)
	{
		FLEXT_CADDATTR_VAR1(c,"dir",dir);
		FLEXT_CADDATTR_VAR1(c,"slope",slope);
		FLEXT_CADDATTR_VAR1(c,"incl",incl);
	}

	virtual Vasp *do_work(OpParam &p) = 0;
		
	virtual Vasp *tx_work(const Argument &arg) 
	{ 
		OpParam p(thisName(),1);													
		p.srch.dir = dir;
		p.srch.slope  = slope;
        p.srch.incl = incl;

		Vasp *ret = do_work(p);
		if(ret) ToOutFloat(1,p.srch.dif);
		return ret;
	}

protected:
	I dir,slope;
    BL incl;

private:
	FLEXT_ATTRVAR_I(dir)
	FLEXT_ATTRVAR_I(slope)
	FLEXT_ATTRVAR_B(incl)
};																				


class vasp_soffset:
	public vasp_search
{																				
	FLEXT_HEADER(vasp_soffset,vasp_search)
public:			
	vasp_soffset(I argc,t_atom *argv): vasp_search(argc,argv) {}
	virtual Vasp *do_work(OpParam &p) 
	{ 
		CVasp cdst(dst),cref(ref);
		return VaspOp::m_soffset(p,cref,arg,&cdst); 
	}

	virtual V m_help() { post("%s - Define starting point by searching for given value",thisName()); }
};																				
VASP_LIB_V("vasp.offset= vasp.o=",vasp_soffset)


class vasp_sframes:
	public vasp_search
{																				
	FLEXT_HEADER(vasp_sframes,vasp_search)
public:			
	vasp_sframes(I argc,t_atom *argv): vasp_search(argc,argv) {}
	virtual Vasp *do_work(OpParam &p) 
	{ 
		CVasp cdst(dst),cref(ref);
		return VaspOp::m_sframes(p,cref,arg,&cdst); 
	}

	virtual V m_help() { post("%s - Define vasp frame length by searching for given value",thisName()); }
};																				
VASP_LIB_V("vasp.frames= vasp.f=",vasp_sframes)


