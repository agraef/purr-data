/*

FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __FFTEASE_H
#define __FFTEASE_H

#define FFTEASE_VERSION "0.0.0"


#define FLEXT_ATTRIBUTES 1 

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 501)
#error You need at least flext version 0.5.1
#endif


#include "pv.h"

// lazy me
#define F float
#define D double
#define I int
#define L long
#define C char
#define V void
#define BL bool
#define S t_sample



class fftease:
	public flext_dsp
{
	FLEXT_HEADER(fftease,flext_dsp)
	
public:
	fftease(I mult,I flags);
	virtual ~fftease();

	static F FromdB(F v) { return pow(10.,v*.05); } 

	inline I get_N() const { return _N; }
	inline F get_Fund() const { return smprt/_N; }

protected:

	virtual BL Init();
	virtual V Exit();

	virtual V m_dsp(I n,S *const *in,S *const *out);
	virtual V m_signal(I n,S *const *in,S *const *out);

	virtual V Set();
	virtual V Clear();
	virtual V Delete();
	virtual V Transform(I _N,S *const *in) = 0;


	V Mult(I n) { _mult = n; MakeVar(); }
	inline I Mult() const { return _mult; }

    F *_input1,*_input2;
    F *_buffer1,*_buffer2;
    F *_channel1,*_channel2;
    F *_output;
    F *_trigland;
    I *_bitshuffle;
    F *_Wanal,*_Wsyn,*_Hwin;

	F *_c_lastphase_in1,*_c_lastphase_in2,*_c_lastphase_out;
	F _c_factor_in;

    I _inCount;

	enum { 
		F_STEREO = 0x01,
		F_BALANCED = 0x02,
		F_BITSHUFFLE = 0x04,
		F_RMS = 0x08,
		F_NOSPEC = 0x10,F_SPECRES = 0x20,F_PHCONV = 0x40,
		F_NOAMP1 = 0x100,
		F_NOPH1 = 0x200,
		F_NOAMP2 = 0x400,
		F_NOPH2 = 0x800
	};

	I _flags;
	F _rms;

private:

	V MakeVar() { _N = Blocksize()*_mult; }

	I blsz;
	F smprt;
	I _mult,_N;
};


#endif
