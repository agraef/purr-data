/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"


fftease::fftease(I mult,I flags):
	_inCount(0),
	_flags(flags),
	blsz(0),smprt(0),
	_mult(mult),_N(0)
{}

fftease::~fftease() {}

BL fftease::Init()
{
	Clear();
	Set();
	return flext_dsp::Init();
}

V fftease::Exit()
{
	Delete();
}

V fftease::m_dsp(I n,S *const *,S *const *)
{
	const F sr = Samplerate();
	if(n != blsz || sr != smprt) {
		blsz = n;
		smprt = sr;
		MakeVar();

		Delete();
		Set();
	}
}

V fftease::m_signal(I n,S *const *in,S *const *out)
{
	/* declare working variables */
	I i; 
	const I _N = get_N(),_Nw = _N,_N2 = _N/2; //,_Nw2 = _Nw/2; 

	/* fill our retaining buffers */
	_inCount += n;

	if(_flags&F_STEREO) {
		for(i = 0; i < _N-n ; i++ ) {
			_input1[i] = _input1[i+n];
			_input2[i] = _input2[i+n];
		}
		for(I j = 0; i < _N; i++,j++) {
			_input1[i] = in[0][j];
			_input2[i] = in[1][j];
		}
	}
	else {
		for (i = 0 ; i < _N-n ; i++ )
			_input1[i] = _input1[i+n];
		for (I j = 0; i < _N; i++,j++ )
			_input1[i] = in[0][j];
	}

	_rms = 0.;
	if(_flags&F_RMS) {
		for ( i=0; i < _Nw; i++ )
			_rms += _input1[i] * _input1[i];
		_rms = sqrt(_rms / _Nw);
	}

	/* apply hamming window and fold our window buffer into the fft buffer */
	fold( _input1, _Wanal, _Nw, _buffer1, _N, _inCount );
	if(_flags&F_STEREO) fold( _input2, _Wanal, _Nw, _buffer2, _N, _inCount );

	/* do an fft */
	if(_flags&F_BITSHUFFLE) {
		pv_rdft( _N, 1, _buffer1, _bitshuffle, _trigland );
		if(_flags&F_STEREO) pv_rdft( _N, 1, _buffer2, _bitshuffle, _trigland );
	}
	else {
		pv_rfft( _buffer1, _N2, 1);
		if(_flags&F_STEREO) pv_rfft( _buffer2, _N2,1);
	}

	if(!(_flags&F_NOSPEC)) {
		if(_flags&F_PHCONV) {
			convert( _buffer1, _channel1, _N2, _c_lastphase_in1, get_Fund(), _c_factor_in );
			if(_flags&F_STEREO) convert( _buffer2, _channel2, _N2, _c_lastphase_in2, get_Fund(), _c_factor_in );
		}
		else {
			leanconvert( _buffer1, _channel1, _N2 , !(_flags&F_NOAMP1),!(_flags&F_NOPH1));
			if(_flags&F_STEREO) leanconvert( _buffer2, _channel2, _N2 ,!(_flags&F_NOAMP2),!(_flags&F_NOPH2) );
		}
	}

	// ---- BEGIN --------------------------------

	Transform(_N,in+((_flags&F_STEREO)?2:1));

	// ---- END --------------------------------

	if(!(_flags&F_NOSPEC)) {
		if(_flags&F_PHCONV)
			unconvert( _channel1, _buffer1, _N2, _c_lastphase_out, get_Fund(), 1./_c_factor_in  );
		else
			leanunconvert( _channel1, _buffer1, _N2 );
	}


	/* do an inverse fft */
	if(_flags&F_BITSHUFFLE)
		pv_rdft( _N, -1, _buffer1, _bitshuffle, _trigland );
	else
		pv_rfft( _buffer1, _N2, 0);

	/* dewindow our result */
	overlapadd( _buffer1, _N, _Wsyn, _output, _Nw, _inCount);

	/* set our output and adjust our retaining output buffer */
	const F mult = 1./_N;
	for ( i = 0; i < n; i++ )
		out[0][i] = _output[i] * mult;

	for ( i = 0; i < _N-n; i++ )
		_output[i] = _output[i+n];
	for (; i < _N; i++ )
		_output[i] = 0.;
}


