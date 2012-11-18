/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPFUNS_H
#define __VASP_OPFUNS_H

#include "opdefs.h"
#include "util.h"


namespace VecOp {

    // assignment

    template<class T> class f_copy { 
    public: 
    	static I run_opt() { return 3; }
        static V run(T &v,T a) { v = a; }
    	static I cun_opt() { return 2; }
        static V cun(T &rv,T &iv,T ra,T ia) { rv = ra,iv = ia; } 
    };

    template<class T> class f_set { 
    public: 
    	static I rbin_opt() { return 3; }
        static V rbin(T &v,T,T b) { v = b; }
    	static I cbin_opt() { return 2; }
        static V cbin(T &rv,T &iv,T,T,T rb,T ib) { rv = rb,iv = ib; } 
    };

    // arithmetic

    template<class T> class f_add {
    public: 
    	static I rbin_opt() { return 3; }
        static V rbin(T &v,T a,T b) { v = a+b; }
    	static I cbin_opt() { return 2; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T ib) { rv = ra+rb,iv = ia+ib; }
    };

    template<class T> class f_sub {
    public: 
    	static I rbin_opt() { return 3; }
        static V rbin(T &v,T a,T b) { v = a-b; }
    	static I cbin_opt() { return 2; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T ib) { rv = ra-rb,iv = ia-ib; }
    };

    template<class T> class f_subr {
    public: 
    	static I rbin_opt() { return 2; }
        static V rbin(T &v,T a,T b) { v = b-a; }
    	static I cbin_opt() { return 2; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T ib) { rv = rb-ra,iv = ib-ia; }
    };

    template<class T> class f_mul {
    public: 
    	static I rbin_opt() { return 3; }
        static V rbin(T &v,T a,T b) { v = a*b; }
    	static I cbin_opt() { return 1; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T ib) { rv = ra*rb-ia*ib, iv = ra*ib+rb*ia; }
    };

    template<class T> class f_div {
    public: 
    	static I rbin_opt() { return 2; }
        static V rbin(T &v,T a,T b) { v = a/b; }

    	static I cbin_opt() { return 0; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T ib) 
        { 
	        register const T den = sqabs(rb,ib);
	        rv = (ra*rb+ia*ib)/den;
	        iv = (ia*rb-ra*ib)/den;
        }
    };

    template<class T> class f_divr {
    public: 
    	static I rbin_opt() { return 2; }
        static V rbin(T &v,T a,T b) { v = b/a; }

    	static I cbin_opt() { return 0; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T ib)
        { 
	        register const T den = sqabs(ra,ia);
	        rv = (rb*ra+ib*ia)/den;
	        iv = (ib*ra-rb*ia)/den;
        }
    };

    template<class T> class f_mod { 
    public: 
    	static I rbin_opt() { return 0; }
        static V rbin(T &v,T a,T b) { v = fmod(a,b); } 
    };

    template<class T> class f_abs {
    public: 
    	static I run_opt() { return 0; }
        static V run(T &v,T a) { v = fabs(a); }
    	static I cun_opt() { return 0; }
        static V cun(T &rv,T &iv,T ra,T ia) { rv = sqrt(ra*ra+ia*ia),iv = 0; }
    };

    template<class T> class f_sign { 
    public: 
    	static I run_opt() { return 0; }
        static V run(T &v,T a) { v = (a == 0?0:(a < 0?-1.:1.)); } 
    };

    template<class T> class f_sqr {
    public: 
    	static I run_opt() { return 3; }
        static V run(T &v,T a) { v = a*a; } 
    	static I cun_opt() { return 1; }
        static V cun(T &rv,T &iv,T ra,T ia) { rv = ra*ra-ia*ia; iv = ra*ia*2; }
    };

    template<class T> class f_ssqr { 
    public: 
    	static I run_opt() { return 0; }
        static V run(T &v,T a) { v = a*fabs(a); } 
    };


    template<class T> class f_sumq {
    public: 
    	static I rop_opt() { return 2; }
        static V rop(T &,T ra,OpParam &p) 
        { 
	        p.norm.minmax += ra; 
        } 
    };

