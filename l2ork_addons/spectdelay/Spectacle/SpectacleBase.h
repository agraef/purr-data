/*
 *  Copyright (C) 2007 John Gibson
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <math.h>
#include "../genlib/Ougens.h"
#include "../../../pd/src/m_pd.h"
//#include "ext.h"	// for Max/MSP post and error functions

//#define NDEBUG
#include <assert.h>

//#include whatever max file has post/error defs

const int kMaxFFTLen = 16384;

#ifndef powf
	#define powf(val, exp) pow((val), (exp))
#endif

#ifdef DEBUG
	#define DPRINT(msg) post((msg))
	#define DPRINT1(msg, arg) post((msg), (arg))
	#define DPRINT2(msg, arg1, arg2) post((msg), (arg1), (arg2))
	#define DPRINT3(msg, arg1, arg2, arg3) post((msg), (arg1), (arg2), (arg3))
#else
	#define DPRINT(msg)
	#define DPRINT1(msg, arg)
	#define DPRINT2(msg, arg1, arg2)
	#define DPRINT3(msg, arg1, arg2, arg3)
#endif

class SpectacleBase {

public:
	SpectacleBase();
	virtual ~SpectacleBase();
	void run(float in[], float out[], int nframes);
	bool set_freqrange(float min, float max);

   // set hold to true to suppress input
   void set_hold(bool hold) { _reading_input = !hold; }

	// set posteq to true to apply EQ after delay, rather than before
	void set_posteq(bool posteq) { _posteq = posteq; }

protected:
	int init(int fftlen, int windowlen, int overlap, float srate);
	void clear();
	void set_srate(float srate);
	virtual void modify_analysis(bool reading_input) = 0;
	virtual const char *instname() = 0;
	inline int closest_bin(const float freq);
	void update_bin_groups(int bin_groups[], int binmaptable[], 
		const float minfreq, const float maxfreq, 
		const int control_table_size);
	void print_bin_groups(int bin_groups[], const char *type);

	// helper functions
	float _ampdb(float db) { return powf(10.0f, db * 0.05f); }
	float _fclamp(float min, float val, float max) {
	                    return (val < min) ? min : ((val > max) ? max : val); }
	void _fswap(float *a, float *b) { float tmp = *a; *a = *b; *b = tmp; }
	int _imax(int x, int y) { return x > y ? x : y; }
	int _imin(int x, int y) { return x < y ? x : y; }
	int _odd(long x) { return (x & 0x00000001); }

	// accessors for subclasses
	void set_fftlen(int fftlen) { _fftlen = fftlen; }
	int get_fftlen() const { return _fftlen; }
	void set_window_len(int window_len) { _window_len = window_len; }
	int get_window_len() const { return _window_len; }
	void set_overlap(int overlap) { _overlap = overlap; }
	int get_overlap() const { return _overlap; }
	float get_minfreq() const { return _minfreq; }
	float get_maxfreq() const { return _maxfreq; }
	float get_srate() const { return _srate; }
	float get_posteq() const { return _posteq; }

	// For a subclass that wants to change the base class array for some reason.
	void set_bin_groups(int bin_groups[]) {
		delete [] _bin_groups;
		_bin_groups = bin_groups;	// assume new one is valid
	}

	int _input_frames, _nargs, _decimation, _overlap, _half_fftlen;
	float _fund_anal_freq, _nyquist;

	// size of the "control tables" (e.g., EQ) used with _bin_groups[] below

	int _control_table_size;

	// This pointer gives us direct access to the Offt object's buffer.
	// It has _fftlen elements, interpreted as described in Offt.h.

	float *_fft_buf;

	// A "bin group" is a range of FFT bins that are controlled by a single
	// element in one of the "control tables."  The <_bin_groups> array has
	// <_half_fftlen> values, each of which is an index into these control
	// tables.  So <_bin_groups> tells us which control table value to use for
	// a given FFT bin.  <_bin_groups> is updated whenever minfreq or maxfreq
	// changes.  The base class _bin_groups array is used by a subclass in any
	// way it wants (typically for EQ).  A subclass may have additional bin
	// groups, but these are handled entirely within the subclass.  The subclass
	// can call update_bin_groups to initialize these extra bin groups.

	int *_bin_groups;

	// Used for specifying bin groups.
	int *_binmaptable;

private:
	int make_windows();
	void prepare_input(const float buf[]);
	void prepare_output();
	static void process_wrapper(const float buf[], const int len, void *obj);
	void process(const float *buf, const int len);
	inline void increment_out_read_index();
	inline void increment_out_write_index();

	bool _print_stats, _reading_input, _posteq;
	int _fftlen, _window_len, _prev_bg_ignorevals;
	int _out_read_index, _out_write_index, _outframes;
	int _window_len_minus_decimation;
	float _srate, _minfreq, _maxfreq;
	float *_anal_window, *_synth_window, *_input, *_output, *_outbuf;
	unsigned long _cursamp;	// good for about 27 hours on a 32bit machine
	Offt *_fft;
	Obucket *_bucket;
};


// Return the index of the FFT bin whose center frequency is closest to <freq>.
inline int SpectacleBase::closest_bin(const float freq)
{
	return int((freq / _fund_anal_freq) + 0.5f);
}


inline void SpectacleBase::increment_out_read_index()
{
	if (++_out_read_index == _outframes)
		_out_read_index = 0;
//	assert(_out_read_index != _out_write_index);
}

inline void SpectacleBase::increment_out_write_index()
{
	if (++_out_write_index == _outframes)
		_out_write_index = 0;
//	assert(_out_write_index != _out_read_index);
}


