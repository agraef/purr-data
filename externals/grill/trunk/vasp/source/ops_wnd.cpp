/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_wnd.h"
#include "oploop.h"
#include <cstring>

// --- window --------------------------

typedef R (*wfunc)(R i,const OpParam &p);

inline R wf_sin(R i,const OpParam &p) { return sin(i*p.wnd.p1+p.wnd.p2); }
inline R wf_hanning(R i,const OpParam &p) { return 0.5*(1+cos(i*p.wnd.p1+p.wnd.p2)); }
inline R wf_hamming(R i,const OpParam &p) { return 0.54 + 0.46 * cos(i*p.wnd.p1+p.wnd.p2); }
inline R wf_blackman(R i,const OpParam &p) { const R x = i*p.wnd.p1+p.wnd.p2; return 0.42+0.5*cos(x)+0.08*cos(2*x); }
inline R wf_connes(R i,const OpParam &p) { const R x = i*p.wnd.p1+p.wnd.p2,x2 = 1-x*x; return x2*x2; }
inline R wf_welch(R i,const OpParam &p) { const R x = i*p.wnd.p1+p.wnd.p2; return 1-x*x; }
inline R wf_lanczos(R i,const OpParam &p) { const R x = i*p.wnd.p1+p.wnd.p2; return x?sin(x)/x:1; }
//inline R wf_gaussian(R i,const OpParam &p) { const R x = i*p.wnd.p1+p.wnd.p2; return pow(2, (-1 * (x / p.wnd.p3) * (x / p.wnd.p3))); }
//inline R wf_kaiser(R i,const OpParam &p) { const R x = i*p.wnd.p1+p.wnd.p2; return i0(p.wnd.p3 * sqrt(1 - (x * x))) / i0(p.wnd.p3); }


static V WndOp(wfunc wf,OpParam &p) {
	register I i;

	if(!p.wnd.mul) {
		register BS *dd = p.rddt;
		_D_LOOP(i,p.frames) *dd = wf(i,p),dd += p.rds; _E_LOOP 
	}
	else {
		register const BS *sd = p.rsdt;
		register BS *dd = p.rddt;
		_D_LOOP(i,p.frames) *dd = *sd*wf(i,p),sd += p.rss,dd += p.rds; _E_LOOP
	}
}

#define WNDOP(WFUNC,OPP) WndOp(WFUNC,OPP)



BL VecOp::d_window(OpParam &p) 
{ 
	// reverse direction?
	BL rev = ((p.revdir?1:0)^(p.symm == 1?1:0)^(p.wnd.inv?1:0)) != 0;

	// set middle sample (if existent) to 1
	if(p.oddrem) p.SkipOddMiddle(1);

	switch(p.wnd.wndtp) {	
	case 0: { // bevel (Bartlett)
		register R inc,cur;
		inc = (rev?-1.:1.)/p.frames; // increase
		cur = rev?(1+inc/2):inc/2; // start

		if(!p.wnd.mul) {
			register BS *dd = p.rddt;
			register I i;
			if(p.rds == 1)
				_D_LOOP(i,p.frames) *(dd++) = cur,cur += inc; _E_LOOP
			else
				_D_LOOP(i,p.frames) *dd = cur,dd += p.rds,cur += inc; _E_LOOP
		}
		else {
			register const BS *sd = p.rsdt;
			register BS *dd = p.rddt;
			register I i;
			if(sd == dd)
				if(p.rss == 1 && p.rds == 1)
					_D_LOOP(i,p.frames) *(dd++) *= cur,cur += inc; _E_LOOP
				else
					_D_LOOP(i,p.frames) *dd *= cur,dd += p.rds,cur += inc; _E_LOOP
			else
				if(p.rss == 1 && p.rds == 1)
					_D_LOOP(i,p.frames) *(dd++) = *(sd++) * cur,cur += inc; _E_LOOP
				else
					_D_LOOP(i,p.frames) *dd = *sd * cur,sd += p.rss,dd += p.rds,cur += inc; _E_LOOP
		}
		break;
	}
	case 1: { // sine
		p.wnd.p1 = (M_PI/2)/p.frames;
		p.wnd.p2 = p.wnd.p1/2+(rev?M_PI/2:0);
		WNDOP(wf_sin,p);
		break;
	}
	case 2: { // Hanning
		p.wnd.p1 = M_PI/p.frames;
		p.wnd.p2 = p.wnd.p1/2+(rev?0:M_PI);
		WNDOP(wf_hanning,p);
		break;
	}	
	case 3: { // Hamming
		p.wnd.p1 = M_PI/p.frames;
		p.wnd.p2 = p.wnd.p1/2+(rev?0:M_PI);
		WNDOP(wf_hamming,p);
		break;
	}	
	case 4: { // Blackman
		p.wnd.p1 = M_PI/p.frames;
		p.wnd.p2 = p.wnd.p1/2+(rev?0:M_PI);
		WNDOP(wf_blackman,p);
		break;
	}	
	case 5: { // Connes (xxx)
		p.wnd.p1 = 1./p.frames;
		p.wnd.p2 = p.wnd.p1/2+(rev?1:0);
		WNDOP(wf_connes,p);
		break;
	}	
	case 6: { // Welch (xxx)
		p.wnd.p1 = 1./p.frames;
		p.wnd.p2 = p.wnd.p1/2+(rev?1:0);
		WNDOP(wf_welch,p);
		break;
	}	
	case 7: { // Lanczos (xxx)
		p.wnd.p1 = M_PI/p.frames;
		p.wnd.p2 = p.wnd.p1/2+(rev?0:M_PI);
		WNDOP(wf_lanczos,p);
		break;
	}	
	default: {
		post("%s: Window function #%i not known",p.opname,p.wnd.wndtp);
		return false;
	}
	}
	
	return true;
}

