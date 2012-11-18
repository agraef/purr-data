/*
xsample - extended sample objects for Max/MSP and pd (pure data)

Copyright (c) 2001-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 39 $
$LastChangedDate$
$LastChangedBy$
*/

#include "main.h"
#include <cstdio>

#ifdef _MSC_VER
#pragma warning (disable:4244)
#endif

class xrecord:
	public xsample
{
	FLEXT_HEADER_S(xrecord,xsample,setup)

public:
	xrecord(int argc,const t_atom *argv);
	
	void m_pos(float pos)
    {
	    curpos = LIKELY(pos)?CASTINT<long>(pos/s2u+.5):0;
        Update(xsc_pos);
        Refresh();
    }

	inline void mg_pos(float &v) const { v = curpos*s2u; }

    void m_start();
	void m_stop();

	inline void m_append(bool app) 
    { 
        appmode = app;
        Update(xsc_play);
        if(!appmode) m_pos(0); 
    }

	void m_draw(int argc,const t_atom *argv);	

protected:
	int inchns;
	bool sigmode,appmode;
	float drintv;

	bool dorec,doloop;
    int mixmode;
	long curpos;  // in samples

    virtual void DoReset();
    virtual void DoUpdate(unsigned int flags);
	
	virtual void CbSignal();
	
	virtual void m_help();
	virtual void m_print();

private:
	static void setup(t_classid c);

	TMPLSIGFUN(s_rec);

	DEFSIGCALL(recfun);

	FLEXT_CALLBACK(m_start)
	FLEXT_CALLBACK(m_stop)

	FLEXT_CALLVAR_F(mg_pos,m_pos)
	FLEXT_CALLBACK(m_all)
	FLEXT_CALLSET_F(m_min)
	FLEXT_CALLSET_F(m_max)
	FLEXT_CALLBACK_F(m_min)
	FLEXT_CALLBACK_F(m_max)

	FLEXT_ATTRVAR_B(doloop)
	FLEXT_ATTRVAR_I(mixmode)
	FLEXT_ATTRVAR_B(sigmode)
	FLEXT_CALLSET_B(m_append)
	FLEXT_ATTRGET_B(appmode)

	FLEXT_CALLBACK_V(m_draw)
};

#ifdef XRECORD_STANDALONE
FLEXT_NEW_DSP_V("xrecord~",xrecord)
#else
FLEXT_LIB_DSP_V("xrecord~",xrecord)
#endif


void xrecord::setup(t_classid c)
{
#ifdef XRECORD_STANDALONE
	flext::post("-------------------------------");
	flext::post("  xrecord~, version " XSAMPLE_VERSION);
    flext::post("");
    flext::post("  (C)2001-2010 Thomas Grill    ");
#ifdef FLEXT_DEBUG
    flext::post("");
    flext::post("DEBUG BUILD - " __DATE__ " " __TIME__);
#endif
	flext::post("-------------------------------");
#endif

	DefineHelp(c,"xrecord~");

	FLEXT_CADDBANG(c,0,m_start);
	FLEXT_CADDMETHOD_(c,0,"start",m_start);
	FLEXT_CADDMETHOD_(c,0,"stop",m_stop);

	FLEXT_CADDATTR_VAR(c,"pos",mg_pos,m_pos);
	FLEXT_CADDATTR_VAR(c,"min",mg_min,m_min);
	FLEXT_CADDATTR_VAR(c,"max",mg_max,m_max);
	FLEXT_CADDMETHOD_(c,0,"all",m_all);
	
	FLEXT_CADDMETHOD_(c,0,"draw",m_draw);
	
	FLEXT_CADDATTR_VAR1(c,"loop",doloop);
	FLEXT_CADDATTR_VAR1(c,"mixmode",mixmode);
	FLEXT_CADDATTR_VAR1(c,"sigmode",sigmode);
	FLEXT_CADDATTR_VAR(c,"append",appmode,m_append);
}

