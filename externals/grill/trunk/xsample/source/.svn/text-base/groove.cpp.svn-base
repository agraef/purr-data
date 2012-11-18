/*
xsample - extended sample objects for Max/MSP and pd (pure data)

Copyright (c) 2001-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 39 $
$LastChangedDate$
$LastChangedBy$
*/

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#include "main.h"
#include <cmath>
#include <cstdio>

#ifdef _MSC_VER
#pragma warning (disable:4244)
#endif

#define XZONE_TABLE 512


class xgroove:
	public xinter
{
	FLEXT_HEADER_S(xgroove,xinter,setup)

public:
	xgroove(int argc,const t_atom *argv);
	virtual ~xgroove();

	void m_pos(float pos)
    {
	    setpos(LIKELY(s2u)?pos/s2u:0);
        Update(xsc_pos,true);
    }

    inline void m_posmod(float pos) { setposmod(LIKELY(pos)?pos/s2u:0); } // motivated by Tim Blechmann

	inline void mg_pos(float &v) const { v = curpos*s2u; }


	enum xs_fade {
		xsf_keeplooppos = 0,xsf_keeplooplen,xsf_keepfade,xsf_inside
	};

	enum xs_shape {
		xss_lin = 0,xss_qsine,xss_hsine
	};


	void ms_xfade(int xf)
    { 
        if(xf < 0 || xf > xsf_inside) xf = xsf_keeplooppos;
	    xfade = (xs_fade)xf;
        Update(xsc_fade,true);
    }

    void ms_xshape(int sh);

	void ms_xzone(float xz);
	void mg_xzone(float &xz) { xz = _xzone*s2u; }

	void m_loop(xs_loop lp)
    { 
	    loopmode = lp,bidir = 1;
        Update(xsc_loop,true);
    }
	
protected:

	double curpos;  // in samples
	float bidir; // +1 or -1

	float _xzone,xzone;
    long znsmin,znsmax;
	xs_fade xfade;
	int xshape;
	t_sample **znbuf;
	t_sample *znpos,*znidx;
	Element *znmul;
	int pblksz;

	inline void setpos(double pos)
	{
		if(UNLIKELY(pos < znsmin)) curpos = znsmin;
		else if(UNLIKELY(pos > znsmax)) curpos = znsmax;
		else curpos = pos;
	}

	inline void setposmod(double pos)
	{
        if(pos >= 0) 
            curpos = znsmin+fmod(pos,znsmax-znsmin);
        else 
            curpos = znsmax+fmod(pos,znsmax-znsmin);
	}

    virtual void DoReset();
    virtual void DoUpdate(unsigned int flags);

	virtual void CbSignal();

	virtual void m_help();
	virtual void m_print();

private:
	static void setup(t_classid c);

    //! return true if something has changed
	bool do_xzone();

	DEFSIGFUN(s_pos_off);
	DEFSIGFUN(s_pos_once);
	DEFSIGFUN(s_pos_loop);
	DEFSIGFUN(s_pos_loopzn);
	DEFSIGFUN(s_pos_bidir);

	DEFSIGCALL(posfun);
	DEFSTCALL(zonefun);

	static Element fade_lin[],fade_qsine[],fade_hsine[];

	FLEXT_CALLBACK_F(m_pos)
	FLEXT_CALLBACK_F(m_posmod)
	FLEXT_CALLBACK_F(m_min)
	FLEXT_CALLBACK_F(m_max)
	FLEXT_CALLBACK(m_all)

    FLEXT_CALLSET_E(m_loop,xs_loop)

	FLEXT_CALLSET_I(ms_xfade)
	FLEXT_ATTRGET_I(xfade)
	FLEXT_CALLSET_I(ms_xshape)
	FLEXT_ATTRGET_I(xshape)
	FLEXT_CALLSET_F(ms_xzone)
	FLEXT_CALLGET_F(mg_xzone)

	FLEXT_CALLVAR_F(mg_pos,m_pos)
	FLEXT_CALLSET_F(m_min)
	FLEXT_CALLSET_F(m_max)
};

