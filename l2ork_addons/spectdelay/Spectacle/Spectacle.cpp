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

// derived from RTcmix SPECTACLE2.cpp, (C) John Gibson 2005.


#include "Spectacle.h"
#include <float.h>

//#define DEBUG
//#define PRINT_DELTIMES
//#define PRINT_DELTIME_CHANGES

#if defined(PRINT_DELTIME_CHANGES) && !defined(PRINT_DELTIMES)
 #define PRINT_DELTIMES
#endif

const float kMaxDelayTime = 20.0f;
#ifdef ANTI_DENORM
	const float kAntiDenormConstant = 1e-18f;
#endif


// ---------------------------------------------------------------- Spectacle --
Spectacle::Spectacle()
	: _maxdelsamps(0L), _maxdeltime(0.0f), _eqconst(0.0f), _deltimeconst(0.0f),
	  _feedbackconst(0.0f), _delay_minfreq(-FLT_MAX), _delay_maxfreq(-FLT_MAX),
	  _eqtable(NULL), _deltimetable(NULL), _feedbacktable(NULL),
	  _delay_bin_groups(NULL), _delay_binmap_table(NULL), _delay_table_size(0)
{
#ifdef ANTI_DENORM
	_antidenorm_offset = kAntiDenormConstant;
#endif
}


// --------------------------------------------------------------- ~Spectacle --
Spectacle::~Spectacle()
{
	// NB: we don't own the EQ, delay time, and feedback tables.

	for (int i = 0; i < _half_fftlen; i++) {
		if (_real_delay[i])
			delete _real_delay[i];
		if (_imag_delay[i])
			delete _imag_delay[i];
	}

	delete [] _delay_bin_groups;
}


// --------------------------------------------------------------------- init --
// Invoke base init, and perform any initialization specific to this subclass.
// Return 0 if okay, -1 if not.

int Spectacle::init(int fftlen, int windowlen, int overlap, float srate,
	float maxdeltime)
{
	if (SpectacleBase::init(fftlen, windowlen, overlap, srate) != 0)
		return -1;

	// Compute maximum delay lag and create delay lines for FFT real and
	// imaginary values.  Remember that these delays function at the decimation
	// rate, not at the audio rate, so the memory footprint is not as large
	// as you would expect -- about 44100 samples per second at fftlen=1024,
	// overlap=2 and SR=44100.

	_maxdeltime = maxdeltime;
	_maxdelsamps = long(maxdeltime * get_srate() / float(_decimation) + 0.5);

	for (int i = 0; i < _half_fftlen; i++) {
		Odelay *delay = new Odelay(_maxdelsamps);
		if (delay == NULL)
			goto bad_alloc;
		_real_delay[i] = delay;
		delay = new Odelay(_maxdelsamps);
		if (delay == NULL)
			goto bad_alloc;
		_imag_delay[i] = delay;
	}

	_delay_bin_groups = new int [_half_fftlen];

	set_delay_freqrange(0.0f, 0.0f);

	return 0;
bad_alloc:
	error("%s: Not enough memory for delay lines.", instname());
	return -1;
}


// -------------------------------------------------------------------- clear --
void Spectacle::clear()
{
	for (int i = 0; i < _half_fftlen; i++) {
		_real_delay[i]->clear();
		_imag_delay[i]->clear();
	}
	SpectacleBase::clear();
}


// ---------------------------------------------------------------- set_srate --
void Spectacle::set_srate(float srate)
{
	if (_delay_maxfreq == _nyquist)
		_delay_maxfreq = 0.0f;
	SpectacleBase::set_srate(srate);
	if (_delay_maxfreq == 0.0f)	// nyquist might have changed
		_delay_maxfreq = _nyquist;
	set_maxdeltime(_maxdeltime);
	update_bin_groups(_delay_bin_groups, _delay_binmap_table,
	                  _delay_minfreq, _delay_maxfreq, _delay_table_size);
}