    // transcendent

    template<class T> class f_powi {
    public: 
    	static I cop_opt() { return 0; }
        static V cop(T &rv,T &iv,T ra,T ia,OpParam &p) 
        { 
	        register const I powi = p.ibin.arg;
            register T rt,it; VecOp::f_sqr<T>::cun(rt,it,ra,ia);
            for(I i = 2; i < powi; ++i) VecOp::f_mul<T>::cbin(rt,it,rt,it,ra,ia);
	        rv = rt,iv = it;
        } 
    };

    template<class T> class f_pow {
    public: 
    	static I rbin_opt() { return 0; }
        static V rbin(T &v,T a,T b) { v = pow(fabs(a),b)*sgn(a); } 

    	static I cbin_opt() { return 0; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T) 
        { 
	        register const T _abs = sqrt(sqabs(ra,ia));
	        if(_abs) {
		        register const T _p = pow(_abs,rb)/_abs;
		        rv = _p*ra,iv = _p*ia;
	        }
	        else
		        rv = iv = 0;
        } 
    protected:
        static T sgn(T x) { return x?(x > 0?1:-1):0; }
    };

    template<class T> class f_sqrt {
    public: 
    	static I run_opt() { return 0; }
        static V run(T &v,T a) { v = sqrt(fabs(a)); } 
    };

    template<class T> class f_ssqrt {
    public:
    	static I run_opt() { return 0; }
        static V run(T &v,T a) { v = sqrt(fabs(a))*sgn(a); } 
    };


    template<class T> class f_exp {
    public: 
    	static I run_opt() { return 0; }
        static V run(T &v,T a) { v = exp(a); } 
    };

    template<class T> class f_log {
    public: 
    	static I run_opt() { return 0; }
        static V run(T &v,T a) { v = log(a); }  // \todo detect NANs
    };

    // comparisons

    template<class T> class f_lwr {
    public: 
    	static I rbin_opt() { return 1; }
        static V rbin(T &v,T a,T b) { v = a < b?1:0; }
    };

    template<class T> class f_gtr {
    public: 
    	static I rbin_opt() { return 1; }
        static V rbin(T &v,T a,T b) { v = a > b?1:0; }
    };

    template<class T> class f_alwr {
    public: 
    	static I rbin_opt() { return 0; }
        static V rbin(T &v,T a,T b) { v = fabs(a) < fabs(b)?1:0; }
    };

    template<class T> class f_agtr {
    public: 
    	static I rbin_opt() { return 0; }
        static V rbin(T &v,T a,T b) { v = fabs(a) > fabs(b)?1:0; }
    };

    template<class T> class f_leq {
    public: 
    	static I rbin_opt() { return 1; }
        static V rbin(T &v,T a,T b) { v = a <= b?1:0; }
    };

    template<class T> class f_geq {
    public: 
    	static I rbin_opt() { return 1; }
        static V rbin(T &v,T a,T b) { v = a >= b?1:0; }
    };

    template<class T> class f_aleq {
    public: 
    	static I rbin_opt() { return 0; }
        static V rbin(T &v,T a,T b) { v = fabs(a) <= fabs(b)?1:0; }
    };

    template<class T> class f_ageq {
    public: 
    	static I rbin_opt() { return 0; }
        static V rbin(T &v,T a,T b) { v = fabs(a) >= fabs(b)?1:0; }
    };

    template<class T> class f_equ {
    public: 
    	static I rbin_opt() { return 1; }
        static V rbin(T &v,T a,T b) { v = a == b?1:0; }
    };

    template<class T> class f_neq {
    public: 
    	static I rbin_opt() { return 1; }
        static V rbin(T &v,T a,T b) { v = a != b?1:0; }
    };

    // min/max

    template<class T> class f_min {
    public: 
    	static I rbin_opt() { return 1; }
        static V rbin(T &v,T a,T b) { v = a < b?a:b; }

    	static I cbin_opt() { return 0; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T ib) 
        { 
	        if(sqabs(ra,ia) < sqabs(rb,ib))	rv = ra,iv = ia; 
	        else rv = rb,iv = ib; 
        }
    };