xrecord::xrecord(int argc,const t_atom *argv):
	inchns(1),
	sigmode(false),appmode(true),
	drintv(0),
	dorec(false),doloop(false),
    mixmode(0)
{
	int argi = 0;
#if FLEXT_SYS == FLEXT_SYS_MAX
	if(argc > argi && CanbeInt(argv[argi])) {
		inchns = GetAInt(argv[argi]);
		argi++;
	}
#endif

	if(argc > argi && IsSymbol(argv[argi])) {
		buf.Set(GetSymbol(argv[argi]),true);
		argi++;

#if FLEXT_SYS == FLEXT_SYS_MAX
		// oldstyle command line?
		if(argi == 1 && argc == 2 && CanbeInt(argv[argi])) {
			inchns = GetAInt(argv[argi]);
			argi++;
			post("%s: old style command line detected - please change to '%s [channels] [buffer]'",thisName(),thisName()); 
		}
#endif
	}

    for(int ci = 0; ci < inchns; ++ci) {
		char tmp[40];
		STD::sprintf(tmp,ci == 0?"Messages/audio channel %i":"Audio channel %i",ci+1);
		AddInSignal(tmp);  // audio signals
	}
	AddInSignal("On/Off/Fade/Mix signal (0..1)"); // on/off signal
	AddInFloat("Starting point of recording");  // min 
	AddInFloat("Ending point of recording");  // max
	AddOutSignal("Current position of recording");  // pos signal
	AddOutFloat("Starting point (rounded to frame)"); // min 
	AddOutFloat("Ending point (rounded to frame)"); // max
	AddOutBang("Bang on loop end/rollover");  // loop bang

	FLEXT_ADDMETHOD(inchns+1,m_min);
	FLEXT_ADDMETHOD(inchns+2,m_max);
}

void xrecord::m_start() 
{ 
    ChkBuffer();

    if(!sigmode && !appmode) { curpos = 0; Update(xsc_pos); }

    dorec = true; 
    Update(xsc_startstop);
    Refresh();
}

void xrecord::m_stop() 
{ 
    ChkBuffer();
    dorec = false; 
    Update(xsc_startstop);
    Refresh();
}

void xrecord::DoReset()
{
    xsample::DoReset();
	curpos = 0;
}

void xrecord::m_draw(int argc,const t_atom *argv)
{
	if(argc >= 1) {
		drintv = GetAInt(argv[0]);
		if(dorec) buf.SetRefrIntv(drintv);
	}
	else
		buf.Dirty(true);
}
	