#ifdef XGROOVE_STANDALONE
FLEXT_NEW_DSP_V("xgroove~",xgroove)
#else
FLEXT_LIB_DSP_V("xgroove~",xgroove)
#endif

Element xgroove::fade_lin[XZONE_TABLE+1];
Element xgroove::fade_qsine[XZONE_TABLE+1];
Element xgroove::fade_hsine[XZONE_TABLE+1];

void xgroove::setup(t_classid c)
{
#ifdef XGROOVE_STANDALONE
	flext::post("-------------------------------");
	flext::post("  xgroove~, version " XSAMPLE_VERSION);
    flext::post("");
    flext::post("  (C)2001-2011 Thomas Grill    ");
#ifdef FLEXT_DEBUG
    flext::post("");
    flext::post("DEBUG BUILD - " __DATE__ " " __TIME__);
#endif
	flext::post("-------------------------------");
#endif

	DefineHelp(c,"xgroove~");

	FLEXT_CADDMETHOD_(c,0,"all",m_all);
	FLEXT_CADDMETHOD(c,1,m_min);
	FLEXT_CADDMETHOD(c,2,m_max);

	FLEXT_CADDATTR_VAR(c,"min",mg_min,m_min); 
	FLEXT_CADDATTR_VAR(c,"max",mg_max,m_max);
	FLEXT_CADDATTR_VAR(c,"pos",mg_pos,m_pos);
	FLEXT_CADDMETHOD_(c,0,"posmod",m_posmod);

	FLEXT_CADDATTR_VAR_E(c,"loop",loopmode,m_loop);

	FLEXT_CADDATTR_VAR(c,"xfade",xfade,ms_xfade);
	FLEXT_CADDATTR_VAR(c,"xzone",mg_xzone,ms_xzone);
	FLEXT_CADDATTR_VAR(c,"xshape",xshape,ms_xshape);

	// initialize fade tables
	for(int i = 0; i <= XZONE_TABLE; ++i) {
		const float x = i*(1.f/XZONE_TABLE);
		// linear
		fade_lin[i] = x;

		// quarter sine wave
		fade_qsine[i] = sin(x*(M_PI/2));

		// half sine wave
		fade_hsine[i] = (sin(x*M_PI-M_PI/2)+1.f)*0.5f;
	}
}

xgroove::xgroove(int argc,const t_atom *argv):
	curpos(0),bidir(1),
	_xzone(0),xzone(0),
	xfade(xsf_keeplooppos),xshape(xss_lin),
	znpos(NULL),znmul(NULL),znidx(NULL),
	pblksz(0)
{
	int argi = 0;
#if FLEXT_SYS == FLEXT_SYS_MAX
	if(argc > argi && CanbeInt(argv[argi])) {
		outchns = GetAInt(argv[argi]);
		argi++;
	}
#endif

	if(argc > argi && IsSymbol(argv[argi])) {

        buf.Set(GetSymbol(argv[argi]),true);
		argi++;
		
#if FLEXT_SYS == FLEXT_SYS_MAX
		// old-style command line?
		if(UNLIKELY(argi == 1 && argc == 2 && CanbeInt(argv[argi]))) {
			outchns = GetAInt(argv[argi]);
			argi++;
			post("%s: old style command line detected - please change to '%s [channels] [buffer]'",thisName(),thisName()); 
		}
#endif
	}

    AddInSignal("Signal of playing speed"); // speed signal
	AddInFloat("Starting point"); // min play pos
	AddInFloat("Ending point"); // max play pos
	for(int ci = 0; ci < outchns; ++ci) {
		char tmp[30];
		STD::sprintf(tmp,"Audio signal channel %i",ci+1); 
		AddOutSignal(tmp); // output
	}
	AddOutSignal("Position currently played"); // position
	AddOutFloat("Starting point (rounded to frame)"); // play min 
	AddOutFloat("Ending point (rounded to frame)"); // play max
	AddOutBang("Bang on loop end/rollover");  // loop bang
	
	// don't know vector size yet -> wait for m_dsp
	znbuf = new t_sample *[outchns];
	for(int i = 0; i < outchns; ++i) znbuf[i] = NULL;

    // initialize crossfade shape
    ms_xshape(xshape);
}

