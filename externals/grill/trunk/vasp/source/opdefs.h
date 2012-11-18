/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPDEFS_H
#define __VASP_OPDEFS_H

#include "oploop.h"
#include "opbase.h"

#ifdef VASP_CHN1  
#define _D_ALWAYS1 1
#else
#define _D_ALWAYS1 0
#endif


namespace VecOp {

// multi-layer templates

template<class T,class OP,int LR>
inline BL vec_un(T *v,const T *a,I n = 0) { 
    const I _n = LR?LR:n;
    for(I i = 0; i < _n; ++i) OP::run(v[i],a[i]); 
    return true;
}

template<class T,class OP,int LR>
inline BL vec_un(T *v,T a,I n = 0) { 
    const I _n = LR?LR:n;
    for(I i = 0; i < _n; ++i) OP::run(v[i],a); 
    return true;
}

template<class T,class TR,class OP,int LR>
inline BL vec_bin(T *v,const T *a,const TR *b,I n = 0) { 
    const I _n = LR?LR:n;
    for(I i = 0; i < _n; ++i) OP::rbin(v[i],a[i],b[i]); 
    return true;
}

template<class T,class TR,class OP,int LR>
inline BL vec_bin(T *v,const T *a,TR b,I n = 0) { 
    const I _n = LR?LR:n;
    for(I i = 0; i < _n; ++i) OP::rbin(v[i],a[i],b); 
    return true;
}

/*! \brief skeleton for unary real operations
*/
template<class T,class OP> BL V__run(register const T *sr,I rss,register T *dr,I rds,I frames)
{																
    register I i;
    if(sr == dr && OP::run_opt() >= 3)												
		if((_D_ALWAYS1 || rds == 1) && OP::run_opt() >= 2)							
            _DE_LOOP(i,frames, ( OP::run(*dr,*dr), dr++ ) )
		else													
			_DE_LOOP(i,frames, ( OP::run(*dr,*dr), dr += rds ) )
	else														
		if((_D_ALWAYS1 || (rss == 1 && rds == 1)) && OP::run_opt() >= 2)			
			_DE_LOOP(i,frames, ( OP::run(*dr,*sr), sr++,dr++ ) )
		else													
			_DE_LOOP(i,frames, ( OP::run(*dr,*sr), sr += rss,dr += rds ) )
	return true;												
}


/*! \brief skeleton for unary complex operations
*/
template<class T,class OP> BL V__cun(register const T *sr,register const T *si,I rss,I iss,register T *dr,register T *di,I rds,I ids,I frames)
{																
    register I i;
	if(sr == dr && si == di && OP::cun_opt() >= 3)									
		if((_D_ALWAYS1 || (rds == 1 && ids == 1)) && OP::cun_opt() >= 2)			
            _DE_LOOP(i,frames, ( OP::cun(*dr,*di,*dr,*di), dr++,di++ ) )
		else													
			_DE_LOOP(i,frames, ( OP::cun(*dr,*di,*dr,*di), dr += rds,di += ids ) )
	else														
		if((_D_ALWAYS1 || (rss == 1 && iss == 1 && rds == 1 && ids == 1)) && OP::cun_opt() >= 2) 
			_DE_LOOP(i,frames, ( OP::cun(*dr,*di,*sr,*si), sr++,si++,dr++,di++ ) )
		else													
			_DE_LOOP(i,frames, ( OP::cun(*dr,*di,*sr,*si), sr += rss,si += iss,dr += rds,di += ids ) )
	return true;												
}

template<class T,class OP> BL V__vun(I layers,register const T *sr,register T *dr,I frames)
{																
    register I i;
    switch(layers) {
    case 1: 
            V__run<T,OP>(sr,1,dr,1,frames); 
            break;
    case 2: 
			_DF_LOOP(i,frames, ( vec_un<T,OP,2>(dr,sr,2), sr += 2, dr += 2) )
            break;
    case 3: 
			_DF_LOOP(i,frames, ( vec_un<T,OP,3>(dr,sr,3), sr += 3, dr += 3) )
            break;
    case 4: 
			_DF_LOOP(i,frames, ( vec_un<T,OP,4>(dr,sr,4), sr += 4, dr += 4) )
            break;
    default:
			_DF_LOOP(i,frames, ( vec_un<T,OP,0>(dr,sr,layers), sr += layers, dr += layers) )
            break;
    }
    return true;
}


template<class T,class OP> BL V__rbin(register const T *sr,I rss,register T *dr,I rds,register const T *ar,I ras,I frames)
{																
	register I i;												
	if(sr == dr && OP::rbin_opt() >= 3)								
		if((_D_ALWAYS1 || (rds == 1 && ras == 1)) && OP::rbin_opt() >= 2)				
            _DE_LOOP(i,frames, ( OP::rbin(*dr,*dr,*ar), dr++,ar++ ) )
		else												
			_DE_LOOP(i,frames, ( OP::rbin(*dr,*dr,*ar), dr += rds,ar += ras ) )
	else													
		if((_D_ALWAYS1 || (rss == 1 && rds == 1 && ras == 1)) && OP::rbin_opt() >= 2)	
			_DE_LOOP(i,frames, ( OP::rbin(*dr,*sr,*ar), sr++,dr++,ar++ ) )
		else												
			_DE_LOOP(i,frames, ( OP::rbin(*dr,*sr,*ar), sr += rss,dr += rds,ar += ras ) )
    return true;
}

template<class T,class OP> BL V__cbin(register const T *sr,register const T *si,I rss,I iss,register T *dr,register T *di,I ids,I rds,const T *ar,const T *ai,I ras,I ias,I frames)
{																
	register I i;												
	if(sr == dr && si == di && OP::cbin_opt() >= 3)							
		if((_D_ALWAYS1 || (rds == 1 && ids == 1 && ras == 1 && ias == 1)) && OP::cbin_opt() >= 2) 
            _DE_LOOP(i,frames, ( OP::cbin(*dr,*di,*dr,*di,*ar,*ai), dr++,di++,ar++,ai++ ) )
		else											
			_DE_LOOP(i,frames, ( OP::cbin(*dr,*di,*dr,*di,*ar,*ai), dr += rds,di += ids,ar += ras,ai += ias ) )
	else												
		_DE_LOOP(i,frames, ( OP::cbin(*dr,*di,*sr,*si,*ar,*ai), sr += rss,si += iss,dr += rds,di += ids,ar += ras,ai += ias ) )
    return true;
}


template<class TR> class _A__vector {
public:
	static BL unroll() { return true; }
    static TR ev(const TR *a,I i,I m) { return a[i*m]; }
};

template<class TR> class _A__scalar {
public:
	static BL unroll() { return true; }
    static TR ev(TR a,I i,I m) { return a; }
};

class _A__env {
public:
	static BL unroll() { return false; }
    static R ev(Env::Iter &a,I i,I m) { return a.ValFwd(i); }
};

template<class T,class TA,class TR,class OP,class EVARG> BL Vx__rbin(register const T *sr,I rss,register T *dr,I rds,TA ar,I frames)
{																
	register I i;												
	if(sr == dr && OP::rbin_opt() >= 3)
		if((_D_ALWAYS1 || rds == 1) && OP::rbin_opt() >= 2)				
            _DQ_LOOP(EVARG::unroll(),i,frames, ( OP::rbin(*dr,*dr,EVARG::ev(ar,i,1)), dr++ ) )
		else												
            _DQ_LOOP(EVARG::unroll(),i,frames, ( OP::rbin(*dr,*dr,EVARG::ev(ar,i,1)), dr += rds ) )
	else													
		if((_D_ALWAYS1 || rss == 1 && rds == 1) && OP::rbin_opt() >= 2)	
            _DQ_LOOP(EVARG::unroll(),i,frames, ( OP::rbin(*dr,*sr,EVARG::ev(ar,i,1)), sr++,dr++ ) )
		else												
            _DQ_LOOP(EVARG::unroll(),i,frames, ( OP::rbin(*dr,*sr,EVARG::ev(ar,i,1)), sr += rss,dr += rds ) )
    return true;
}

template<class T,class TA1,class TA2,class TR,class OP,class EVARG1,class EVARG2> BL Vx__cbin(register const T *sr,register const T *si,I rss,I iss,register T *dr,register T *di,I ids,I rds,TA1 ar,TA2 ai,I ras,I ias,I frames)
{																
	register I i;												
	if(sr == dr && si == di && OP::cbin_opt() >= 3)							
		if((_D_ALWAYS1 || (rds == 1 && ids == 1 && ras == 1 && ias == 1)) && OP::cbin_opt() >= 2) 
            _DQ_LOOP(EVARG1::unroll() && EVARG2::unroll(),i,frames, ( OP::cbin(*dr,*di,*dr,*di,EVARG1::ev(ar,i,1),EVARG2::ev(ai,i,1)), dr++,di++ ) )
		else											
            _DQ_LOOP(EVARG1::unroll() && EVARG2::unroll(),i,frames, ( OP::cbin(*dr,*di,*dr,*di,EVARG1::ev(ar,i,ras),EVARG2::ev(ai,i,ias)), dr += rds,di += ids ) )
	else												
        _DQ_LOOP(EVARG1::unroll() && EVARG2::unroll(),i,frames, ( OP::cbin(*dr,*di,*sr,*si,EVARG1::ev(ar,i,ras),EVARG2::ev(ai,i,ias)), sr += rss,si += iss,dr += rds,di += ids ) )
    return true;
}

template<class T,class TA,class TR,class OP,class EVARG> BL Vx__vbin(I layers,register const T *sr,register T *dr,TA ar,I frames)
{																
    register I i;
    switch(layers) {
    case 1: 
            Vx__rbin<T,TA,TR,OP,EVARG>(sr,1,dr,1,ar,frames); 
            break;
    case 2: 
			_DF_LOOP(i,frames, ( vec_bin<T,TR,OP,2>(dr,sr,EVARG::ev(ar,i,2),2), sr += 2, dr += 2) )
            break;
    case 3: 
			_DF_LOOP(i,frames, ( vec_bin<T,TR,OP,3>(dr,sr,EVARG::ev(ar,i,3),3), sr += 3, dr += 3) )
            break;
    case 4: 
			_DF_LOOP(i,frames, ( vec_bin<T,TR,OP,4>(dr,sr,EVARG::ev(ar,i,4),4), sr += 4, dr += 4) )
            break;
    default:
			_DF_LOOP(i,frames, ( vec_bin<T,TR,OP,0>(dr,sr,EVARG::ev(ar,i,layers),layers), sr += layers, dr += layers) )
            break;
    }
    return true;
}

template<class T,class OP> inline BL V__vbin(I layers,register const T *sr,register T *dr,register const T *ar,I frames)
{
    return Vx__vbin<T,const T *,T,OP,_A__vector<T> >(layers,sr,dr,ar,frames);
}

/*! \brief skeleton for binary real operations
*/
template<class T,class OP> BL _F__rbin(OpParam &p)					
{																
	if(p.HasArg() && p.arg[0].Is()) {							
		switch(p.arg[0].argtp) {								
		case OpParam::Arg::arg_v: {	
            V__rbin<T,OP>(p.rsdt,p.rss,p.rddt,p.rds,p.arg[0].v.rdt,p.arg[0].v.rs,p.frames);
			break;												
		}														
		case OpParam::Arg::arg_env: {							
			Env::Iter it(*p.arg[0].e.env); it.Init(0);
            Vx__rbin<T,Env::Iter &,R,OP,_A__env >(p.rsdt,p.rss,p.rddt,p.rds,it,p.frames);
			break;												
		}														
		case OpParam::Arg::arg_x: {							
            Vx__rbin<T,R,T,OP,_A__scalar<R> >(p.rsdt,p.rss,p.rddt,p.rds,p.arg[0].x.r,p.frames);
			break;												
		}
        case OpParam::Arg::arg_: assert(false);
		}														
	}															
	else {														
        Vx__rbin<T,T,T,OP,_A__scalar<T> >(p.rsdt,p.rss,p.rddt,p.rds,p.rbin.arg,p.frames);
	}															
	return true;												
}


/*! \brief skeleton for binary complex operations
*/
template<class T,class OP> BL _F__cbin(OpParam &p)							
{																
	if(p.HasArg() && p.arg[0].Is()) {							
		switch(p.arg[0].argtp) {									
		case OpParam::Arg::arg_v: {									
			if(p.arg[0].v.idt)
                V__cbin<T,OP>(p.rsdt,p.isdt,p.rss,p.iss,p.rddt,p.iddt,p.rds,p.ids,p.arg[0].v.rdt,p.arg[0].v.idt,p.arg[0].v.rs,p.arg[0].v.is,p.frames);
            else
                Vx__cbin<T,const T *,T,T,OP,_A__vector<T>,_A__scalar<T> >(p.rsdt,p.isdt,p.rss,p.iss,p.rddt,p.iddt,p.rds,p.ids,p.arg[0].v.rdt,0,p.arg[0].v.rs,1,p.frames);
			break;												
		}														
		case OpParam::Arg::arg_env: {							
			Env::Iter it(*p.arg[0].e.env); it.Init(0);			
            Vx__cbin<T,Env::Iter &,T,R,OP,_A__env,_A__scalar<T> >(p.rsdt,p.isdt,p.rss,p.iss,p.rddt,p.iddt,p.rds,p.ids,it,0,1,1,p.frames);
			break;												
		}														
		case OpParam::Arg::arg_x: {								
            Vx__cbin<T,R,R,R,OP,_A__scalar<R>,_A__scalar<R> >(p.rsdt,p.isdt,p.rss,p.iss,p.rddt,p.iddt,p.rds,p.ids,p.arg[0].x.r,p.arg[0].x.i,1,1,p.frames);
			break;												
		}
        case OpParam::Arg::arg_: assert(false);
		}														
	}															
	else {														
        Vx__cbin<T,T,T,T,OP,_A__scalar<T>,_A__scalar<T> >(p.rsdt,p.isdt,p.rss,p.iss,p.rddt,p.iddt,p.rds,p.ids,p.cbin.rarg,p.cbin.iarg,1,1,p.frames);
	}															
	return true;												
}

/*! \brief skeleton for real operations with parameter block
*/
template<class T,class ARG,class OP> BL V__rop(ARG p,register const BS *sr,I rss,register BS *dr,I rds,I frames)
{																
	register I i;												
	if(sr == dr && OP::rop_opt() >= 3)												
		if((_D_ALWAYS1 || rds == 1) && OP::rop_opt() >= 2)											
            _DE_LOOP(i,frames, ( OP::rop(*dr,*dr,p), dr++ ) )
		else													
			_DE_LOOP(i,frames, ( OP::rop(*dr,*dr,p), dr += rds ) )
	else														
		if((_D_ALWAYS1 || (rss == 1 && p.rds == 1)) && OP::rop_opt() >= 2)			
			_DE_LOOP(i,frames, ( OP::rop(*dr,*sr,p), sr++,dr++ ) )
		else													
			_DE_LOOP(i,frames, ( OP::rop(*dr,*sr,p), sr += rss,dr += rds ) )
	return true;												
}

/*! \brief skeleton for complex operations with parameter block
*/
template<class T,class ARG,class OP> BL V__cop(ARG p,register const BS *sr,register const BS *si,I rss,I iss,register BS *dr,register BS *di,I rds,I ids,I frames)
{																
	register I i;												
	if(sr == dr && si == di && OP::cop_opt() >= 3)									
		if((_D_ALWAYS1 || (rds == 1 && ids == 1)) && OP::cop_opt() >= 2)			
			_DE_LOOP(i,frames, ( OP::cop(*dr,*di,*dr,*di,p), dr++,di++ ) )
		else													
			_DE_LOOP(i,frames, ( OP::cop(*dr,*di,*dr,*di,p), dr += rds,di += ids ) )
	else														
		if((_D_ALWAYS1 || (p.rss == 1 && p.iss == 1 && p.rds == 1 && p.ids == 1)) && OP::cop_opt() >= 2) 
			_DE_LOOP(i,frames, ( OP::cop(*dr,*di,*sr,*si,p), sr++,si++,dr++,di++ ) )
		else													
			_DE_LOOP(i,frames, ( OP::cop(*dr,*di,*sr,*si,p), sr += rss,si += iss,dr += rds,di += ids ) )
	return true;												
}


template<class T> BL _d__run(V fun(T &v,T a),OpParam &p) 
{ 
	int i;
	if(p.rds == 1 && p.rss == 1)
		_DE_LOOP(i,p.frames, ( fun(p.rddt[i],p.rsdt[i]) ) ) 
	else
		_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.rsdt[p.rss*i]) ) ) 
	return true;
}