void fftease::Set()
{
	/* preset the objects data */
	const I n = Blocksize(),_N = n*Mult(),_Nw = _N,_N2 = _N/2; //,_Nw2 = _Nw/2;

	_inCount = -_Nw;

	/* assign memory to the buffers */
	_input1 = new F[_Nw];
	ZeroMem(_input1,_Nw*sizeof(*_input1));
	_buffer1 = new F[_N];
	if(_flags&F_STEREO) {
		_input2 = new F[_Nw];
		ZeroMem(_input2,_Nw*sizeof(*_input2));
		_buffer2 = new F[_N];
	}

	if(!(_flags&F_NOSPEC) || (_flags&F_SPECRES)) {
		_channel1 = new F[_N+2];
		ZeroMem(_channel1,(_N+2)*sizeof(*_channel1));
		if(_flags&F_STEREO) {
			_channel2 = new F[_N+2];
			ZeroMem(_channel2,(_N+2)*sizeof(*_channel2));
		}

		if(_flags&F_PHCONV) {
			_c_lastphase_in1 = new F[_N2+1];
			ZeroMem(_c_lastphase_in1,(_N2+1)*sizeof(*_c_lastphase_in1));
			if(_flags&F_STEREO) {
				_c_lastphase_in2 = new F[_N2+1];
				ZeroMem(_c_lastphase_in2,(_N2+1)*sizeof(*_c_lastphase_in2));
			}
			_c_lastphase_out = new F[_N2+1];
			ZeroMem(_c_lastphase_out,(_N2+1)*sizeof(*_c_lastphase_out));

			_c_factor_in = Samplerate()/(n * PV_2PI);
		}
	}

	_output = new F[_Nw];
	ZeroMem(_output,_Nw*sizeof(*_output));

	if(_flags&F_BITSHUFFLE) {
		_bitshuffle = new I[_N*2];
		_trigland = new F[_N*2];
		init_rdft( _N, _bitshuffle, _trigland);
	}

	_Hwin = new F[_Nw];
	_Wanal = new F[_Nw];
	_Wsyn = new F[_Nw];
	if(_flags&F_BALANCED)
		makewindows( _Hwin, _Wanal, _Wsyn, _Nw, _N, n, 0);
	else
		makehanning( _Hwin, _Wanal, _Wsyn, _Nw, _N, n, 0,0);
}

void fftease::Clear()
{
	_bitshuffle = NULL;
	_trigland = NULL;
	_input1 = _input2 = NULL;
	_Hwin = NULL;
	_Wanal = _Wsyn = NULL;
	_buffer1 = _buffer2 = NULL;
	_channel1 = _channel2 = NULL;
	_output = NULL;

	_c_lastphase_in1 = _c_lastphase_in2 = _c_lastphase_out = NULL;
}

void fftease::Delete()
{
	if(_input1) delete[] _input1;
	if(_buffer1) delete[] _buffer1;
	if(_channel1) delete[] _channel1;
	if(_input2) delete[] _input2;
	if(_buffer2) delete[] _buffer2;
	if(_channel2) delete[] _channel2;

	if(_c_lastphase_in1) delete[] _c_lastphase_in1;
	if(_c_lastphase_in2) delete[] _c_lastphase_in2;
	if(_c_lastphase_out) delete[] _c_lastphase_out;

	if(_output) delete[] _output;

	if(_bitshuffle) delete[] _bitshuffle;
	if(_trigland) delete[] _trigland;

	if(_Hwin) delete[] _Hwin;
	if(_Wanal) delete[] _Wanal;
	if(_Wsyn) delete[] _Wsyn;
}


