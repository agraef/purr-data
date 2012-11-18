/* RTcmix - Copyright (C) 2005  The RTcmix Development Team
   See ``AUTHORS'' for a list of contributors. See ``LICENSE'' for
   the license to this software and for a DISCLAIMER OF ALL WARRANTIES.
*/

#include "Offt.h"
#ifndef FFTW
#include "FFTReal.h"
#endif


Offt::Offt(int fftsize, unsigned int flags)
	: _len(fftsize)
{
#ifdef FFTW
	_plan_r2c = _plan_c2r = NULL;
	_buf = (float *) fftwf_malloc(sizeof(float) * _len);
	int csize = sizeof(fftwf_complex) * ((_len / 2) + 1);
	_cbuf = (fftwf_complex *) fftwf_malloc(csize);
	if (flags & kRealToComplex)
		_plan_r2c = fftwf_plan_dft_r2c_1d(_len, _buf, _cbuf, FFTW_ESTIMATE);
	if (flags & kComplexToReal)
		_plan_c2r = fftwf_plan_dft_c2r_1d(_len, _cbuf, _buf, FFTW_ESTIMATE);
#else // !FFTW
	_buf = new float [_len];
	_tmp = new float [_len];
	_fftobj = new FFTReal(_len);
#endif // !FFTW
}

Offt::~Offt()
{
#ifdef FFTW
	if (_plan_r2c)
		fftwf_destroy_plan(_plan_r2c);
	if (_plan_c2r)
		fftwf_destroy_plan(_plan_c2r);
	fftwf_free(_buf);
	fftwf_free(_cbuf);
#else // !FFTW
	delete [] _buf;
	delete [] _tmp;
	delete _fftobj;
#endif // !FFTW
}


#ifdef FFTW

void Offt::r2c()
{
	fftwf_execute(_plan_r2c);

	// _cbuf has complex result in real,imaginary pairs from DC to Nyquist;
	// copy into _buf while reordering to...
	//    re(0), re(len/2), re(1), im(1), re(2), im(2)...re(len/2-1), im(len/2-1)
	// and normalizing by 1 / fftlen.
	float scale = 1.0 / _len;
	int last = _len / 2;
	_buf[0] = _cbuf[0][0] * scale;
	_buf[1] = _cbuf[last][0] * scale;
	for (int i = 1; i < last; i++) {
		_buf[i + i] = _cbuf[i][0] * scale;
		_buf[i + i + 1] = _cbuf[i][1] * scale;
	}
}

void Offt::c2r()
{
	// _buf has complex data; copy into _cbuf while reordering from...
	//    re(0), re(len/2), re(1), im(1), re(2), im(2)...re(len/2-1), im(len/2-1)
	// to real,imaginary pairs from DC to Nyquist;
	int last = _len / 2;
	_cbuf[0][0] = _buf[0];
	_cbuf[last][0] = _buf[1];
	for (int i = 1; i < last; i++) {
		_cbuf[i][0] = _buf[i + i];
		_cbuf[i][1] = _buf[i + i + 1];
	}

	fftwf_execute(_plan_c2r);
}

#else // !FFTW

void Offt::r2c()
{
	_fftobj->do_fft(_tmp, _buf);

	// _tmp has complex result; copy into _buf while reordering from...
	//    re(0), re(1), re(2)...re(len/2), im(1), im(2)...im(len/2-1)
	// to...
	//    re(0), re(len/2), re(1), im(1), re(2), im(2)...re(len/2-1), im(len/2-1)
	// and normalizing by 1 / fftlen.

	float scale = 1.0 / _len;
	int j = _len / 2;
	_buf[0] = _tmp[0] * scale;
	_buf[1] = _tmp[j] * scale;
#if 0
	for (int i = 1; i < j; i++) {
		_buf[i + i] = _tmp[i] * scale;				// real
		_buf[i + i + 1] = _tmp[j + i] * scale;		// imag
	}
#else
	int i = j - 1;
	float *bp = &_buf[_len - 1];
	float *tp = &_tmp[j + i];
	do {
		*bp-- = *tp-- * scale;         // imag
		*bp-- = _tmp[i--] * scale;     // real
	} while (i > 0);
#endif
}

void Offt::c2r()
{
	// _buf has complex data; copy into _tmp while reordering from...
	//    re(0), re(len/2), re(1), im(1), re(2), im(2)...re(len/2-1), im(len/2-1)
	// to...
	//    re(0), re(1), re(2)...re(len/2), im(1), im(2)...im(len/2-1)

	int j = _len / 2;
	_tmp[0] = _buf[0];
	_tmp[j] = _buf[1];
#if 0
	for (int i = 1; i < j; i++) {
		_tmp[i] = _buf[i + i];				// real
		_tmp[j + i] = _buf[i + i + 1];	// imag
	}
#else
	int i = j - 1;
	float *bp = &_buf[_len - 1];
	float *tp = &_tmp[j + i];
	do {
		*tp-- = *bp--;         // imag
		_tmp[i--] = *bp--;     // real
	} while (i > 0);
#endif

	_fftobj->do_ifft(_tmp, _buf);

	// _buf now holds real output
}

#endif // !FFTW


#include <stdio.h>

void Offt::printbuf(float *buf, char *msg)
{
	printf("\n%s\n", msg);
	for (int i = 0; i < _len; i++)
		printf("\t[%d] = %f\n", i, buf[i]);
}