template<class T> BL _d__cun(V fun(T &rv,T &iv,T ra,T ia),OpParam &p)
{ 
	int i;
	if(p.rds == 1 && p.ids == 1 && p.rss == 1 && p.iss == 1)
		_DE_LOOP(i,p.frames, ( fun(p.rddt[i],p.iddt[i],p.rsdt[i],p.isdt[i]) ) ) 
	else
		_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.iddt[p.ids*i],p.rsdt[p.rss*i],p.isdt[p.iss*i]) ) ) 
	return true;
}

template<class T> BL _d__rbin(V fun(T &v,T a,T b),OpParam &p)
{ 
	int i;
	if(p.HasArg() && p.arg[0].Is()) {							
		switch(p.arg[0].argtp) {								
		case OpParam::Arg::arg_v: {	
			const T *adr = p.arg[0].v.rdt;
			const I asr = p.arg[0].v.rs;
			_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.rsdt[p.rss*i],adr[asr*i]) ) ) 
			break;												
		}														
		case OpParam::Arg::arg_env: {							
			Env::Iter it(*p.arg[0].e.env); it.Init(0);
			_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.rsdt[p.rss*i],it.ValFwd(i)) ) ) 
			break;												
		}														
		case OpParam::Arg::arg_x: {
			const T av = p.arg[0].x.r;
			_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.rsdt[p.rss*i],av) ) ) 
			break;												
		}
        case OpParam::Arg::arg_: assert(false);
        }														
	}															
	else {														
		_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.rsdt[p.rss*i],p.rbin.arg) ) ) 
	}															
	return true;
}

