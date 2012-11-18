/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"


class burrow:
	public fftease
{
	FLEXT_HEADER_S(burrow,fftease,setup)
	
public:
	burrow(I argc,const t_atom *argv);

protected:

	virtual V Transform(I _N2,S *const *in);

    BL _invert;

    F _threshold,_multiplier;
    F _thresh_dB,_mult_dB;

private:
	V ms_thresh(F v) { _threshold = FromdB(_thresh_dB = v); }
	V ms_mult(F v) { _multiplier = FromdB(_mult_dB = v); }


	static V setup(t_classid c);

	FLEXT_ATTRGET_F(_thresh_dB)
	FLEXT_CALLSET_F(ms_thresh)
	FLEXT_ATTRGET_F(_mult_dB)
	FLEXT_CALLSET_F(ms_mult)
	FLEXT_ATTRVAR_B(_invert)
};

FLEXT_LIB_DSP_V("burrow~",burrow)


V burrow::setup(t_classid c)
{
	FLEXT_CADDATTR_VAR(c,"thresh",_thresh_dB,ms_thresh);
	FLEXT_CADDATTR_VAR(c,"mult",_mult_dB,ms_mult);
	FLEXT_CADDATTR_VAR1(c,"invert",_invert);
}


burrow::burrow(I argc,const t_atom *argv):
	fftease(4,F_STEREO|F_BALANCED|F_BITSHUFFLE|F_NOPH2),
	_invert(false),
	_thresh_dB(-30),_mult_dB(-18)
{
	/* parse and set object's options given */
	if(argc >= 1) {
		if(CanbeFloat(argv[0]))
			_thresh_dB = GetAFloat(argv[0]);
		else
			post("%s - Threshold must be a float value - set to %0f",thisName(),_thresh_dB);
	}
	if(argc >= 2) {
		if(CanbeFloat(argv[1]))
			_mult_dB = GetAFloat(argv[1]);
		else
			post("%s - Multiplier must be a float value - set to %0f",thisName(),_mult_dB);
	}
	if(argc >= 3) {
		if(CanbeBool(argv[2]))
			_invert = GetABool(argv[2]);
		else
			post("%s - Invert flag must be a boolean value - set to %i",thisName(),_invert?1:0);
	}

	ms_thresh(_thresh_dB);
	ms_mult(_mult_dB);

	AddInSignal("Messages and input signal");
	AddInSignal("Reference signal");
	AddOutSignal("Transformed signal");
}


V burrow::Transform(I _N,S *const *)
{
	register const F thr = _threshold,mul = _multiplier;

	// use simple threshold from second signal to trigger filtering 
	// transform does not need phase of signal 2 

	if(_invert)
		for (I i = 0; i <= _N; i += 2) 
			if(_channel2[i] < thr) _channel1[i] *= mul;
	else
		for (I i = 0; i <= _N; i += 2)
			if(_channel2[i] > thr) _channel1[i] *= mul;
}