// ----------------------------------------------------------- set_maxdeltime --
// Reset delay lines to accommodate a maximum delay of <time> seconds at the
// current sampling rate.  Assumes that caller constrains all delay times to
// fit this new maximum.
void Spectacle::set_maxdeltime(float time)
{
	_maxdeltime = time;
	_maxdelsamps = long(time * get_srate() / float(_decimation) + 0.5);
	for (int i = 0; i < _half_fftlen; i++) {
		_real_delay[i]->resize(_maxdelsamps);
		_imag_delay[i]->resize(_maxdelsamps);
	}
}


// -------------------------------------------------------------- set_eqtable --
void Spectacle::set_eqtable(float *table, int len)
{
	if (len == 0) {	// no table set yet by caller
		_eqconst = 0.0;
		_eqtable = NULL;
		_control_table_size = 0;
	}
	else if (len == 1) {
		_eqconst = table[0];
		_eqtable = NULL;
		_control_table_size = 0;
	}
	else {
		_eqconst = 0.0;
		_eqtable = table;
		if (len != _control_table_size) {
			_control_table_size = len;
			update_bin_groups(_bin_groups, NULL, get_minfreq(), get_maxfreq(),
		                                                 _control_table_size);
		}
	}
}


// ------------------------------------------------------------- set_deltable --
void Spectacle::set_deltable(float *table, int len)
{
	if (len == 0) {	// no table set yet by caller
		_deltimeconst = 0.0;
		_deltimetable = NULL;
	}
	else if (len == 1) {
		_deltimeconst = table[0];
		_deltimetable = NULL;
		// NB: don't set _delay_table_size to zero
	}
	else {
		_deltimeconst = 0.0;
		_deltimetable = table;
		if (len != _delay_table_size) {
			_delay_table_size = len;
			update_bin_groups(_delay_bin_groups, _delay_binmap_table, 
			                  _delay_minfreq, _delay_maxfreq, _delay_table_size);
		}
	}
}


// ------------------------------------------------------------ set_feedtable --
void Spectacle::set_feedtable(float *table, int len)
{
	if (len == 0) {	// no table set yet by caller
		_feedbackconst = 0.0;
		_feedbacktable = NULL;
	}
	else if (len == 1) {
		_feedbackconst = table[0];
		_feedbacktable = NULL;
	}
	else if (len == _delay_table_size) {
		_feedbackconst = 0.0;
		_feedbacktable = table;
	}
	// else don't do anything -- spectacle_fb_msg insures we never get here
}


// ------------------------------------------------------ set_delay_freqrange --
// Similar to set_freqrange in base class, but for _delay_bin_groups.
bool Spectacle::set_delay_freqrange(float min, float max)
{
	if (min != _delay_minfreq || max != _delay_maxfreq) {
		if (max == 0.0f)
			max = _nyquist;
		if (max < min)
			_fswap(&min, &max);
		if (max > _nyquist)
			max = _nyquist;
		_delay_maxfreq = max;
		_delay_minfreq = _fclamp(0.0f, min, max);
		update_bin_groups(_delay_bin_groups, _delay_binmap_table,
		                             _delay_minfreq, max, _delay_table_size);
		return true;
	}
	return false;
}


// --------------------------------------------------------- set_binmap_table --
// Set pointer to externally allocated table.  Pass table=NULL for no table.
// <len> must be the same as the current control table size, or else we use
// no table, without warning -- responsibility of caller to handle this.

void Spectacle::set_binmap_table(int *table, int len)
{
	if (table == NULL || len == 0 || len != _control_table_size)
		_binmaptable = NULL;
	else
		_binmaptable = table;
	update_bin_groups(_bin_groups, NULL, get_minfreq(), get_maxfreq(),
	                                                     _control_table_size);
}


// --------------------------------------------------- set_delay_binmap_table --
// Set pointer to externally allocated table.  Pass table=NULL for no table.
// <len> must be the same as the current delay table size, or else we use
// no table, without warning -- responsibility of caller to handle this.