xgroove::~xgroove()
{
	if(znbuf) {
		for(int i = 0; i < outchns; ++i) if(znbuf[i]) FreeAligned(znbuf[i]); 
		delete[] znbuf;
	}

	if(znpos) FreeAligned(znpos);
	if(znidx) FreeAligned(znidx);
}

void xgroove::DoReset()
{
    xinter::DoReset();
	curpos = 0; 
	bidir = 1;
}

void xgroove::ms_xzone(float xz) 
{ 
	ChkBuffer(true);

	_xzone = (UNLIKELY(xz < 0) || UNLIKELY(!s2u))?0:xz/s2u; 
    Update(xsc_fade,true);
}

void xgroove::ms_xshape(int sh) 
{ 
    if(UNLIKELY(sh < 0) || UNLIKELY(sh > xss_hsine)) sh = xss_lin;

	xshape = (xs_shape)sh;
	switch(xshape) {
		case xss_qsine: znmul = fade_qsine; break;
		case xss_hsine: znmul = fade_hsine; break;
		default:
			post("%s - shape parameter invalid, set to linear",thisName());
		case xss_lin: 
			znmul = fade_lin; break;
	}

	// no need to recalc the fade zone here
}


void xgroove::s_pos_off(int n,t_sample *const *invecs,t_sample *const *outvecs)
{
	t_sample *pos = outvecs[outchns];

	SetSamples(pos,n,curpos);
	
	playfun(n,&pos,outvecs); 

	SetSamples(pos,n,scale(curpos));
}

void xgroove::s_pos_once(int n,t_sample *const *invecs,t_sample *const *outvecs)
{
	const t_sample *speed = invecs[0];
	t_sample *pos = outvecs[outchns];
	bool lpbang = false;

	const double smin = curmin,smax = curmax,plen = smax-smin;

	if(LIKELY(plen > 0)) {
		register double o = curpos;

		for(int i = 0; i < n; ++i) {	
			const t_sample spd = speed[i];  // must be first because the vector is reused for output!
			
			if(UNLIKELY(!(o < smax))) { o = smax; lpbang = true; }
			else if(UNLIKELY(o < smin)) { o = smin; lpbang = true; }
			
			pos[i] = o;
			o += spd;
		}
		// normalize and store current playing position
		setpos(o);

		playfun(n,&pos,outvecs); 

		arrscale(n,pos,pos);
	} 
	else 
		s_pos_off(n,invecs,outvecs);
		
    if(UNLIKELY(lpbang)) {
        doplay = false;
        ToOutBang(outchns+3);
    }
}

