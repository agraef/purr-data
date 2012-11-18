/*
xsample - extended sample objects for Max/MSP and pd (pure data)

Copyright (c) 2001-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 39 $
$LastChangedDate$
$LastChangedBy$
*/

#ifndef __XSAMPLE_H
#define __XSAMPLE_H

#include "prefix.h"

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5.0
#endif

#define XSAMPLE_VERSION "0.3.2pre"

extern "C++" {

// most compilers are somehow broken - in other words - can't handle all C++ features

#if defined(_MSC_VER)
// MS VC 6.0 can't handle <int,int> templates?! -> no optimization
// MS VC .NET 2002 just dies with template optimization switched on
	#if _MSC_VER >= 1310
		#define TMPLOPT
	#endif
#elif defined(__BORLANDC__) 
// handles all optimizations
	#define TMPLOPT 
#elif defined(__GNUC__)
// GNUC 2.95.2 dies at compile with <int,int> templates
#if __GNUC__ >= 3
	#define TMPLOPT  // only workable with gcc >= 3.0
#endif
#elif defined(__MWERKS__)
// CodeWarrior <= 8 can't take address of a template member function 
	#ifndef FLEXT_DEBUG
	#define TMPLOPT
	#endif
//	#define SIGSTATIC  // define that for CW6
#elif defined(__MRC__)
// Apple MPW - MrCpp
//	#define TMPLOPT  // template optimation for more speed
#else
// another compiler
//	#define TMPLOPT  // template optimation for more speed (about 10%)
	//#define SIGSTATIC  // another redirection to avoid addresses of class member functions
#endif


#if defined(__MWERKS__) && !defined(__MACH__)
	#define STD std
#else
	#define STD
#endif


#ifdef __ALTIVEC__
#if FLEXT_CPU == FLEXT_CPU_PPC && defined(__MWERKS__)
	#pragma altivec_model on
	#include <vBasicOps.h>
	#include <vectorOps.h>
#elif FLEXT_CPU == FLEXT_CPU_PPC && defined(__GNUG__)
	#include <vecLib/vBasicOps.h>
	#include <vecLib/vectorOps.h>
#endif
    // this is for the UInt32 prototype (thanks to Jamie)
    // \TODO we'd rather not use Carbon but some other framework
    #ifdef __MACH__
    #include <Carbon/Carbon.h>
    #endif

	// Initialize a prefetch constant for use with vec_dst(), vec_dstt(), vec_dstst or vec_dststt
	// Taken from the "AltiVec tutorial" by Ian Ollmann, Ph.D. 
	inline UInt32 GetPrefetchConstant( int blockSizeInVectors,int blockCount,int blockStride )
	{
//		FLEXT_ASSERT( blockSizeInVectors > 0 && blockSizeInVectors <= 32 );
//		FLEXT_ASSERT( blockCount > 0 && blockCount <= 256 );
//		FLEXT_ASSERT( blockStride > MIN_SHRT && blockStride <= MAX_SHRT );
		return ((blockSizeInVectors << 24) & 0x1F000000) |
			((blockCount << 16) && 0x00FF0000) |
			(blockStride & 0xFFFF);
	}
#endif

#if 0 // FLEXT_CPU == FLEXT_CPU_PPC && defined(__GNUC__)
#include <ppc_intrinsics.h>
template<typename I,typename F> 
inline I CASTINT( F f )
{
    I i;
    __stfiwx(__fctiwz(f),0,&i);
    return i;
}
#elif FLEXT_CPU == FLEXT_CPU_INTEL && defined(_MSC_VER)
template<typename I,typename F>
inline I CASTINT(F x) {
//  by Laurent de Soras (http://ldesoras.free.fr)
//    assert (x > static_cast <double> (INT_MIN / 2) + 1.0);
//    assert (x < static_cast <double> (INT_MAX / 2) + 1.0);
    const float round_towards_m_i = -0.5f;
    I i;
    __asm {
        fld x
        fadd st,st
        fabs
        fadd round_towards_m_i
        fistp i
        sar i, 1
    }
    if(x < 0) i = -i;
    return i;
}
#else
template<typename I,typename F> inline I CASTINT(F o) { return static_cast<I>(o); }
#endif

typedef flext::buffer::Element Element;

class xsample:
	public flext_dsp
{
	FLEXT_HEADER_S(xsample,flext_dsp,setup)
	
public:
	xsample();
	~xsample();
    
    enum xs_change {
        xsc__ = 0,
        xsc_units = 0x0001,
        xsc_play = 0x0002,
        xsc_pos = 0x0008,
        xsc_range = 0x0010,
        xsc_transport = 0x0020,
        xsc_fade = 0x0040,
        
        xsc_intp = xsc_play,
        xsc_srate = xsc_play|xsc_units,
        xsc_chns = xsc_play,
        xsc_loop = xsc_play,
        xsc_startstop = xsc_play|xsc_transport,
        xsc_buffer = xsc_units|xsc_pos|xsc_range|xsc_play,
        xsc_reset = xsc_buffer,
        xsc_all = 0xffff
    };
	
	enum xs_unit { 
        xsu_sample = 0,xsu_buffer,xsu_ms,xsu_s 
    };
	
	enum xs_intp {
		xsi_none = 0,xsi_4p,xsi_lin
	};
	
	enum xs_sclmd {
		xss_unitsinbuf = 0,xss_unitsinloop,xss_buffer,xss_loop
	};
	
protected:
    virtual bool Finalize();

	buffer buf;

	void m_reset() 
    { 
        ChkBuffer(true);
        DoReset(); 
        Refresh(); 
    }

  	void m_set(int argc,const t_atom *argv);

    void m_refresh() 
    { 
        Update(xsc_buffer,true);
    }

    void m_units(xs_unit mode)
    {
        unitmode = mode;
        Update(xsc_units,true);
    }

    void m_sclmode(xs_sclmd mode)
    {
        sclmode = mode;
        Update(xsc_units,true);
    }

    void m_all() 
    { 
        ChkBuffer(true); 
        ResetRange(); 
        Refresh(); 
    }

    void m_wrap(bool w)
    {
        wrap = w;
        Update(xsc_pos|xsc_range,true);
    }

	void m_min(float mn);
	void m_max(float mx);

	xs_unit unitmode;
	xs_sclmd sclmode;

	long curmin,curmax; //,curlen;  // in samples
	long sclmin; // in samples
	float sclmul;
	float s2u;  // sample to unit conversion factor
    bool wrap;

	inline float scale(float smp) const { return (smp-sclmin)*sclmul; }
	
    static void arrscale(int n,const t_sample *in,t_sample *out,t_sample add,t_sample mul) { flext::ScaleSamples(out,in,mul,add,n); }
	inline void arrscale(int n,const t_sample *in,t_sample *out) const { arrscale(n,in,out,-sclmin*sclmul,sclmul); }
	
	static void arrmul(int n,const t_sample *in,t_sample *out,t_sample mul) { flext::MulSamples(out,in,mul,n); }
	inline void arrmul(int n,const t_sample *in,t_sample *out) const { arrmul(n,in,out,(t_sample)(1.f/s2u)); }

	void mg_buffer(AtomList &l) { if(buf.Symbol()) { l(1); SetSymbol(l[0],buf.Symbol()); } }
	inline void ms_buffer(const AtomList &l) { m_set(l.Count(),l.Atoms()); }

	inline void mg_min(float &v) const { v = curmin*s2u; }
	inline void mg_max(float &v) const { v = curmax*s2u; }
    
    void Refresh() { if(update && !Initing()) { DoUpdate(update); update = xsc__; } }
    void Update(unsigned int f,bool refr = false) { update |= f; if(refr) Refresh(); }

    //! return 0...invalid, 1...changed, -1...unchanged
	int ChkBuffer(bool refr = false);
    
    typedef flext::buffer::lock_t lock_t;

    //! Lock buffer (buffer must be checked ok)
    lock_t Lock() { return buf.Lock(); }
    //! Unlock buffer (buffer must be checked ok)
    void Unlock(lock_t l) { buf.Unlock(l); }

    void ResetRange() 
    { 
	    curmin = 0; 
        curmax = buf.Frames();
        Update(xsc_range);
    }

    virtual void DoReset();
    virtual void DoUpdate(unsigned int flags);

	virtual void CbLoadbang();
	virtual bool CbDsp();

	virtual void m_help() = 0;
	virtual void m_print() = 0;

private:

    unsigned int update;

	static void setup(t_classid c);

	FLEXT_CALLBACK(m_help)
	FLEXT_CALLBACK_V(m_set)
	FLEXT_CALLBACK(m_print)
	FLEXT_CALLBACK(m_refresh)
	FLEXT_CALLBACK(m_reset)

	FLEXT_CALLVAR_V(mg_buffer,ms_buffer)

	FLEXT_CALLSET_E(m_units,xs_unit)
	FLEXT_ATTRGET_E(unitmode,xs_unit)
	FLEXT_CALLSET_E(m_sclmode,xs_sclmd)
	FLEXT_ATTRGET_E(sclmode,xs_sclmd)

	FLEXT_ATTRGET_F(s2u)

	FLEXT_CALLSET_B(m_wrap)
	FLEXT_ATTRGET_B(wrap)

protected:
	FLEXT_CALLGET_F(mg_min)
	FLEXT_CALLGET_F(mg_max)
};


// defines which are used in the derived classes
#ifdef SIGSTATIC
	#ifdef TMPLOPT
		#define TMPLFUN(FUN,BCHNS,IOCHNS) &thisType::st_##FUN<BCHNS,IOCHNS>
		#define TMPLSTF(FUN,BCHNS,IOCHNS) &thisType::FUN<BCHNS,IOCHNS>
		#define SIGFUN(FUN) &thisType::st_##FUN
		#define TMPLDEF template <int _BCHNS_,int _IOCHNS_>
		#define TMPLCALL <_BCHNS_,_IOCHNS_>
	#else
		#define TMPLFUN(FUN,BCHNS,IOCHNS) &thisType::st_##FUN
		#define TMPLSTF(FUN,BCHNS,IOCHNS) &thisType::FUN
		#define SIGFUN(FUN) &thisType::st_##FUN
		#define TMPLDEF 
		#define TMPLCALL
	#endif

	#define DEFSIGFUN(NAME) \
	static void st_##NAME(thisType *obj,int n,t_sample *const *in,t_sample *const *out)  { obj->NAME (n,in,out); } \
	void NAME(int n,t_sample *const *in,t_sample *const *out)

	#define TMPLSIGFUN(NAME) \
	TMPLDEF static void st_##NAME(thisType *obj,int n,t_sample *const *in,t_sample *const *out)  { obj->NAME TMPLCALL (n,in,out); } \
	TMPLDEF void NAME(int n,t_sample *const *in,t_sample *const *out)

	#define TMPLSTFUN(NAME) TMPLDEF static void NAME(const Element *bdt,const int smin,const int smax,const int n,const int inchns,const int outchns,t_sample *const *invecs,t_sample *const *outvecs)

	#define SETSIGFUN(VAR,FUN) v_##VAR = FUN

	#define SETSTFUN(VAR,FUN) VAR = FUN

	#define DEFSIGCALL(NAME) \
	inline void NAME(int n,t_sample *const *in,t_sample *const *out) { (*v_##NAME)(this,n,in,out); } \
	void (*v_##NAME)(thisType *obj,int n,t_sample *const *in,t_sample *const *out) 

	#define DEFSTCALL(NAME) \
	void (*NAME)(const Element *bdt,const int smin,const int smax,const int n,const int inchns,const int outchns,t_sample *const *invecs,t_sample *const *outvecs)

#else
	#ifdef TMPLOPT
		#define TMPLFUN(FUN,BCHNS,IOCHNS) &thisType::FUN<BCHNS,IOCHNS>
		#define SIGFUN(FUN) &thisType::FUN
		#define TMPLDEF template <int _BCHNS_,int _IOCHNS_>
		#define TMPLCALL <_BCHNS_,_IOCHNS_>
	#else
		#define TMPLFUN(FUN,BCHNS,IOCHNS) &thisType::FUN
		#define SIGFUN(FUN) &thisType::FUN
		#define TMPLDEF 
		#define TMPLCALL
	#endif
	
	#define TMPLSTF(FUN,BCHNS,IOCHNS) TMPLFUN(FUN,BCHNS,IOCHNS) 

	#define DEFSIGFUN(NAME)	void NAME(int n,t_sample *const *in,t_sample *const *out)
	#define TMPLSIGFUN(NAME) TMPLDEF void NAME(int n,t_sample *const *in,t_sample *const *out)
	#define TMPLSTFUN(NAME) TMPLDEF static void NAME(const Element *bdt,const int smin,const int smax,const int n,const int inchns,const int outchns,t_sample *const *invecs,t_sample *const *outvecs,bool looped)

	#define SETSIGFUN(VAR,FUN) v_##VAR = FUN

	#define DEFSIGCALL(NAME) \
	inline void NAME(int n,t_sample *const *in,t_sample *const *out) { (this->*v_##NAME)(n,in,out); } \
	void (thisType::*v_##NAME)(int n,t_sample *const *invecs,t_sample *const *outvecs)

	#define SETSTFUN(VAR,FUN) VAR = FUN

	#define DEFSTCALL(NAME) \
	void (*NAME)(const Element *bdt,const int smin,const int smax,const int n,const int inchns,const int outchns,t_sample *const *invecs,t_sample *const *outvecs,bool looped)
#endif





#ifndef MIN
#define MIN(x,y) ((x) < (y)?(x):(y))
#endif

// in the signal functions
#ifdef TMPLOPT
	// optimization by using constants for channel numbers
	#define SIGCHNS(BCHNS,bchns,IOCHNS,iochns)  \
		const int BCHNS = _BCHNS_ < 0?(bchns):_BCHNS_;  \
		const int IOCHNS = _IOCHNS_ < 0?MIN(iochns,BCHNS):MIN(_IOCHNS_,BCHNS)
#else 
	// no template optimization
	#if FLEXT_SYS == FLEXT_SYS_PD // only mono buffers
		#define SIGCHNS(BCHNS,bchns,IOCHNS,iochns)   \
			const int BCHNS = 1;  \
			const int IOCHNS = MIN(iochns,BCHNS)
	#else // MAXMSP
		#define SIGCHNS(BCHNS,bchns,IOCHNS,iochns)   \
			const int BCHNS = bchns;  \
			const int IOCHNS = MIN(iochns,BCHNS)
	#endif
#endif


class xinter:
	public xsample
{
	FLEXT_HEADER_S(xinter,xsample,setup)
	
public:

	enum xs_loop {
		xsl_once = 0,xsl_loop,xsl_bidir
	};

    xinter()
        : outchns(1),doplay(false)
        , interp(xsi_4p),loopmode(xsl_loop)
    {}
	
    void m_start() 
    { 
	    ChkBuffer();
        doplay = true;
        Update(xsc_startstop,true);
    }

    void m_stop() 
    { 
	    ChkBuffer();
        doplay = false;
        Update(xsc_startstop,true);
    }

	void m_interp(xs_intp mode)
    { 
        interp = mode; 
        Update(xsc_intp,true);
    }

protected:

	int outchns;
	bool doplay;	
	xs_intp interp;
	xs_loop loopmode;

	TMPLSIGFUN(s_play0);
	TMPLSIGFUN(s_play1);
	TMPLSIGFUN(s_play2);
	TMPLSIGFUN(s_play4);

	TMPLSTFUN(st_play0);
	TMPLSTFUN(st_play1);
	TMPLSTFUN(st_play2);
	TMPLSTFUN(st_play4);

	DEFSIGCALL(playfun);
	DEFSIGCALL(zerofun);

	virtual void DoUpdate(unsigned int flags);

	FLEXT_CALLBACK(m_start)
	FLEXT_CALLBACK(m_stop)

	FLEXT_CALLSET_E(m_interp,xs_intp)
	FLEXT_ATTRGET_E(interp,xs_intp)

	FLEXT_ATTRGET_E(loopmode,xs_loop)

private:
	static void setup(t_classid c);
};

#ifdef TMPLOPT
#include "inter.h"
#endif

}

#endif
