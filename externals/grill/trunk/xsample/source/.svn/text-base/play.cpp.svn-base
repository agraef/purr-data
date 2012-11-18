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


class xplay:
	public xinter
{
	FLEXT_HEADER_S(xplay,xinter,setup)

public:
	xplay(int argc,const t_atom *argv);

	void m_loop(xs_loop lp)
    { 
	    loopmode = lp;
        Update(xsc_loop,true);
    }
	
protected:

    virtual void CbSignal();

    virtual void m_help();
	virtual void m_print();
	
private:
	static void setup(t_classid c);

    FLEXT_CALLSET_E(m_loop,xs_loop)
};

#ifdef XPLAY_STANDALONE
FLEXT_NEW_DSP_V("xplay~",xplay)
#else
FLEXT_LIB_DSP_V("xplay~",xplay)
#endif


void xplay::setup(t_classid c)
{
#ifdef XPLAY_STANDALONE
	flext::post("-------------------------------");
	flext::post("   xplay~, version " XSAMPLE_VERSION);
    flext::post("");
    flext::post("  (C)2001-2010 Thomas Grill    ");
#ifdef FLEXT_DEBUG
    flext::post("");
    flext::post("DEBUG BUILD - " __DATE__ " " __TIME__);
#endif
	flext::post("-------------------------------");
#endif

	DefineHelp(c,"xplay~");

	FLEXT_CADDATTR_VAR_E(c,"loop",loopmode,m_loop);
}

xplay::xplay(int argc,const t_atom *argv)
{
    // set the loopmode to non-wrapping (for sample interpolation)
    loopmode = xsl_once;

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
		// oldstyle command line?
		if(argi == 1 && argc == 2 && CanbeInt(argv[argi])) {
			outchns = GetAInt(argv[argi]);
			argi++;
			post("%s: old style command line detected - please change to '%s [channels] [buffer]'",thisName(),thisName()); 
		}
#endif
	}

    AddInSignal("Messages and Signal of playing position");  // pos signal
	for(int ci = 0; ci < outchns; ++ci) {
		char tmp[30];
		STD::sprintf(tmp,"Audio signal channel %i",ci+1); 
		AddOutSignal(tmp);
	}
}

void xplay::CbSignal() 
{ 
	int ret = ChkBuffer(true);
    int n = Blocksize();
    const t_sample *const *in = InSig();
    t_sample *const *out = OutSig();

	// check whether buffer is invalid or changed
	if(ret) {
        const lock_t l = Lock();
        
		// convert position units to frames
		arrmul(n,in[0],out[0]);
		// call resample routine
		playfun(n,out,out); 
        
        Unlock(l);

        Refresh();
	}
	else
		zerofun(n,out,out);
}
	


void xplay::m_help()
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
	post("Inlets: 1:Messages/Position signal");
	post("Outlets: 1:Audio signal");	
	post("Methods:");
	post("\thelp: shows this help");
	post("\tset name: set buffer");
	post("\tenable 0/1: turn dsp calculation off/on");
	post("\tprint: print current settings");
	post("\tbang/start: begin playing");
	post("\tstop: stop playing");
	post("\treset: checks buffer");
	post("\trefresh: checks buffer and refreshes outlets");
	post("\t@units 0/1/2/3: set units to samples/buffer size/ms/s");
	post("\t@interp 0/1/2: set interpolation to off/4-point/linear");
	post("\t@loop 0/1/2: sets looping (interpolation) to off/forward/bidirectional");
	post("");
}

void xplay::m_print()
{
	const char *interp_txt[] = {"off","4-point","linear"};
	// print all current settings
	post("%s - current settings:",thisName());
	post("bufname = '%s', length = %.3f, channels = %i",buf.Name(),(float)(buf.Frames()*s2u),buf.Channels()); 
	post("out channels = %i, samples/unit = %.3f, interpolation = %s",outchns,(float)(1./s2u),interp_txt[interp >= xsi_none && interp <= xsi_lin?interp:xsi_none]); 
	post("");
}
