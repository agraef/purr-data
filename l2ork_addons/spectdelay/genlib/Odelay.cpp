/* RTcmix - Copyright (C) 2004  The RTcmix Development Team
   See ``AUTHORS'' for a list of contributors. See ``LICENSE'' for
   the license to this software and for a DISCLAIMER OF ALL WARRANTIES.
*/

#include "Odelay.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

Odelay::Odelay(long defaultLength) : _dline(NULL), _len(0)
{
	if (defaultLength > 0) {
		_outpoint = 0;
		resize(defaultLength);
		clear();
		_inpoint = _len - 1;
	}
	else {
		printf("****program error: Odelay defaultLength not > 0\n");
		assert(defaultLength > 0); // doesn't print anything in Max/MSP or console
	}
}

Odelay::~Odelay()
{
	free(_dline);
}

void Odelay::clear()
{
	for (long i = 0; i < _len; i++)
		_dline[i] = 0.0;
	_lastout = 0.0;
}

void Odelay::fill(double val)
{
	for (long i = 0; i < _len; i++)
		_dline[i] = val;
	_lastout = val;
}

void Odelay::putsamp(float samp)
{
	_dline[_inpoint++] = samp;
	if (_inpoint == _len)
		_inpoint = 0;
}

float Odelay::getsamp(double lagsamps)
{
	_outpoint = _inpoint - (long) lagsamps;
#ifdef AUTOEXPAND
	if (lagsamps >= (double) _len)
		resize((long)(lagsamps + 0.5));
#endif
	if (_len > 0) {
		while (_outpoint < 0)
			_outpoint += _len;
	}
	return _lastout = _dline[_outpoint++];
}

// Set output pointer <_outpoint>.

void Odelay::setdelay(double lagsamps)
{
	_outpoint = _inpoint - (long) lagsamps;
#ifdef AUTOEXPAND
	if (lagsamps >= (double) _len)
		resize((long)(lagsamps + 0.5));
#endif
	if (_len > 0) {
		while (_outpoint < 0)
			_outpoint += _len;
	}
}

float Odelay::next(float input)
{
	_dline[_inpoint++] = input;
	if (_inpoint == _len)
		_inpoint = 0;
	_lastout = _dline[_outpoint++];
	if (_outpoint == _len)
		_outpoint = 0;
	return _lastout;
}

static float *newFloats(float *oldptr, long oldlen, long *newlen)
{
	float *ptr = NULL;
	if (oldptr == NULL) {
		ptr = (float *) malloc(*newlen * sizeof(float));
	}
	else {
		float *newptr = (float *) realloc(oldptr, *newlen * sizeof(float));
		if (newptr) {
			ptr = newptr;
			// Zero out new portion.
			for (long n = oldlen; n < *newlen; ++n)
				ptr[n] = 0.0f;
		}
		else {
			*newlen = oldlen;	// notify caller that realloc failed
			ptr = oldptr;
		}
	}
	return ptr;
}

int Odelay::resize(long thisLength)
{
	const long oldlen = _len;

	if (thisLength == oldlen)
		return oldlen;

// For Max/MSP we rely on object to manage maxdeltime and don't take advantage
// of Doug's code to automatically resize delay lines.
#ifdef AUTOEXPAND
	// Make a guess at how big the new array should be.
	long newlen = (thisLength < _len * 2) ? _len * 2 : _len + thisLength;
#else
	long newlen = thisLength;
#endif
	_dline = ::newFloats(_dline, _len, &newlen);
	if (_outpoint < 0 && newlen > 0) {
		while (_outpoint < 0)
			_outpoint += oldlen;
		long shift = newlen - oldlen;
		memcpy(&_dline[_outpoint + shift],
				&_dline[_outpoint],
				sizeof(float) * (oldlen - _outpoint));
		_outpoint += shift;
	}
	return _len = newlen;
}

float Odelay::delay() const
{
	return (float) (_len > 0) ? abs((_outpoint - _inpoint) % _len) : 0;
}
