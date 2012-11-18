// RTcmix - Copyright (C) 2005  The RTcmix Development Team
// See ``AUTHORS'' for a list of contributors. See ``LICENSE'' for
// the license to this software and for a DISCLAIMER OF ALL WARRANTIES.

// Obucket maintains a private buffer of floats.  Add to the buffer one float
// at a time.  When Obucket sees that the buffer is full, it invokes a callback
// function with the buffer, its length and a user-supplied context.  This
// is useful for instruments that must process in blocks whose size has no
// clear and predictable relationship to the run() buffer size (for example,
// instruments that do FFT-based processing).
//
// JGG, 6/1/05

class Obucket {
public:
	typedef void (*ProcessFunction)(const float *buf, const int len,
	                                                  void *context);
	Obucket(int len, ProcessFunction callback, void *context);
	~Obucket();
	inline bool drop(float item);		// call this to drop <item> in bucket
	void flush(float defaultval = 0.0f);
	void clear(float defaultval = 0.0f);

private:
	int _len;
	int _index;
	ProcessFunction _callback;
	void *_context;
	float *_buf;
};


// Store <item> into the buffer; if full, invoke callback and return true;
// otherwise return false.

inline bool Obucket::drop(float item)
{
	_buf[_index++] = item;
	if (_index == _len) {
		(*_callback)(_buf, _len, _context);
		_index = 0;
		return true;
	}
	return false;
}

