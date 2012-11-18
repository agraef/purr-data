/* RTcmix - Copyright (C) 2004  The RTcmix Development Team
   See ``AUTHORS'' for a list of contributors. See ``LICENSE'' for
   the license to this software and for a DISCLAIMER OF ALL WARRANTIES.
*/
#ifndef _OOSCIL_H_
#define _OOSCIL_H_ 1

// Simple non-interpolating oscillator.  Does not handle negative frequencies!
//
// NB: For the convenience of callers (like LFOPField) who need to decide
// at runtime whether to interpolate, this class provides an interpolating
// lookup method, nexti.  -JGG

class Ooscil
{
	double _si, _phase, _lendivSR;
	float *_array;
	int _length;

public:
	Ooscil(float srate, float freq, float array[], int len);

	float next();		// non-interpolating
	float nexti();		// linear interpolating

	void setsrate(float srate);	// NB: caller must call setfreq after this
	inline void setfreq(float freq) { _si = freq * _lendivSR; }
	inline void setphase(double phase) { _phase = phase; }
	inline double getphase() const { return _phase; }
	inline int getlength() const { return _length; }
};

#endif // _OOSCIL_H_