    template<class T> class f_max {
    public: 
    	static I rbin_opt() { return 1; }
        static V rbin(T &v,T a,T b) { v = a > b?a:b; }

    	static I cbin_opt() { return 0; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T ib) 
        { 
	        if(sqabs(ra,ia) > sqabs(rb,ib))	rv = ra,iv = ia; 
	        else rv = rb,iv = ib; 
        }
    };

    template<class T> class f_minmax {
    public:
    	static I cun_opt() { return 0; }
        static V cun(T &rv,T &iv,T ra,T ia) 
        { 
	        if(ra < ia)	rv = ra,iv = ia; 
	        else rv = ia,iv = ra; 
        } 
    };

    template<class T> class f_minq {
    public: 
    	static I rop_opt() { return 0; }
        static V rop(T &,T ra,OpParam &p) 
        { 
	        if(ra < p.norm.minmax) p.norm.minmax = ra; 
        } 

    	static I cop_opt() { return 0; }
        static V cop(T &,T &,T ra,T ia,OpParam &p) 
        { 
	        register T s = sqabs(ra,ia); 
	        if(s < p.norm.minmax) p.norm.minmax = s; 
        } 
    };

    template<class T> class f_maxq {
    public: 
    	static I rop_opt() { return 0; }
        static V rop(T &,T ra,OpParam &p) 
        { 
	        if(ra > p.norm.minmax) p.norm.minmax = ra; 
        } 

    	static I cop_opt() { return 0; }
        static V cop(T &,T &,T ra,T ia,OpParam &p) 
        { 
	        register T s = sqabs(ra,ia); 
	        if(s > p.norm.minmax) p.norm.minmax = s; 
        } 
    };

    template<class T> class f_aminq {
    public: 
    	static I rop_opt() { return 0; }
        static V rop(T &,T ra,OpParam &p) 
        { 
	        register T s = fabs(ra); 
	        if(s < p.norm.minmax) p.norm.minmax = s; 
        } 
    };

    template<class T> class f_amaxq {
    public: 
    	static I rop_opt() { return 0; }
        static V rop(T &,T ra,OpParam &p) 
        { 
	        register T s = fabs(ra); 
	        if(s > p.norm.minmax) p.norm.minmax = s; 
        } 
    };


    // gating

    template<class T> class f_gate {
    public:
    	static I rbin_opt() { return 0; }
        static V rbin(T &rv,T ra,T rb) { if(fabs(ra) >= rb) rv = ra; else rv = 0; } 

    	static I cbin_opt() { return 0; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T) 
        { 
	        register const T _abs = sqabs(ra,ia);

	        if(_abs >= rb*rb) rv = ra,iv = ia;
	        else rv = iv = 0;
        } 
    };

    template<class T> class f_igate {
    public:
    	static I rbin_opt() { return 0; }
        static V rbin(T &rv,T ra,T rb) { if(fabs(ra) <= rb) rv = ra; else rv = 0; } 

    	static I cbin_opt() { return 0; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T) 
        { 
	        register const T _abs = sqabs(ra,ia);

	        if(_abs <= rb*rb) rv = ra,iv = ia;
	        else rv = iv = 0;
        } 
    };
    
    // complex

    template<class T> class f_norm {
    public:
    	static I cun_opt() { return 0; }
        static V cun(T &rv,T &iv,T ra,T ia) 
        { 
	        register T f = sqabs(ra,ia);
	        if(f) { f = 1./sqrt(f); rv = ra*f,iv = ia*f; }
	        else rv = iv = 0;
        }
    };

    template<class T> class f_conj {
    public:
    	static I cun_opt() { return 2; }
        static V cun(T &,T &iv,T,T ia) { iv = -ia; }
    };

    template<class T> class f_polar {
    public:
    	static I cun_opt() { return 0; }
        static V cun(T &rv,T &iv,T ra,T ia) { rv = sqrt(sqabs(ra,ia)),iv = arg(ra,ia); }
    };