void xgroove::s_pos_loop(int n,t_sample *const *invecs,t_sample *const *outvecs)
{
	const t_sample *speed = invecs[0];
	t_sample *pos = outvecs[outchns];
	bool lpbang = false;

#ifdef __VEC__
	// prefetch cache
	vec_dst(speed,GetPrefetchConstant(1,n>>2,0),0);
#endif

	const double smin = curmin,smax = curmax,plen = smax-smin;

	if(LIKELY(plen > 0)) {
		register double o = curpos;

        if(wrap && UNLIKELY(smin < 0) && UNLIKELY(smax >= buf.Frames())) {
		    for(int i = 0; i < n; ++i) {	
			    const t_sample spd = speed[i];  // must be first because the vector is reused for output!

			    // normalize offset
			    if(UNLIKELY(!(o < smax))) {  // faster than o >= smax
				    o = fmod(o-smin,plen)+smin;
				    lpbang = true;
			    }
			    else if(UNLIKELY(o < smin)) {
				    o = fmod(o-smin,plen)+smax; 
				    lpbang = true;
			    }

                // TODO normalize to 0...buf.Frames()
			    pos[i] = o;
			    o += spd;
		    }
        }
        else {
			///////////////////////////////////
			// Most of the time is spent in here
			///////////////////////////////////
		    for(int i = 0; i < n; ++i) {	
			    const t_sample spd = speed[i];  // must be first because the vector is reused for output!

			    // normalize offset
			    if(UNLIKELY(!(o < smax))) {  // faster than o >= smax
				    o = fmod(o-smin,plen)+smin;
				    lpbang = true;
			    }
			    else if(UNLIKELY(o < smin)) {
				    o = fmod(o-smin,plen)+smax; 
				    lpbang = true;
			    }

			    pos[i] = o;
			    o += spd;
		    }
        }

		// normalize and store current playing position
		setpos(o);

		playfun(n,&pos,outvecs); 

		arrscale(n,pos,pos);
	} 
	else 
		s_pos_off(n,invecs,outvecs);
		
#ifdef __VEC__
	vec_dss(0);
#endif

	if(UNLIKELY(lpbang)) ToOutBang(outchns+3);
}

void xgroove::s_pos_loopzn(int n,t_sample *const *invecs,t_sample *const *outvecs)
{
	const t_sample *speed = invecs[0];
	t_sample *pos = outvecs[outchns];
	bool lpbang = false;

	FLEXT_ASSERT(xzone);

	const float xz = xzone,xf = (float)XZONE_TABLE/xz;

    // adapt the playing bounds to the current cross-fade zone
    const long smin = znsmin,smax = znsmax,plen = smax-smin;

	// temporary storage
	const long cmin = curmin,cmax = curmax;
	// hack -> set curmin/curmax to loop extremes so that sampling functions (playfun) don't get confused
	curmin = smin,curmax = smax;

	if(LIKELY(plen > 0)) {
		bool inzn = false;
		register double o = curpos;

		// calculate inner cross-fade boundaries
		const double lmin = smin+xz,lmax = smax-xz,lsh = lmax-lmin+xz;
		const double lmin2 = lmin-xz/2,lmax2 = lmax+xz/2;

 		for(int i = 0; i < n; ++i) {	
			// normalize offset
            if(UNLIKELY(o < smin)) {
				o = fmod(o-smin,plen)+smax; 
				lpbang = true;
			}
			else if(UNLIKELY(!(o < smax))) {
				o = fmod(o-smin,plen)+smin;
				lpbang = true;
			}

			if(UNLIKELY(o < lmin)) {
				register float inp;
				if(o < lmin2) {
					// in first half of early cross-fade zone
					// this happens only once, then the offset is normalized to the end
					// of the loop (before mid of late crossfade)

					o += lsh;
					// now lmax <= o <= lmax2
					lpbang = true;

					inp = xz-(float)(o-lmax);  // 0 <= inp < xz
					znpos[i] = lmin-inp;
				}
				else { 
					// in second half of early cross-fade zone

					inp = xz+(float)(o-lmin);  // 0 <= inp < xz
					znpos[i] = lmax+inp;
				}
				znidx[i] = inp*xf;
				inzn = true;
			}
			else if(UNLIKELY(!(o < lmax))) {
				register float inp;
				if(!(o < lmax2)) {
					// in second half of late cross-fade zone
					// this happens only once, then the offset is normalized to the beginning
					// of the loop (after mid of early crossfade)
					o -= lsh;
					// now lmin2 <= o <= lmin
					lpbang = true;

					inp = xz+(float)(o-lmin);  // 0 <= inp < xz
					znpos[i] = lmax+inp;
				}
				else { 
					// in first half of late cross-fade zone
					inp = xz-(float)(o-lmax);  // 0 <= inp < xz
					znpos[i] = lmin-inp;
				}
				znidx[i] = inp*xf;
				inzn = true;
			}
			else
				znidx[i] = XZONE_TABLE,znpos[i] = 0;

			const t_sample spd = speed[i];  // must be first because the vector is reused for output!
			pos[i] = o;
			o += spd;
		}

		// normalize and store current playing position
		setpos(o);

		// calculate samples (1st voice)
		playfun(n,&pos,outvecs); 

		// rescale position vector
		arrscale(n,pos,pos);

		if(UNLIKELY(inzn)) {
			// only if we have touched the cross-fade zone
			
			// calculate samples in loop zone (2nd voice)
			playfun(n,&znpos,znbuf); 
			
			// calculate counterpart in loop fade
			arrscale(n,znidx,znpos,XZONE_TABLE,-1);
			
			// calculate fade coefficients by sampling from the fade curve
			zonefun(znmul,0,XZONE_TABLE+1,n,1,1,&znidx,&znidx,false);
			zonefun(znmul,0,XZONE_TABLE+1,n,1,1,&znpos,&znpos,false);

			// mix voices for all channels
			for(int o = 0; o < outchns; ++o) {
				MulSamples(outvecs[o],outvecs[o],znidx,n);
				MulSamples(znbuf[o],znbuf[o],znpos,n);
				AddSamples(outvecs[o],outvecs[o],znbuf[o],n);
			}
		}
	} 
	else 
		s_pos_off(n,invecs,outvecs);
		
	curmin = cmin,curmax = cmax;

	if(UNLIKELY(lpbang)) ToOutBang(outchns+3);
}

