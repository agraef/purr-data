/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include <stdlib.h>

class morphine:
	public fftease
{
	FLEXT_HEADER_S(morphine,fftease,setup)
	
public:
	morphine(I argc,const t_atom *argv);

protected:

	virtual V Transform(I n,S *const *in);

	F _index;

	struct pickme { I bin; F value; };
	static I sortpicks( const V *a, const V *b );

    pickme *_picks;

private:
	virtual V Set();
	virtual V Clear();
	virtual V Delete();

	static V setup(t_classid c);

	FLEXT_ATTRVAR_F(_index)
};

FLEXT_LIB_DSP_V("morphine~",morphine)


V morphine::setup(t_classid c)
{
	FLEXT_CADDATTR_VAR1(c,"index",_index);
}


morphine::morphine(I argc,const t_atom *argv):
	fftease(4,F_STEREO|F_BALANCED|F_BITSHUFFLE),
	_index(0)
{
	/* parse and set object's options given */
	if(argc >= 1) {
		if(CanbeFloat(argv[0]))
			_index = GetAFloat(argv[0]);
		else
			post("%s - Index must be a float value - set to %f",thisName(),_index);
	}

	AddInSignal("Messages and input signal 1");
	AddInSignal("Input signal 2");
	AddOutSignal("Transformed signal");
}

V morphine::Clear()
{
	_picks = NULL;
	fftease::Clear();
}

V morphine::Delete() 
{
	fftease::Delete();
	if(_picks) delete[] _picks;
}

V morphine::Set() 
{
	fftease::Set();

	_picks = new pickme[get_N()/2+1];
}

I morphine::sortpicks( const void *a, const void *b )
{
	if ( ((pickme *)a)->value > ((pickme *) b)->value ) return 1;
	if ( ((pickme *) a)->value < ((pickme *) b)->value ) return -1;
	return 0;
}

V morphine::Transform(I _N,S *const *)
{
	const I _N2 = _N/2;
	I i;
    for ( i = 0; i <= _N2; i++ ) {
		// find amplitude differences between home and visitors 
		_picks[i].value = fabs( _channel1[i*2] - _channel2[i*2]);
		_picks[i].bin = i;
    }

    // sort our differences in ascending order 
    qsort( _picks, _N2+1, sizeof(pickme), sortpicks );

	I ix2 = (I)(_index*(_N2+1)+.5)*2;
	if(ix2 < 0) ix2 = 0; 
	else if(ix2 > _N+2) ix2 = _N+2;

    // choose the bins that are least different first 
    for (i=0; i < ix2; i += 2) {
		_channel1[i] = _channel2[i];
		_channel1[i+1] = _channel2[i+1];
    }
}

