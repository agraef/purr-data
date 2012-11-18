/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include <stdlib.h>

class disarray:
	public fftease
{
	FLEXT_HEADER_S(disarray,fftease,setup)
	
public:
	disarray(I argc,const t_atom *argv);

protected:

	virtual V Transform(I _N2,S *const *in);

	BL _qual;
	I _shuffle_count,_max_bin;
	F _freq;
	I *_shuffle_in,*_shuffle_out;

	V reset_shuffle();

private:
	virtual V Set();
	virtual V Clear();
	virtual V Delete();

	V ms_freq(F f);

	static V setup(t_classid c);

	FLEXT_CALLBACK(reset_shuffle)
	FLEXT_ATTRGET_F(_freq)
	FLEXT_CALLSET_F(ms_freq)
	FLEXT_ATTRVAR_I(_shuffle_count)
};

FLEXT_LIB_DSP_V("disarray~",disarray)


V disarray::setup(t_classid c)
{
	FLEXT_CADDBANG(c,0,reset_shuffle);

	FLEXT_CADDATTR_VAR(c,"knee",_freq,ms_freq);
	FLEXT_CADDATTR_VAR1(c,"partials",_shuffle_count);
}


disarray::disarray(I argc,const t_atom *argv):
	fftease(2,F_BITSHUFFLE),
	_qual(false),_shuffle_count(20),_max_bin(0),_freq(1300)
{
	/* parse and set object's options given */
	if(argc >= 1) {
		if(CanbeFloat(argv[0]))
			_freq = GetAFloat(argv[0]);
		else
			post("%s - Freq must be a float value - set to %0f",thisName(),_freq);
	}
	if(argc >= 2) {
		if(CanbeBool(argv[1]))
			_qual = GetABool(argv[1]);
		else
			post("%s - Quality must be a boolean value - set to %0i",thisName(),_qual?1:0);
	}
	if(argc >= 3) {
		if(CanbeInt(argv[2]))
			_shuffle_count = GetAInt(argv[2]);
		else
			post("%s - Partials must be an integer value - set to %0i",thisName(),_shuffle_count);
	}

	Mult(_qual?4:2);
	if(_qual) _flags |= F_BALANCED;

	AddInSignal("Messages and input signal");
	AddOutSignal("Transformed signal");
}


V disarray::Clear()
{
	_shuffle_in = _shuffle_out = NULL;
	fftease::Clear();
}

V disarray::Delete() 
{
	fftease::Delete();
	if(_shuffle_in) delete[] _shuffle_in;
	if(_shuffle_out) delete[] _shuffle_out;
}

V disarray::Set() 
{
	fftease::Set();

	const I _N2 = get_N()/2;

	_shuffle_in = new I[_N2];
	_shuffle_out = new I[_N2];

	// calculate _max_bin
	ms_freq(_freq);
}

V disarray::ms_freq(F f)
{
	_freq = f; // store original

	const F funda = get_Fund();

	// TG: This is a different, but steady correction than in original fftease
	if( f < funda ) f = funda;
	else if(f > Samplerate()/2) f = Samplerate()/2;

	_max_bin = (I)(f/funda+0.5);

	reset_shuffle();
}

inline V swap(F &a,F &b) { F t = a; a = b; b = t; }
inline V swap(I &a,I &b) { I t = a; a = b; b = t; }

V disarray::Transform(I _N,S *const *)
{
	I shcnt = _shuffle_count;
	if(shcnt < 0) shcnt = 0;
	else if(shcnt > _N/2) shcnt = _N/2;

	for(I i = 0; i < shcnt; i++)
		// leave phase, just swap amplitudes
		swap(_channel1[ _shuffle_in[i] * 2 ],_channel1[ _shuffle_out[i] * 2]);
}


V disarray::reset_shuffle()
{
	const I _N2 = get_N()/2;

	I i;
	for( i = 0; i < _N2; i++ )
		_shuffle_out[i] = _shuffle_in[i] = i ;

	for( i = 0; i < _max_bin*2; i++ ) {
		I p1 = _shuffle_out[ rand()%_max_bin ];
		I p2 = _shuffle_out[ rand()%_max_bin ];
		swap(_shuffle_out[ p1 ],_shuffle_out[ p2 ]);
	}

}

