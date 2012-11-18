/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include <stdlib.h>

class scrape:
	public fftease
{
	FLEXT_HEADER_S(scrape,fftease,setup)
	
public:
	scrape(I argc,const t_atom *argv);

protected:

	virtual V Transform(I n,S *const *in);

	F _thresh1,_thresh2;
	F _knee,_cutoff;
	F *_threshfunc;

	V UpdThrFun();

private:
	virtual V Set();
	virtual V Clear();
	virtual V Delete();

	static V setup(t_classid c);

	inline V ms_knee(F knee) { _knee = knee; UpdThrFun(); }
	inline V ms_cutoff(F cutoff) { _cutoff = cutoff; UpdThrFun(); }
	inline V ms_thresh1(F thr) { _thresh1 = thr; UpdThrFun(); }
	inline V ms_thresh2(F thr) { _thresh2 = thr; UpdThrFun(); }

	FLEXT_ATTRGET_F(_knee)
	FLEXT_CALLSET_F(ms_knee)
	FLEXT_ATTRGET_F(_cutoff)
	FLEXT_CALLSET_F(ms_cutoff)
	FLEXT_ATTRGET_F(_thresh1)
	FLEXT_CALLSET_F(ms_thresh1)
	FLEXT_ATTRGET_F(_thresh2)
	FLEXT_CALLSET_F(ms_thresh2)
};

FLEXT_LIB_DSP_V("scrape~",scrape)


V scrape::setup(t_classid c)
{
	FLEXT_CADDATTR_VAR(c,"knee",_knee,ms_knee);
	FLEXT_CADDATTR_VAR(c,"cutoff",_cutoff,ms_cutoff);
	FLEXT_CADDATTR_VAR(c,"thresh1",_thresh1,ms_thresh1);
	FLEXT_CADDATTR_VAR(c,"thresh2",_thresh2,ms_thresh2);
}


scrape::scrape(I argc,const t_atom *argv):
	fftease(4,F_BALANCED|F_BITSHUFFLE),
	_thresh1(.0001),_thresh2(.09),
	_knee(1000),_cutoff(4000)

{
	/* parse and set object's options given */
	if(argc >= 1) {
		if(CanbeFloat(argv[0]))
			_knee = GetAFloat(argv[0]);
		else
			post("%s - Knee must be a float value - set to %f",thisName(),_knee);
	}
	if(argc >= 2) {
		if(CanbeFloat(argv[1])) {
			F c = GetAFloat(argv[1]);
			if(c > 0) _cutoff = c;
			else
				post("%s - Cutoff must be > 0 - set to %f",thisName(),_cutoff);
		}
		else
			post("%s - Cutoff must be a float value - set to %f",thisName(),_cutoff);
	}

	AddInSignal("Messages and input signal");
	AddInSignal("Multiplier signal");
	AddOutSignal("Transformed signal");
}

V scrape::Clear()
{
	_threshfunc = NULL;
	fftease::Clear();
}

V scrape::Delete() 
{
	fftease::Delete();
	if(_threshfunc) delete[] _threshfunc;
}

V scrape::Set() 
{
	fftease::Set();

	_threshfunc = new F[get_N()/2];
	UpdThrFun();
}

V scrape::UpdThrFun() 
{
	const I _N2 = get_N()/2; 
	const F funda = get_Fund();

	F curfreq = funda;
	for(I i = 0; i < _N2; i++ ) {
		if( curfreq  < _knee )
			_threshfunc[i] = 0;
		else if( curfreq >= _knee && curfreq < _cutoff ) {
			const F m = (_knee - curfreq) / (_cutoff - _knee) ;
			_threshfunc[i] = (1-m) * _thresh1 + m * _thresh2 ;
		} else
			_threshfunc[i] = _thresh2;

		curfreq += funda ;
	}
}

/*! \brief Do the spectral transformation

	\remark maxamp is calculated later than in the original FFTease scrape~ object
*/
V scrape::Transform(I _N,S *const *in)
{
	const F fmult = *in[0];
	const F *thrf = _threshfunc;

	I i;
	F maxamp = 1.;
	for( i = 0; i <= _N; i += 2 )
		if(maxamp < _channel1[i]) 
			maxamp = _channel1[i];

	for( i = 0; i <= _N; i += 2 )
		if(_channel1[i] < *(thrf++) * maxamp) 
			_channel1[i] *= fmult;
}