    template<class T> class f_rect {
    public:
    	static I cun_opt() { return 0; }
        static V cun(T &rv,T &iv,T ra,T ia) { rv = ra*cos(ia),iv = ra*sin(ia); }
    };

    template<class T> class f_radd {
    public:
    	static I cbin_opt() { return 0; }
        static V cbin(T &rv,T &iv,T ra,T ia,T rb,T) 
        { 
	        register const T _abs = sqrt(sqabs(ra,ia))+rb;
	        register const T _phi = arg(ra,ia);

	        rv = _abs*cos(_phi),iv = _abs*sin(_phi);
        } 
    };

    // extra

    template<class T> class f_fix {
    public:
        /*! \brief Bashes denormals and NANs to zero

	        \param a argument list 
	        \param v destination vasp (NULL for in-place operation)
	        \return normalized destination vasp
        */
    	static I run_opt() { return 0; }
        static V run(T &v,T a) 
        { 
	        if(a != a) // NAN
		        v = 0; 
	        else {
		        // denormal bashing (doesn't propagate to the next stage)

		        static const T anti_denormal = (T)1.e-18;
		        a += anti_denormal;
		        a -= anti_denormal;
		        v = a; 
	        }
        } 
    };

}



#define DEFOP(T,FUN,OP,KIND) \
namespace VecOp { inline BL FUN(OpParam &p) { return D__##KIND(T,f_##OP < T > ,p); } }


#define DEFVEC_R(T,OP) \
    static BL r_##OP (I len,T *dr,I rds,const T *sr,I rss) { return VecOp::V__rbin<T,VecOp::f_##OP <T> >(sr,rss,dr,rds,len); } \
    static BL v_##OP##_(I layers,const T *sr,T *dr,const T *ar,I len) { return VecOp::V__vbin<T,VecOp::f_##OP <T> >(layers,sr,dr,ar,len); } \
    static BL v_##OP (I dim,const I *dims,I layers,T *dr,const T *sr,const T *ar) { return VecOp::V__vmulti<T>(v_##OP##_,layers,sr,dr,ar,dim,dims); }

#define DEFVEC_C(T,OP) \
    static BL c_##OP (I len,T *dr,T *di,I rds,I ids,const T *sr,I rss,I iss) { return VecOp::V__cbin<T,VecOp::f_##OP <T> >(sr,rss,iss,dr,rds,ids,len); }

#define DEFVEC_B(T,OP) DEFVEC_R(T,OP) DEFVEC_C(T,OP)


template<class T>
class VecFun {
public:
	DEFVEC_B(T,copy)	

	DEFVEC_B(T,add)
	DEFVEC_B(T,sub)
	DEFVEC_B(T,subr)
	DEFVEC_B(T,mul)
	DEFVEC_B(T,div)
	DEFVEC_B(T,divr)
	DEFVEC_R(T,mod)
	DEFVEC_B(T,abs)
	DEFVEC_R(T,sign)
	DEFVEC_B(T,sqr)
	DEFVEC_R(T,ssqr)

	DEFVEC_C(T,powi)
	DEFVEC_B(T,pow)
	DEFVEC_R(T,sqrt)
	DEFVEC_R(T,ssqrt)
	DEFVEC_R(T,exp)
	DEFVEC_R(T,log)

	DEFVEC_R(T,lwr)
	DEFVEC_R(T,gtr)
	DEFVEC_R(T,alwr)
	DEFVEC_R(T,agtr)
	DEFVEC_R(T,leq)
	DEFVEC_R(T,geq)
	DEFVEC_R(T,aleq)
	DEFVEC_R(T,ageq)
	DEFVEC_R(T,equ)
	DEFVEC_R(T,neq)

	DEFVEC_B(T,min)
	DEFVEC_B(T,max)
	DEFVEC_C(T,minmax)
	DEFVEC_C(T,gate)
	DEFVEC_C(T,igate)

	DEFVEC_C(T,norm)
	DEFVEC_C(T,conj)
	DEFVEC_C(T,polar)
	DEFVEC_C(T,rect)
	DEFVEC_C(T,radd)

	DEFVEC_R(T,fix)
};



#endif
