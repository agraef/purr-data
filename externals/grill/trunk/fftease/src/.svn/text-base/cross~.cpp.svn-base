/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"


class cross:
	public fftease
{
	FLEXT_HEADER_S(cross,fftease,setup)
	
public:
	cross();

protected:

	F *amps;
	BL memory;

	virtual V Set();
	virtual V Clear();
	virtual V Delete();

	virtual V Transform(I _N2,S *const *in);

private:
	static V setup(t_classid c);

	FLEXT_ATTRVAR_B(memory)
};

FLEXT_LIB_DSP("cross~",cross)


V cross::setup(t_classid c)
{
	FLEXT_CADDATTR_VAR1(c,"memorize",memory);
}

cross::cross():
	fftease(2,F_STEREO|F_BALANCED|F_BITSHUFFLE|F_NOPH2),
	memory(true)
{
	AddInSignal("Messages and driver signal");
	AddInSignal("Filter signal");
	AddInSignal("Threshold signal for cross synthesis");
	AddOutSignal("Transformed signal");
}

V cross::Clear()
{
	amps = NULL;
	fftease::Clear();
}

V cross::Delete() 
{
	fftease::Delete();
	if(amps) delete[] amps;
}

V cross::Set() 
{
	fftease::Set();
	const I _N2 = get_N()/2;
	amps = new F[_N2];
	ZeroMem(amps,_N2*sizeof(*amps));
}

V cross::Transform(I _N,S *const *in)
{
	// filled only once per signal vector!!
	register const F threshie = *in[0];
	F *amp = amps;

	for (I i = 0; i <= _N; i += 2,amp++)
		if( _channel2[i] > threshie )
			*amp = _channel1[i] *= _channel2[i];
		else if(memory)
			// retrieve previous value 
			_channel1[i] = *amp; 

}

