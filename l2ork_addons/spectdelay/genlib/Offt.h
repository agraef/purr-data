// RTcmix - Copyright (C) 2005  The RTcmix Development Team
// See ``AUTHORS'' for a list of contributors. See ``LICENSE'' for
// the license to this software and for a DISCLAIMER OF ALL WARRANTIES.

// Offt provides an interface to two different FFT libraries: FFTW v3
// <www.fftw.org> and FFTReal by Laurent de Soras <http://ldesoras.free.fr>.
// This is a compile time choice, made via the configure script.  By default
// you get FFTReal, unless you configure --with-fftw.  This code uses only
// the float version of the fftw library, libfftw3f.
//
// To use the Offt object, call the constructor with the FFT size, which must
// be a power of 2.  Then call getbuf() to retrieve a pointer to the buffer
// you use to pass data back and forth to the object.  Load up this buffer
// with real samples, then call r2c() to convert the buffer to complex FFT data.
// The format is the same as the old cmix fft routine, with pairs of real and
// imaginary floats, and buf[1] replaced with the real value of Nyquist.
// Muck around with the FFT complex data, then call c2r() to turn it back into
// real-valued samples.
//
// Check out Obucket also.  This class makes it fairly easy to decouple the
// FFT length from your instrument's buffer size and to have a fixed latency,
// regardless of note start time.  See insts/jg/SPECTACLE2_BASE.cpp for an
// example.
//                                                     -John Gibson, 6/4/05

#ifdef FFTW
#include <fftw3.h>
#else
class FFTReal;
#endif

class Offt {
public:
	enum {
		kRealToComplex = 1,
		kComplexToReal = 2
	};

	Offt(int fftsize, unsigned int flags = kRealToComplex | kComplexToReal);
	~Offt();
	float *getbuf() const { return _buf; }
	void r2c();
	void c2r();

private:
	void printbuf(float *buf, char *msg);

	int _len;
	float *_buf;
#ifdef FFTW
	fftwf_complex *_cbuf;
	fftwf_plan _plan_r2c, _plan_c2r;
#else
	float *_tmp;
	FFTReal *_fftobj;
#endif
};

