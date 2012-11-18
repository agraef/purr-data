/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include <stdlib.h>

class ether:
	public fftease
{
	FLEXT_HEADER_S(ether,fftease,setup)
	
public:
	ether(I argc,const t_atom *argv);

protected:

	virtual V Transform(I n,S *const *in);

	BL _qual,_invert;
	F _threshMult;

private:
	static V setup(t_classid c);


	FLEXT_ATTRVAR_B(_invert)
	FLEXT_ATTRVAR_F(_threshMult)
};

FLEXT_LIB_DSP_V("ether~",ether)


V ether::setup(t_classid c)
{
	FLEXT_CADDATTR_VAR1(c,"invert",_invert);
	FLEXT_CADDATTR_VAR1(c,"index",_threshMult);
}


ether::ether(I argc,const t_atom *argv):
	fftease(2,F_STEREO|F_BITSHUFFLE),
	_qual(false),_invert(false),_threshMult(0)
{
	/* parse and set object's options given */
	if(argc >= 1) {
		if(CanbeBool(argv[0]))
			_qual = GetABool(argv[0]);
		else
			post("%s - Quality must be a boolean value - set to %0i",thisName(),_qual?1:0);
	}

	Mult(_qual?4:2);
	if(_qual) _flags |= F_BALANCED;

	AddInSignal("Messages and input signal");
	AddInSignal("Reference signal");
	AddOutSignal("Transformed signal");
}


V ether::Transform(I _N,S *const *)
{
	const BL inv = _invert;
	const F threshMult = _threshMult?_threshMult:1;

	for (I i = 0; i <= _N; i += 2) {
		F &amp1 = _channel1[i];
		F &phase1 = _channel1[i+1];

		F &amp2 = _channel2[i];
		F &phase2 = _channel2[i+1];

		// use simple threshold for inverse compositing 

		if(inv?(amp1 > amp2*threshMult):(amp1 < amp2*threshMult) ) amp1 = amp2;
		if (phase1 == 0. ) phase1 = phase2;
	}
}