void Spectacle::set_delay_binmap_table(int *table, int len)
{
	if (table == NULL || len == 0 || len != _delay_table_size)
		_delay_binmap_table = NULL;
	else
		_delay_binmap_table = table;
	update_bin_groups(_delay_bin_groups, _delay_binmap_table, 
	                       _delay_minfreq, _delay_maxfreq, _delay_table_size);
}


// ---------------------------------------------------------- modify_analysis --
void Spectacle::modify_analysis(bool reading_input)
{
	DPRINT("modify_analysis: .....................");

#ifdef PRINT_DELTIMES
	static float *prevdeltimes = NULL;
	if (prevdeltimes == NULL) {
		prevdeltimes = new float [_delay_table_size];
		post("\nmodify_analysis: delay times --------------------------");
		for (int i = 0; i < _delay_table_size; i++) {
			prevdeltimes[i] = _deltimetable[i];
			post("[%d] %f", i, _deltimetable[i]);
		}
 #ifdef PRINT_DELTIME_CHANGES
		post("\nmodify_analysis: delay time changes -------------------");
 #endif
	}
#endif

	const bool posteq = get_posteq();
	float eq;

	// NB: check EQ table size, rather than table pointer, to determine whether
	// we should use an EQ constant. Otherwise, there's a danger we could read
	// a null EQ table pointer, if set_eqtable called by non-perf thread.
	if (_control_table_size == 0)
		eq = _ampdb(_eqconst);

	for (int i = 0; i < _half_fftlen; i++) {
		int index = i << 1;

		if (_control_table_size > 0) {
			// EQ uses base class bin groups array.
			const int bg = _bin_groups[i];
			eq = _ampdb(_eqtable[bg]);
		}

		float real, imag;
		if (reading_input) {
		   if (posteq) {
				real = _fft_buf[index];
				imag = _fft_buf[index + 1];
			}
			else {
				real = _fft_buf[index] * eq;
				imag = _fft_buf[index + 1] * eq;
			}
		}
		else {
			real = 0.0f;
			imag = 0.0f;
		}

		const int bg = _delay_bin_groups[i];

		// NB: caller must assure that deltime is in range
		const float deltime = _deltimetable ? _deltimetable[bg] : _deltimeconst;

#ifdef PRINT_DELTIME_CHANGES
		if (deltime != prevdeltimes[bg]) {
			post("[%d] %f", bg, deltime);
			prevdeltimes[bg] = deltime;
		}
#endif

		if (deltime == 0.0f) {
		   if (posteq) {
				_fft_buf[index] = real * eq;
				_fft_buf[index + 1] = imag * eq;
			}
			else {
				_fft_buf[index] = real;
				_fft_buf[index + 1] = imag;
			}
		}
		else {
			const long delsamps = long((deltime * get_srate()) + 0.5)
			                                                     / _decimation;
			const float newreal = _real_delay[i]->getsamp(delsamps);
			const float newimag = _imag_delay[i]->getsamp(delsamps);
			const float feedback = _feedbacktable ? _feedbacktable[bg]
			                                      : _feedbackconst;
			if (feedback != 0.0) {
#ifdef ANTI_DENORM
				float fbsig = (newreal * feedback) + _antidenorm_offset;
				_real_delay[i]->putsamp(real + fbsig);
				fbsig = (newimag * feedback) + _antidenorm_offset;
				_imag_delay[i]->putsamp(imag + fbsig);
#else
				_real_delay[i]->putsamp(real + (newreal * feedback));
				_imag_delay[i]->putsamp(imag + (newimag * feedback));
#endif
			}
			else {
				_real_delay[i]->putsamp(real);
				_imag_delay[i]->putsamp(imag);
			}
		   if (posteq) {
				_fft_buf[index] = newreal * eq;
				_fft_buf[index + 1] = newimag * eq;
			}
			else {
				_fft_buf[index] = newreal;
				_fft_buf[index + 1] = newimag;
			}
		}
	}

	_fft_buf[1] = 0.0f;	// clear Nyquist real value

#ifdef ANTI_DENORM
	_antidenorm_offset = -_antidenorm_offset;
#endif
}


