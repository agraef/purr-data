/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include <stdlib.h>

class dentist:
	public fftease
{
	FLEXT_HEADER_S(dentist,fftease,setup)
	
public:
	dentist(I argc,const t_atom *argv);

protected:

	virtual V Transform(I _N2,S *const *in);

	BL *_bin_selection;
	I _teeth;  
	F _knee;
	I _max_bin; // determined by _knee and fundamental frequency

	V reset_shuffle();

private:

	virtual V Set();
	virtual V Clear();
	virtual V Delete();

	V ms_knee(F knee);
	V ms_teeth(I teeth) { _teeth = teeth;	reset_shuffle(); }

	
	static V setup(t_classid c);

	FLEXT_CALLBACK(reset_shuffle)
	FLEXT_ATTRGET_F(_knee)
	FLEXT_CALLSET_F(ms_knee)
	FLEXT_ATTRGET_I(_teeth)
	FLEXT_CALLSET_I(ms_teeth)
};

FLEXT_LIB_DSP_V("dentist~",dentist)


V dentist::setup(t_classid c)
{
	FLEXT_CADDBANG(c,0,reset_shuffle);

	FLEXT_CADDATTR_VAR(c,"knee",_knee,ms_knee);
	FLEXT_CADDATTR_VAR(c,"teeth",_teeth,ms_teeth);
}


dentist::dentist(I argc,const t_atom *argv):
	fftease(4,F_BALANCED|F_BITSHUFFLE),	
	_teeth(10),_knee(500),_max_bin(0)
{
	/* parse and set object's options given */
	if(argc >= 1) {
		if(CanbeFloat(argv[0]))
			_knee = GetAFloat(argv[0]);
		else
			post("%s - Knee must be a float value - set to %0f",thisName(),_knee);
	}
	if(argc >= 2) {
		if(CanbeInt(argv[1]))
			_teeth = GetAInt(argv[1]);
		else
			post("%s - Teeth must be an integer value - set to %0i",thisName(),_teeth);
	}

	AddInSignal("Messages and input signal");
	AddOutSignal("Transformed signal");
}

V dentist::Clear()
{
	_bin_selection = NULL;
	fftease::Clear();
}

V dentist::Delete() 
{
	fftease::Delete();
	if(_bin_selection) delete[] _bin_selection;
}


V dentist::ms_knee(F f)
{
	_knee = f;	// store original

	const F funda = get_Fund();

	// TG: This is a different, but steady correction than in original fftease
	if( f < funda ) f = funda;
	else if(f > Samplerate()/2) f = Samplerate()/2;

	_max_bin = (I)(f/funda+0.5);

	reset_shuffle();
}


V dentist::Set()
{
	fftease::Set();
	
	_bin_selection = new BL[get_N()/2];

	// calculation of _max_bin
	ms_knee(_knee); 
}

V dentist::Transform(I _N,S *const *)
{
	const BL *bs = _bin_selection;
	for(I i = 0; i < _N ; i += 2)
		if(!*(bs++)) _channel1[i] = 0;
}


V dentist::reset_shuffle()
{
	const I _N2 = get_N()/2;
	I t = _teeth;

	// check number of teeth
	if( t < 0 ) t = 0;
	else if( t > _N2 ) t = _N2;

	// clear and set random bins
	I i;
	for( i = 0; i < _N2; i++ )
		_bin_selection[i] = false;
	for( i = 0; i < t; i++ )
		_bin_selection[rand()%_max_bin] = true;
}