void xgroove::s_pos_bidir(int n,t_sample *const *invecs,t_sample *const *outvecs)
{
	const t_sample *speed = invecs[0];
	t_sample *pos = outvecs[outchns];
	bool lpbang = false;

	const int smin = curmin,smax = curmax,plen = smax-smin;

	if(LIKELY(plen > 0)) {
		register double o = curpos;
		register float bd = bidir;

		for(int i = 0; i < n; ++i) {	
			const t_sample spd = speed[i];  // must be first because the vector is reused for output!

			// normalize offset
            // \todo at the moment fmod doesn't take bidirectionality into account!!
			if(UNLIKELY(!(o < smax))) {
				o = smax-fmod(o-smax,plen); // mirror the position at smax
				bd = -bd;
				lpbang = true;
			}
			else if(UNLIKELY(o < smin)) {
				o = smin+fmod(smin-o,plen); // mirror the position at smin
				bd = -bd;
				lpbang = true;
			}

			pos[i] = o;
			o += spd*bd;
		}
		// normalize and store current playing position
		setpos(o);

		bidir = bd;
		playfun(n,&pos,outvecs); 

		arrscale(n,pos,pos);
	} 
    else
		s_pos_off(n,invecs,outvecs);
		
	if(UNLIKELY(lpbang)) ToOutBang(outchns+3);
}

void xgroove::CbSignal() 
{ 
    int ret = ChkBuffer(true);

    if(LIKELY(ret)) {
        FLEXT_ASSERT(buf.Valid());
        
        const lock_t l = Lock();
		posfun(Blocksize(),InSig(),OutSig()); 
        Unlock(l);

        Refresh();
    }
	else
		zerofun(Blocksize(),InSig(),OutSig());
}