template<class T> BL _d__cbin(V fun(T &rv,T &iv,T ra,T ia,T rb,T ib),OpParam &p)
{ 
	int i;
	if(p.HasArg() && p.arg[0].Is()) {							
		switch(p.arg[0].argtp) {									
		case OpParam::Arg::arg_v: {									
			const T *adr = p.arg[0].v.rdt,*adi = p.arg[0].v.idt;
			const I asr = p.arg[0].v.rs,asi = p.arg[0].v.is;
			if(adi)
				_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.iddt[p.ids*i],p.rsdt[p.rss*i],p.isdt[p.iss*i],adr[asr*i],adi[asi*i]) ) ) 
            else
				_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.iddt[p.ids*i],p.rsdt[p.rss*i],p.isdt[p.iss*i],adr[asr*i],0) ) ) 
			break;												
		}														
		case OpParam::Arg::arg_env: {							
			Env::Iter it(*p.arg[0].e.env); it.Init(0);			
			_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.iddt[p.ids*i],p.rsdt[p.rss*i],p.isdt[p.iss*i],it.ValFwd(i),0) ) ) 
			break;												
		}														
		case OpParam::Arg::arg_x: {								
			const T avr = p.arg[0].x.r,avi = p.arg[0].x.i;
			_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.iddt[p.ids*i],p.rsdt[p.rss*i],p.isdt[p.iss*i],avr,avi) ) ) 
			break;
		}
        case OpParam::Arg::arg_: assert(false);
		}														
	}															
	else {														
		_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.iddt[p.ids*i],p.rsdt[p.rss*i],p.isdt[p.iss*i],p.cbin.rarg,p.cbin.iarg) ) ) 
	}															
	return true;
}

