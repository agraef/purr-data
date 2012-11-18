// RTcmix - Copyright (C) 2005  The RTcmix Development Team
// See ``AUTHORS'' for a list of contributors. See ``LICENSE'' for
// the license to this software and for a DISCLAIMER OF ALL WARRANTIES.

#include "Obucket.h"

Obucket::Obucket(int len, ProcessFunction callback, void *context)
	: _len(len), _index(0), _callback(callback), _context(context)
{
	_buf = new float [len];
}

Obucket::~Obucket()
{
	delete [] _buf;
}

void Obucket::flush(float defaultval)
{
	if (_index > 0) {
		for (int i = _index; i < _len; i++)
			_buf[i] = defaultval;
		(*_callback)(_buf, _len, _context);
		_index = 0;
	}
}

void Obucket::clear(float defaultval)
{
	for (int i = 0; i < _len; i++)
		_buf[i] = defaultval;
}