void xgroove::DoUpdate(unsigned int flags)
{
    xinter::DoUpdate(flags);

    if(flags&xsc_range) {
        // output new range
	    ToOutFloat(outchns+1,curmin*s2u);
	    ToOutFloat(outchns+2,curmax*s2u);
    }

    if(flags&(xsc_fade|xsc_range))
        if(do_xzone()) flags |= xsc_play;

    if(flags&(xsc_pos|xsc_range))
        // normalize position
        setpos(curpos);

    // loop zone must already be set
    if(flags&xsc_play) {
	    if(doplay) {
		    switch(loopmode) {
		    case xsl_once: 
			    SETSIGFUN(posfun,SIGFUN(s_pos_once)); 
			    break;
		    case xsl_loop: 
			    if(xzone > 0) {
				    const int blksz = Blocksize();

				    if(pblksz != blksz) {
					    for(int o = 0; o < outchns; ++o) {
						    if(znbuf[o]) FreeAligned(znbuf[o]); 
						    znbuf[o] = (t_sample *)NewAligned(blksz*sizeof(t_sample)); 
					    }
    				
					    if(znpos) FreeAligned(znpos); 
					    znpos = (t_sample *)NewAligned(blksz*sizeof(t_sample));
					    if(znidx) FreeAligned(znidx);
					    znidx = (t_sample *)NewAligned(blksz*sizeof(t_sample));

					    pblksz = blksz;
				    }

				    SETSIGFUN(posfun,SIGFUN(s_pos_loopzn)); 

				    // linear interpolation should be just ok for fade zone, no?
				    switch(outchns) {
					    case 1:	SETSTFUN(zonefun,TMPLSTF(st_play2,1,1)); break;
					    case 2:	SETSTFUN(zonefun,TMPLSTF(st_play2,1,2)); break;
					    case 4:	SETSTFUN(zonefun,TMPLSTF(st_play2,1,4)); break;
					    default: SETSTFUN(zonefun,TMPLSTF(st_play2,1,-1));
				    }
			    }
			    else
				    SETSIGFUN(posfun,SIGFUN(s_pos_loop)); 
			    break;
		    case xsl_bidir: 
			    SETSIGFUN(posfun,SIGFUN(s_pos_bidir)); 
			    break;
            default: ; // just to prevent warning
		    }
	    }
	    else
		    SETSIGFUN(posfun,SIGFUN(s_pos_off));
    }
}

bool xgroove::do_xzone()
{
	// \todo do we really need this?
	if(UNLIKELY(!s2u)) return false; // this can happen if DSP is off 

    const long frames = buf.Frames();
    if(UNLIKELY(!frames)) return false;

	xzone = _xzone; // make a copy for changing it

	if(xfade == xsf_inside) { 
		// fade zone goes inside the loop -> loop becomes shorter

		// \todo what about round-off?
		const long maxfd = (curmax-curmin)/2;
		if(xzone > maxfd) xzone = maxfd;

		znsmin = curmin,znsmax = curmax;
	}
	else if(xfade == xsf_keepfade) { 
		// try to keep fade zone
		// change of loop bounds may happen

		// restrict xzone to half of buffer
		const long maxfd = frames/2;
		if(xzone > maxfd) xzone = maxfd;

		// \todo what about round-off?
        const long hzone = CASTINT<long>(xzone/2.f+0.5f);
		znsmin = curmin-hzone;
		znsmax = curmax+hzone;

		// widen loop if xzone doesn't fit into it
		// \todo check formula
		long lack = CASTINT<long>(ceil((xzone*2.f-(znsmax-znsmin))/2.f));
		if(lack > 0) znsmin -= lack,znsmax += lack;

        if(!wrap) {
		    // check buffer limits and shift bounds if necessary
		    if(znsmin < 0) {
			    znsmax -= znsmin;
			    znsmin = 0;
		    }
		    if(znsmax > frames) 
                znsmax = frames;
        }
	}
	else if(xfade == xsf_keeplooplen) { 
		// try to keep loop length
		// shifting of loop bounds may happen

		const long plen = curmax-curmin;
		if(xzone > plen) xzone = plen;
		const long maxfd = frames-plen;
		if(xzone > maxfd) xzone = maxfd;

		// \todo what about round-off?
        const long hzone = CASTINT<long>(xzone/2.f+0.5f);
		znsmin = curmin-hzone;
		znsmax = curmax+hzone;

        if(!wrap) {
		    // check buffer limits and shift bounds if necessary
		    // both cases can't happen because of xzone having been limited above
		    if(znsmin < 0) {
			    znsmax -= znsmin;
			    znsmin = 0;
		    }
		    else if(znsmax > frames) {
			    znsmin -= znsmax-frames;
			    znsmax = frames;
		    }
        }
	}
	else if(xfade == xsf_keeplooppos) { 
		// try to keep loop position and length

		// restrict fade zone to maximum length 
		const long plen = curmax-curmin;
		if(xzone > plen) xzone = plen;

		// \todo what about round-off?
        const long hzone = CASTINT<long>(xzone/2.f+0.5f);
		znsmin = curmin-hzone;
		znsmax = curmax+hzone;

		long ovr = znsmax-frames;
		if(-znsmin > ovr) ovr = -znsmin;
		if(ovr > 0) {
			znsmin += ovr;
			znsmax -= ovr;
			xzone -= ovr*2;
		}
	}

	FLEXT_ASSERT(znsmin <= znsmax && (znsmax-znsmin) >= xzone*2.f);

    return true;
}