template<class T> BL _d__rop(V fun(T &v,T a,OpParam &p),OpParam &p)
{ 
	int i;
	if(p.rds == 1 && p.rss == 1)
		_DE_LOOP(i,p.frames, ( fun(p.rddt[i],p.rsdt[i],p) ) ) 
	else
		_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.rsdt[p.rss*i],p) ) ) 
	return true;
}

template<class T> BL _d__cop(V fun(T &rv,T &iv,T ra,T ia,OpParam &p),OpParam &p)
{ 
	int i;
	if(p.rds == 1 && p.ids == 1 && p.rss == 1 && p.iss == 1)
		_DE_LOOP(i,p.frames, ( fun(p.rddt[i],p.iddt[i],p.rsdt[i],p.isdt[i],p) ) ) 
	else
		_DF_LOOP(i,p.frames, ( fun(p.rddt[p.rds*i],p.iddt[p.ids*i],p.rsdt[p.rss*i],p.isdt[p.iss*i],p) ) ) 
	return true;
}


/*
template<class T,class CL> inline BL _D__run(OpParam &p) { return V__run<T,CL>(p.rsdt,p.rss,p.rddt,p.rds,p.frames); }
template<class T,class CL> inline BL _D__cun(OpParam &p) { return V__cun<T,CL>(p.rsdt,p.isdt,p.rss,p.iss,p.rddt,p.iddt,p.rds,p.ids,p.frames); }
template<class T,class CL> inline BL _D__rbin(OpParam &p) { return _F__rbin<T,CL>(p); }
template<class T,class CL> inline BL _D__cbin(OpParam &p) { return _F__cbin<T,CL>(p); }			
template<class T,class CL> inline BL _D__rop(OpParam &p) { return V__rop<T,OpParam &,CL>(p,p.rsdt,p.rss,p.rddt,p.rds,p.frames); }
template<class T,class CL> inline BL _D__cop(OpParam &p) { return V__cop<T,OpParam &,CL>(p,p.rsdt,p.isdt,p.rss,p.iss,p.rddt,p.iddt,p.rds,p.ids,p.frames); }

#ifdef VASP_COMPACT
    template<class T,class CL> BL D__run(OpParam &p) { return _d__run<T>(CL::run,p); }
    template<class T,class CL> BL D__cun(OpParam &p) { return _d__cun<T>(CL::cun,p); }
    template<class T,class CL> BL D__rbin(OpParam &p) { return _d__rbin<T>(CL::rbin,p); }
    template<class T,class CL> BL D__cbin(OpParam &p) { return _d__cbin<T>(CL::cbin,p); }
	template<class T,class CL> BL D__rop(OpParam &p) { return _d__rop<T>(CL::rop,p); }
	template<class T,class CL> BL D__cop(OpParam &p) { return _d__cop<T>(CL::cop,p); }
#else
	template<class T,class CL> BL D__run(OpParam &p) { return CL::run_opt()?_D__run<T,CL>(p):_d__run<T>(CL::run,p); }
	template<class T,class CL> BL D__cun(OpParam &p) { return CL::cun_opt()?_D__cun<T,CL>(p):_d__cun<T>(CL::cun,p); }
	template<class T,class CL> BL D__rbin(OpParam &p) { return CL::rbin_opt()?_D__rbin<T,CL>(p):_d__rbin<T>(CL::rbin,p); }
	template<class T,class CL> BL D__cbin(OpParam &p) { return CL::cbin_opt()?_D__cbin<T,CL>(p):_d__cbin<T>(CL::cbin,p); }
	template<class T,class CL> BL D__rop(OpParam &p) { return CL::rop_opt()?_D__rop<T,CL>(p):_d__rop<T>(CL::rop,p); }
	template<class T,class CL> BL D__cop(OpParam &p) { return CL::cop_opt()?_D__cop<T,CL>(p):_d__cop<T>(CL::cop,p); }
#endif
*/

