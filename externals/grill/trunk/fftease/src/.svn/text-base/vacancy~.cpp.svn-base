/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include <stdlib.h>

class vacancy:
	public fftease
{
	FLEXT_HEADER_S(vacancy,fftease,setup)
	
public:
	vacancy(I argc,const t_atom *argv);

protected:
	virtual V Transform(I n,S *const *in);

	V ms_thresh(F thr) { _threshold = FromdB(_threshdB = thr); }
	V ms_userms(BL r) { if(r) _flags |= F_RMS; else _flags &= ~F_RMS; }

	F _threshold,_threshdB;
	BL _invert,_useRms,_swapPhase;

private:
	static V setup(t_classid c);

	FLEXT_ATTRGET_F(_threshdB)
	FLEXT_CALLSET_F(ms_thresh)
	FLEXT_ATTRVAR_B(_invert)
	FLEXT_ATTRGET_B(_useRms)
	FLEXT_CALLSET_B(ms_userms)
	FLEXT_ATTRVAR_B(_swapPhase)
};

FLEXT_LIB_DSP_V("vacancy~",vacancy)


V vacancy::setup(t_classid c)
{
	FLEXT_CADDATTR_VAR(c,"thresh",_threshdB,ms_thresh);
	FLEXT_CADDATTR_VAR1(c,"invert",_invert);
	FLEXT_CADDATTR_VAR(c,"rms",_useRms,ms_userms);
	FLEXT_CADDATTR_VAR1(c,"swap",_swapPhase);
}


vacancy::vacancy(I argc,const t_atom *argv):
	fftease(2,F_STEREO|F_BITSHUFFLE),
	_threshdB(-30),_invert(false),_useRms(true),_swapPhase(false)
{
	/* parse and set object's options given */
	if(argc >= 1) {
		if(CanbeFloat(argv[0]))
			_threshdB = GetAFloat(argv[0]);
		else
			post("%s - Threshold must be a float value - set to %f",thisName(),_threshdB);
	}

	if(argc >= 2) {
		if(CanbeBool(argv[1]))
			_invert = GetABool(argv[1]);
		else
			post("%s - Invert must be a boolean value - set to %0i",thisName(),_invert?1:0);
	}

	if(argc >= 3) {
		if(CanbeBool(argv[2]))
			_useRms = GetABool(argv[2]);
		else
			post("%s - Rms flag must be a boolean value - set to %0i",thisName(),_useRms?1:0);
	}

	if(argc >= 4) {
		if(CanbeBool(argv[3]))
			_swapPhase = GetABool(argv[3]);
		else
			post("%s - Swapphase flag must be a boolean value - set to %0i",thisName(),_swapPhase?1:0);
	}

	ms_thresh(_threshdB);
	ms_userms(_useRms);

	AddInSignal("Messages and input signal");
	AddInSignal("Reference signal");
	AddOutSignal("Transformed signal");
}


V vacancy::Transform(I _N,S *const *)
{
	const F useme = _useRms?_rms * _threshold:_threshold;

	// composite here please
	if (_invert) {
		if (_swapPhase) {
			for (I i=0; i < _N; i+=2 )
				if ( _channel1[i] > useme && _channel2[i] < _channel1[i] ) {
					_channel1[i] = _channel2[i];
					_channel1[i+1] = _channel2[i+1];
				}
		}
		else {
			for (I i=0; i < _N; i+=2 )
				if ( _channel1[i] > useme && _channel2[i] < _channel1[i] ) {
					_channel1[i] = _channel2[i];
					if ( _channel1[i+1] == 0. ) _channel1[i+1] = _channel2[i+1];
				}
		}
	}
	else {
		if (_swapPhase) {
			for (I i=0; i < _N; i+=2 )
				if ( _channel1[i] < useme && _channel2[i] > _channel1[i] ) {
					_channel1[i] = _channel2[i];
					_channel1[i+1] = _channel2[i+1];
				}
		}
		else {
			for (I i=0; i < _N; i+=2 )
				if ( _channel1[i] < useme && _channel2[i] > _channel1[i] ) {
					_channel1[i] = _channel2[i];
					if ( _channel1[i+1] == 0. ) _channel1[i+1] = _channel2[i+1];
				}
		}
	}
}
