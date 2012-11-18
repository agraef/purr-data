/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include <stdlib.h>

class xsyn:
	public fftease
{
	FLEXT_HEADER(xsyn,fftease)
	
public:
	xsyn();

protected:
	virtual V Transform(I n,S *const *in);
};

FLEXT_LIB_DSP("xsyn~",xsyn)


xsyn::xsyn():
	fftease(2,F_STEREO|F_BITSHUFFLE|F_NOPH2)
{
	AddInSignal("Messages and source signal 1");
	AddInSignal("Source signal 2");
	AddOutSignal("Transformed signal");
}


V xsyn::Transform(I _N,S *const *)
{
    F maxamp = 0;
	I i;
    for( i = 0; i <= _N; i+= 2 )
		if(_channel2[i] > maxamp ) maxamp = _channel2[i];

	const F f = maxamp?1./maxamp:1.;

    for( i = 0; i <= _N; i+= 2 )
		_channel1[i] *= (_channel2[i] * f);
}