// MSVC 6 can't handle optimization here!! (silently produces wrong code!!!)

#define _D__run(T,CL,p) V__run< T,CL >(p.rsdt,p.rss,p.rddt,p.rds,p.frames)
#define _D__cun(T,CL,p) V__cun< T,CL >(p.rsdt,p.isdt,p.rss,p.iss,p.rddt,p.iddt,p.rds,p.ids,p.frames)
#define _D__rbin(T,CL,p) _F__rbin< T,CL >(p)
#define _D__cbin(T,CL,p) _F__cbin< T,CL >(p)
#define _D__rop(T,CL,p) V__rop< T,OpParam &,CL >(p,p.rsdt,p.rss,p.rddt,p.rds,p.frames)
#define _D__cop(T,CL,p) V__cop< T,OpParam &,CL >(p,p.rsdt,p.isdt,p.rss,p.iss,p.rddt,p.iddt,p.rds,p.ids,p.frames)

#if defined(VASP_COMPACT) || (defined(_MSC_VER) && _MSC_VER < 1300)
    #define D__run(T,CL,p) _d__run< T >(CL::run,p)
    #define D__cun(T,CL,p) _d__cun< T >(CL::cun,p)
    #define D__rbin(T,CL,p) _d__rbin< T >(CL::rbin,p)
    #define D__cbin(T,CL,p) _d__cbin< T >(CL::cbin,p)
    #define D__rop(T,CL,p) _d__rop< T >(CL::rop,p)
    #define D__cop(T,CL,p) _d__cop< T >(CL::cop,p)
