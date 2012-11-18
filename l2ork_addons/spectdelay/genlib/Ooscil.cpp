/* RTcmix - Copyright (C) 2004  The RTcmix Development Team
   See ``AUTHORS'' for a list of contributors. See ``LICENSE'' for
   the license to this software and for a DISCLAIMER OF ALL WARRANTIES.
*/
#include "Ooscil.h"

Ooscil::Ooscil(float srate, float freq, float array[], int len)
	: _array(array), _length(len)
{
	_lendivSR = (double) _length / srate;
	_si = freq * _lendivSR;
	_phase = 0.0;
}

// NB: caller must call setfreq after this
void Ooscil::setsrate(float srate)
{
	_lendivSR = (double) _length / srate;
}

float Ooscil::next()
{
	int i = (int) _phase;
	float output = _array[i];

	// prepare for next call
	_phase += _si;
	while (_phase >= (double) _length)
		_phase -= (double) _length;

	return output;
}

float Ooscil::nexti()
{
	int i = (int) _phase;
	int k = (i + 1) % _length;
	double frac = _phase - (double) i;
	float output = _array[i] + ((_array[k] - _array[i]) * frac);

	// prepare for next call
	_phase += _si;
	while (_phase >= (double) _length)
		_phase -= (double) _length;

	return output;
}

