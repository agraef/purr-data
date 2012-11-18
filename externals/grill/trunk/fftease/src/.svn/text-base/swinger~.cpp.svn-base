/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include <stdlib.h>


// add quality switch

class swinger:
	public fftease
{
	FLEXT_HEADER(swinger,fftease)
	
public:
	swinger(I argc,const t_atom *argv);

protected:

	BL _qual;

	virtual V Transform(I n,S *const *in);
};

FLEXT_LIB_DSP_V("swinger~",swinger)


swinger::swinger(I argc,const t_atom *argv):
	fftease(2,F_STEREO|F_BITSHUFFLE|F_NOPH1|F_NOAMP2),
	_qual(false)
{
	/* parse and set object's options given */
	if(argc >= 1) {
		if(CanbeBool(argv[0]))
			_qual = GetABool(argv[0]);
		else
			post("%s - Quality must be a boolean value - set to %0i",thisName(),_qual?1:0);
	}

	if(_qual) {
		Mult(4);
		_flags |= F_BALANCED;
	}
	else
		Mult(2);

	AddInSignal("Messages and input signal");
	AddInSignal("Signal to supply phase information");
	AddOutSignal("Transformed signal");
}


V swinger::Transform(I _N,S *const *)
{
	for (I i = 0; i <= _N; i += 2) _channel1[i+1] = _channel2[i+1];
}