TMPLDEF void xrecord::s_rec(int n,t_sample *const *invecs,t_sample *const *outvecs)
{
	SIGCHNS(BCHNS,buf.Channels(),ICHNS,inchns);

	const t_sample *const *sig = invecs;
	register int si = 0;
	const t_sample *on = invecs[inchns];
	t_sample *pos = outvecs[0];

	bool lpbang = false;
	register const float pf = sclmul;
	register long o = curpos;
	
	if(o < curmin) o = curmin;

	if(dorec && LIKELY(curmax > curmin)) {
		while(n) {
			long ncur = curmax-o; // at max to buffer or recording end

			if(UNLIKELY(ncur <= 0)) {	// end of buffer
				if(doloop) { 
					ncur = curmax-(o = curmin);
				}
                else {
					// loop expired;
                    dorec = false; 
                    Update(xsc_startstop);
                }
					
				lpbang = true;
			}

			if(UNLIKELY(!dorec)) break;

			if(UNLIKELY(ncur > n)) ncur = n;
			
			register int i;
			register Element *bf = buf.Data()+o*BCHNS;
			register float p = scale(o);

			if(sigmode) {
				if(appmode) {
					// append to current position
				
					switch(mixmode) {
                    case 0:
						for(i = 0; i < ncur; ++i,++si) {	
							if(!(*(on++) < 0)) {
								for(int ci = 0; ci < ICHNS; ++ci)
									bf[ci] = sig[ci][si];	
								bf += BCHNS;
								*(pos++) = p,p += pf,++o;
							}
							else 
								*(pos++) = p;
						}
                        break;
                    case 1:
						for(i = 0; i < ncur; ++i,++si) {	
							register const t_sample g = *(on++);
							if(!(g < 0)) {
								for(int ci = 0; ci < ICHNS; ++ci)
									bf[ci] = bf[ci]*(1.-g)+sig[ci][si]*g;
								bf += BCHNS;
								*(pos++) = p,p += pf,++o;
							}
							else 
								*(pos++) = p;
						}
                        break;
                    case 2:
						for(i = 0; i < ncur; ++i,++si) {	
							if(!(*(on++) < 0)) {
								for(int ci = 0; ci < ICHNS; ++ci)
									bf[ci] += sig[ci][si];	
								bf += BCHNS;
								*(pos++) = p,p += pf,++o;
							}
							else 
								*(pos++) = p;
						}
                        break;
                    }
				}
				else {  
					// don't append
					switch(mixmode) {
                        case 0: {
						    for(i = 0; i < ncur; ++i,++si) {	
							    if(!(*(on++) < 0))
							    { 	
								    for(int ci = 0; ci < ICHNS; ++ci)
									    bf[ci] = sig[ci][si];	
								    bf += BCHNS;
								    *(pos++) = p,p += pf,++o;
							    }
							    else {
								    *(pos++) = p = scale(o = 0);
								    bf = buf.Data();
							    }
						    }
                            break;
                        }
                        case 1: {
						    for(i = 0; i < ncur; ++i,++si) {	
							    register const t_sample g = *(on++);
							    if(!(g < 0)) {
								    for(int ci = 0; ci < ICHNS; ++ci)
									    bf[ci] = bf[ci]*(1.-g)+sig[ci][si]*g;
								    bf += BCHNS;
								    *(pos++) = p,p += pf,++o;
							    }
							    else {
								    *(pos++) = p = scale(o = 0);
								    bf = buf.Data();
							    }
						    }
                            break;
                        }
                        case 2: {
						    for(i = 0; i < ncur; ++i,++si) {	
							    if(!(*(on++) < 0))
							    { 	
								    for(int ci = 0; ci < ICHNS; ++ci)
									    bf[ci] += sig[ci][si];	
								    bf += BCHNS;
								    *(pos++) = p,p += pf,++o;
							    }
							    else {
								    *(pos++) = p = scale(o = 0);
								    bf = buf.Data();
							    }
						    }
                            break;
					    }
                    }
				}
			}
			else { 
				// message mode
				
				// \TODO Altivec optimization for that!
				switch(mixmode) {
                    case 0: {
					    for(int ci = 0; ci < ICHNS; ++ci) {	
						    register Element *b = bf+ci;
						    register const t_sample *s = sig[ci]+si;
						    for(i = 0; i < ncur; ++i,b += BCHNS,++s) 
                                *b = *s;	
					    }
					    si += ncur;
                        break;
                    }
                    case 1: {
					    for(i = 0; i < ncur; ++i,++si) {	
						    register const t_sample w = *(on++);
						    for(int ci = 0; ci < ICHNS; ++ci)
							    bf[ci] = bf[ci]*(1.-w)+sig[ci][si]*w;
						    bf += BCHNS;
					    }
                        break;
                    }
                    case 2: {
					    for(int ci = 0; ci < ICHNS; ++ci) {	
						    register Element *b = bf+ci;
						    register const t_sample *s = sig[ci]+si;
						    for(i = 0; i < ncur; ++i,b += BCHNS,++s) 
                                *b += *s;	
					    }
					    si += ncur;
                        break;
                    }
				}

				for(i = 0; i < ncur; ++i) {
					*(pos++) = p,p += pf,++o;
				}
			}

			n -= ncur;
		} 
		curpos = o;
		
		buf.Dirty(); 
	}

	if(n) {
		register float p = scale(o);
		while(n--) *(pos++) = p;
	}
	
	if(lpbang) ToOutBang(3);
}

void xrecord::CbSignal() 
{ 
    int ret = ChkBuffer(true);

    if(ret) {
		// call the appropriate dsp function
        
        const lock_t l = Lock();
		recfun(Blocksize(),InSig(),OutSig());
        Unlock(l);
         
        Refresh();
    }
	else
		// set position signal to zero
		ZeroSamples(OutSig()[0],Blocksize());
}