#else
	#define D__run(T,CL,p) ( CL::run_opt()?_D__run(T,CL,p):_d__run<T>(CL::run,p) ) 
	#define D__cun(T,CL,p) ( CL::cun_opt()?_D__cun(T,CL,p):_d__cun<T>(CL::cun,p) )
	#define D__rbin(T,CL,p) ( CL::rbin_opt()?_D__rbin(T,CL,p):_d__rbin<T>(CL::rbin,p) )
	#define D__cbin(T,CL,p) ( CL::cbin_opt()?_D__cbin(T,CL,p):_d__cbin<T>(CL::cbin,p) )
	#define D__rop(T,CL,p) ( CL::rop_opt()?_D__rop(T,CL,p):_d__rop<T>(CL::rop,p) )
	#define D__cop(T,CL,p) ( CL::cop_opt()?_D__cop(T,CL,p):_d__cop<T>(CL::cop,p) )
#endif


// process multi-dimensional data

template<class T> inline BL V__vmulti(BL vbin(I layers,const T *sr,T *dr,const T *ar,I len),I layers,const T *sr,T *dr,const T *ar,I dim,const I *dims)
{
	if(dim == 1 || !dims) {
		return vbin(layers,sr,dr,ar,dims?dims[0]:dim);
	}
	else if(dim > 1) {
		// calculate stride for next dimensions
		I i,s,str = layers*dims[0];
		for(i = 1; i < dim-1; ++i) str *= dims[i];
		const I dimn = dims[i];
		
		for(s = i = 0; i < dimn; ++i,s += str)
			V__vmulti(vbin,layers,sr+s,dr+s,ar+s,dim-1,dims);
		return true;
	}
	else
		return false;
}



} // namespace VecOp

#endif