void xgroove::m_help()
{
	post("%s - part of xsample objects, version " XSAMPLE_VERSION,thisName());
	post("(C) Thomas Grill, 2001-2010");
#if FLEXT_SYS == FLEXT_SYS_MAX
	post("Arguments: %s [channels=1] [buffer]",thisName());
#else
	post("Arguments: %s [buffer]",thisName());
#endif
	post("Inlets: 1:Messages/Speed signal, 2:Min position, 3:Max position");
	post("Outlets: 1:Audio signal, 2:Position signal, 3:Min position (rounded), 4:Max position (rounded)");	
	post("Methods:");
	post("\thelp: shows this help");
	post("\tset [name] / @buffer [name]: set buffer or reinit");
	post("\tenable 0/1: turn dsp calculation off/on");	
	post("\treset: reset min/max playing points and playing offset");
	post("\tprint: print current settings");
	post("\t@loop 0/1/2: sets looping to off/forward/bidirectional");
	post("\t@interp 0/1/2: set interpolation to off/4-point/linear");
	post("\t@min {unit}: set minimum playing point");
	post("\t@max {unit}: set maximum playing point");
	post("\tall: select entire buffer length");
	post("\tpos {unit}: set playing position (obeying the current scale mode)");
	post("\tposmod {unit}: set playing position (modulo into min/max range)");
	post("\tbang/start: start playing");
	post("\tstop: stop playing");
	post("\trefresh: checks buffer and refreshes outlets");
	post("\t@units 0/1/2/3: set units to frames/buffer size/ms/s");
	post("\t@sclmode 0/1/2/3: set range of position to units/units in loop/buffer/loop");
	post("\t@xzone {unit}: length of loop crossfade zone");
	post("\t@xfade 0/1/2/3: fade mode (keep loop/keep loop length/keep fade/inside loop)");
	post("\t@xshape 0/1/2: shape of crossfade (linear/quarter sine/half sine)");
	post("");
} 

void xgroove::m_print()
{
	static const char *sclmode_txt[] = {"units","units in loop","buffer","loop"};
	static const char *interp_txt[] = {"off","4-point","linear"};
	static const char *loop_txt[] = {"once","looping","bidir"};

	// print all current settings
	post("%s - current settings:",thisName());
	post("bufname = '%s', length = %.3f, channels = %i",buf.Name(),(float)(buf.Frames()*s2u),buf.Channels()); 
	post("out channels = %i, frames/unit = %.3f, scale mode = %s",outchns,(float)(1./s2u),sclmode_txt[sclmode]); 
	post("loop = %s, interpolation = %s",loop_txt[(int)loopmode],interp_txt[interp >= xsi_none && interp <= xsi_lin?interp:xsi_none]); 
	post("loop crossfade zone = %.3f",(float)(xzone*s2u)); 
	post("");
}
