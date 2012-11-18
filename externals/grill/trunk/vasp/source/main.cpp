/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"


const C *VASP_VERSION = "0.1.4pre";

#include "opfuns.h"

static V vasp_main()
{
	post("");
	post("-----------------------------------------");
	post("           VASP modular %s            ",VASP_VERSION);
	post("   vector assembling signal processor    ");
	post("       (C)2002-2009 Thomas Grill         ");
#ifdef FLEXT_DEBUG
	post("   DEBUG BUILD - " __DATE__ " " __TIME__);
#endif
	post("");
	post("          http://grrrr.org/ext           ");
	post("-----------------------------------------");
	post("");

	// call the objects' setup routines

	VASP_SETUP(v); // vasp
	VASP_SETUP(multi); // vasp.m

	VASP_SETUP(check);  // vasp.check
	VASP_SETUP(update);  // vasp.update
//	VASP_SETUP(post);  // vasp.post

	VASP_SETUP(sync);  // vasp.sync
	VASP_SETUP(radio);  // vasp.radio

	VASP_SETUP(vector); // vasp.vector
	VASP_SETUP(qvectors); // vasp.vectors?

	VASP_SETUP(size);  // vasp.size 
	VASP_SETUP(dsize);  // vasp.size+ 
	VASP_SETUP(qsize);  // vasp.size?
	VASP_SETUP(msize);  // vasp.size*
	VASP_SETUP(rsize);  // vasp.size/

	VASP_SETUP(offset);  // vasp.offset
	VASP_SETUP(doffset); // vasp.offset+
	VASP_SETUP(qoffset);  // vasp.offset?

	VASP_SETUP(frames);  // vasp.frames
	VASP_SETUP(dframes);  // vasp.frames+
	VASP_SETUP(qframes);  // vasp.frames?
	VASP_SETUP(mframes);  // vasp.frames*
	VASP_SETUP(rframes);  // vasp.frames/

	VASP_SETUP(channel);  // vasp.channel
	VASP_SETUP(qchannel);  // vasp.channel?
	VASP_SETUP(qchannels);  // vasp.channels?

	VASP_SETUP(split);
	VASP_SETUP(join);
	VASP_SETUP(spit);
	VASP_SETUP(gather);
	VASP_SETUP(part);

	VASP_SETUP(list);
	VASP_SETUP(nonzero);
 
	VASP_SETUP(imm); // vasp.imm 

	VASP__SETUP(set);
	VASP__SETUP(cset);
	VASP_SETUP(copy);
	VASP_SETUP(ccopy);

	VASP__SETUP(add);
	VASP__SETUP(cadd);
	VASP__SETUP(sub);
	VASP__SETUP(csub);
	VASP__SETUP(subr);
	VASP__SETUP(csubr);
	VASP__SETUP(mul);
	VASP__SETUP(cmul);
	VASP__SETUP(div);
	VASP__SETUP(cdiv);
	VASP__SETUP(divr);
	VASP__SETUP(cdivr);
	VASP__SETUP(mod);

	VASP__SETUP(sign) 
	VASP__SETUP(abs) 
	VASP__SETUP(cabs) 

	VASP_SETUP(qsum)

	VASP__SETUP(lwr)
	VASP__SETUP(gtr)
	VASP__SETUP(alwr)
	VASP__SETUP(agtr)
	VASP__SETUP(leq)
	VASP__SETUP(geq)
	VASP__SETUP(aleq)
	VASP__SETUP(ageq)
	VASP__SETUP(equ)
	VASP__SETUP(neq)

	VASP__SETUP(min)
	VASP__SETUP(rmin)
	VASP__SETUP(max)
	VASP__SETUP(rmax)

	VASP__SETUP(minmax) 

	VASP_SETUP(qmin)
	VASP_SETUP(qmax)
	VASP_SETUP(qamin)
	VASP_SETUP(qamax)
	VASP_SETUP(qrmin)
	VASP_SETUP(qrmax)

	VASP__SETUP(gate);
	VASP__SETUP(rgate);
//	VASP__SETUP(igate);
//	VASP__SETUP(rigate);

	VASP_SETUP(peaks) 
	VASP_SETUP(valleys) 
	VASP_SETUP(rpeaks) 
	VASP_SETUP(rvalleys) 

	VASP_SETUP(qpeaks);

/*
	VASP_SETUP(qvalleys);
	VASP_SETUP(qrpeaks);
	VASP_SETUP(qrvalleys);
*/

	VASP__SETUP(sqr) 
	VASP__SETUP(ssqr) 
	VASP__SETUP(csqr) 
	VASP__SETUP(sqrt) 
	VASP__SETUP(ssqrt) 
	VASP__SETUP(pow) 
	VASP__SETUP(cpowi) 
	VASP__SETUP(rpow);
	VASP__SETUP(radd);

	VASP__SETUP(exp) 
	VASP__SETUP(log) 

	VASP__SETUP(polar) 
	VASP__SETUP(rect) 

	VASP__SETUP(cnorm)
//	VASP__SETUP(cswap)
//	VASP__SETUP(cconj)

	VASP_SETUP(shift)
	VASP_SETUP(xshift)
	VASP__SETUP(rot)
	VASP__SETUP(xrot)
	VASP__SETUP(mirr)
	VASP__SETUP(xmirr)

	VASP__SETUP(osc)
	VASP__SETUP(mosc)
	VASP__SETUP(cosc)
	VASP__SETUP(mcosc)
	VASP__SETUP(phasor)
	VASP__SETUP(mphasor)
	VASP__SETUP(noise)
	VASP__SETUP(cnoise)

	VASP__SETUP(window)
	VASP__SETUP(mwindow)
	VASP__SETUP(iwindow)
	VASP__SETUP(miwindow)
	VASP__SETUP(xwindow)
	VASP__SETUP(mxwindow)

	VASP__SETUP(flp)
	VASP__SETUP(fhp)

	VASP__SETUP(int) 
	VASP__SETUP(dif) 

	VASP__SETUP(fix) 

	VASP__SETUP(tilt)
	VASP__SETUP(xtilt)

	VASP__SETUP(soffset)
	VASP__SETUP(sframes)

	VASP__SETUP(rfft)
	VASP__SETUP(rifft)
	VASP__SETUP(cfft)
	VASP__SETUP(cifft)
}

FLEXT_LIB_SETUP(vasp,vasp_main)