Vasp *VaspOp::m_window(OpParam &p,CVasp &src,const Argument &arg,CVasp *dst,BL inv,BL mul,BL symm) 
{ 
	static const int wndnum = 8;
	static const char *wndtps[wndnum] = {"lin","sin","hanning","hamming","blackman","connes","welch","lanczos" /*,"gaussian","kaiser"*/};

	Vasp *ret = NULL;
	RVecBlock *vecs = GetRVecs(p.opname,src,dst);
	if(vecs) {
		p.wnd.wndtp = -1;

		if(arg.IsList() && arg.GetList().Count() >= 1) {
			// window mode
			const flext::AtomList &l = arg.GetList();
			if(flext::IsSymbol(l[0])) {
				I i;
				const C *s = flext::GetString(l[0]);
				p.wnd.wndtp = -1;
				for(i = 0; i < wndnum; ++i)
					if(!strcmp(wndtps[i],s)) { p.wnd.wndtp = i; break; }
			}
			else if(flext::CanbeInt(l[0])) {
				p.wnd.wndtp = flext::GetAInt(l[0]);
			}
			else p.wnd.wndtp = -1;
		}
		
		if(p.wnd.wndtp < 0) {
			post("%s - invalid window type - using lin",p.opname);
			p.wnd.wndtp = 0;
		}
		
		p.wnd.inv = inv;
		p.wnd.mul = mul;			
		ret = DoOp(vecs,VecOp::d_window,p,symm);
		delete vecs;
	}

	return ret;
}

VASP_ANYOP("vasp.window vasp.wnd",window,0,false,VASP_ARG(),"Sets target vasp to window function")
VASP_ANYOP("vasp.*window vasp.*wnd",mwindow,0,true,VASP_ARG(),"Multiplies a vasp by window function")
VASP_ANYOP("vasp.!window vasp.!wnd",iwindow,0,false,VASP_ARG(),"Sets target vasp to reverse window function")
VASP_ANYOP("vasp.*!window vasp.*!wnd",miwindow,0,true,VASP_ARG(),"Multiplies a vasp by reverse window function")
VASP_ANYOP("vasp.xwindow vasp.xwnd",xwindow,0,false,VASP_ARG(),"Sets target vasp to symmetrical window function")
VASP_ANYOP("vasp.*xwindow vasp.*xwnd",mxwindow,0,true,VASP_ARG(),"Multiplies a vasp by symmetrical window function")