void xrecord::DoUpdate(unsigned int flags)
{
    xsample::DoUpdate(flags);

    if(flags&(xsc_pos|xsc_range)) {
	    if(curpos < curmin) curpos = curmin;
	    else if(curpos > curmax) curpos = curmax;
    }

    if(flags&xsc_range) {
	    ToOutFloat(1,curmin*s2u);
	    ToOutFloat(2,curmax*s2u);
    }

    if(flags&xsc_transport && buf.Ok()) {
        if(dorec)
        	buf.SetRefrIntv(drintv);
        else {
	        buf.Dirty(true);
	        buf.SetRefrIntv(0);
        }
    }

    if(flags&xsc_play) {
	    switch(buf.Channels()*1000+inchns) {
	    case 1001:	SETSIGFUN(recfun,TMPLFUN(s_rec,1,1));	break;
	    case 1002:	SETSIGFUN(recfun,TMPLFUN(s_rec,1,2));	break;
	    case 2001:	SETSIGFUN(recfun,TMPLFUN(s_rec,2,1));	break;
	    case 2002:	SETSIGFUN(recfun,TMPLFUN(s_rec,2,2));	break;
	    case 4001:
	    case 4002:
	    case 4003:	SETSIGFUN(recfun,TMPLFUN(s_rec,4,-1));	break;
	    case 4004:	SETSIGFUN(recfun,TMPLFUN(s_rec,4,4));	break;
	    default:	SETSIGFUN(recfun,TMPLFUN(s_rec,-1,-1));	break;
	    }
    }
}


void xrecord::m_help()
{
	post("%s - part of xsample objects, version " XSAMPLE_VERSION,thisName());
#ifdef FLEXT_DEBUG
	post("compiled on " __DATE__ " " __TIME__);
#endif
	post("(C) Thomas Grill, 2001-2011");
#if FLEXT_SYS == FLEXT_SYS_MAX
	post("Arguments: %s [channels=1] [buffer]",thisName());
#else
	post("Arguments: %s [buffer]",thisName());
#endif
	post("Inlets: 1:Messages/Audio signal, 2:Trigger signal, 3:Min point, 4: Max point");
	post("Outlets: 1:Position signal, 2:Min point, 3:Max point");	
	post("Methods:");
	post("\thelp: shows this help");
	post("\tset [name]: set buffer or reinit");
	post("\tenable 0/1: turn dsp calculation off/on");	
	post("\treset: reset min/max recording points and recording offset");
	post("\tprint: print current settings");
	post("\t@sigmode 0/1: specify message or signal triggered recording");
	post("\t@append 0/1: reset recording position or append to current position");
	post("\t@loop 0/1: switches looping off/on");
	post("\t@mixmode 0/1/2: specify how audio signal should be mixed in (none,mixed,added)");
	post("\tmin {unit}: set minimum recording point");
	post("\tmax {unit}: set maximum recording point");
	post("\tall: select entire buffer length");
	post("\tpos {unit}: set recording position (obeying the current scale mode)");
	post("\tbang/start: start recording");
	post("\tstop: stop recording");
	post("\trefresh: checks buffer and refreshes outlets");
	post("\t@units 0/1/2/3: set units to frames/buffer size/ms/s");
	post("\t@sclmode 0/1/2/3: set range of position to units/units in loop/buffer/loop");
	post("\tdraw [{float}]: redraw buffer immediately (arg omitted) or periodic (in ms)");
	post("");
}

void xrecord::m_print()
{
	static const char sclmode_txt[][20] = {"units","units in loop","buffer","loop"};

	// print all current settings
	post("%s - current settings:",thisName());
	post("bufname = '%s', length = %.3f, channels = %i",buf.Name(),(float)(buf.Frames()*s2u),buf.Channels()); 
	post("in channels = %i, frames/unit = %.3f, scale mode = %s",inchns,(float)(1./s2u),sclmode_txt[sclmode]); 
	post("sigmode = %s, append = %s, loop = %s, mixmode = %i",sigmode?"yes":"no",appmode?"yes":"no",doloop?"yes":"no",mixmode); 
	post("");
}

