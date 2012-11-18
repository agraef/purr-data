/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include <stdlib.h>

class thresher:
	public fftease
{
	FLEXT_HEADER_S(thresher,fftease,setup)
	
public:
	thresher();

protected:
	virtual V Set();
	virtual V Clear();
	virtual V Delete();

	virtual V Transform(I n,S *const *in);

	F *_compositeFrame;
	I *_framesLeft;
	BL _firstFrame;

	F _moveThreshold;
	I _maxHoldFrames;

private:
	static V setup(t_classid c);
};

FLEXT_LIB_DSP("thresher~ nacho~",thresher)


V thresher::setup(t_classid )
{
}

void thresher::Set()
{
	fftease::Set();

	const F rate = Samplerate();
	const I n = Blocksize(),_N = get_N(),_N2 = _N/2; 

	_compositeFrame = new F[_N+2];
	_framesLeft = new I[_N2+1];

	_firstFrame = true;
	_moveThreshold = .00001 ;
	F _maxHoldTime = 5.0 ;
	_maxHoldFrames = (I)(_maxHoldTime *rate/n);
}

void thresher::Clear()
{
	fftease::Clear();
	_compositeFrame = NULL;
	_framesLeft = NULL;
}

void thresher::Delete()
{
	fftease::Delete();
	if(_compositeFrame) delete[] _compositeFrame;
	if(_framesLeft) delete[] _framesLeft;
}


thresher::thresher():
	fftease(4,F_BALANCED|F_BITSHUFFLE|F_PHCONV)
{
	AddInSignal("Messages and input signal");
	AddOutSignal("Transformed signal");
}


V thresher::Transform(I _N,S *const *)
{
	I *fr = _framesLeft;
	if( _firstFrame ) {
		for (I i = 0; i <= _N; i += 2,++fr ){
			_compositeFrame[i] = _channel1[i];
			_compositeFrame[i+1] = _channel1[i+1];
			*fr = _maxHoldFrames;
		}
		_firstFrame = false;
	}
	else {
		for(I i = 0; i <= _N; i += 2,++fr ){
			if( (fabs( _compositeFrame[i] - _channel1[i] ) > _moveThreshold) || (*fr <= 0) ) {
				_compositeFrame[i] = _channel1[i];
				_compositeFrame[i+1] = _channel1[i+1];
				*fr = _maxHoldFrames;
			}
			else {
				_channel1[i] = _compositeFrame[i];
				_channel1[i+1] = _compositeFrame[i+1];
				--*fr;
			}
		}
	}
}
