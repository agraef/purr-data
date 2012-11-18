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

// derived from RTcmix SPECTEQ2.cpp, (C) John Gibson 2005.

//#define DEBUG
#include "SpectEQ.h"

// ------------------------------------------------------------------ SpectEQ --
SpectEQ::SpectEQ()
	: _eqconst(0.0f), _eqtable(NULL)
{
}


// ----------------------------------------------------------------- ~SpectEQ --
SpectEQ::~SpectEQ()
{
	// We don't own eq table.
}


// --------------------------------------------------------------------- init --
// Invoke base init, and perform any initialization specific to this subclass.
// Return 0 if okay, -1 if not.

int SpectEQ::init(int fftlen, int windowlen, int overlap, float srate)
{
	return SpectacleBase::init(fftlen, windowlen, overlap, srate);
}


// ---------------------------------------------------------------- set_srate --
void SpectEQ::set_srate(float srate)
{
	SpectacleBase::set_srate(srate);
}


// -------------------------------------------------------------- set_eqtable --
void SpectEQ::set_eqtable(float *table, int len)
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


// --------------------------------------------------------- set_binmap_table --
// Set pointer to externally allocated table.  Pass table=NULL for no table.
// <len> must be the same as the current control table size, or else we use
// no table, without warning -- responsibility of caller to handle this.

void SpectEQ::set_binmap_table(int *table, int len)
{
	if (len != _control_table_size)
		_binmaptable = NULL;
	else
		_binmaptable = table;
	update_bin_groups(_bin_groups, NULL, get_minfreq(), get_maxfreq(),
		                                           _control_table_size);
}


// ---------------------------------------------------------- modify_analysis --
void SpectEQ::modify_analysis(bool)
{
	DPRINT("modify_analysis: .....................");

	// NB: check EQ table size, rather than table pointer, to determine whether
	// we should use an EQ constant. Otherwise, there's a danger we could read
	// a null EQ table pointer, if set_eqtable called by non-perf thread.
	if (_control_table_size == 0) {		// constant gain across bins
		const float eq = _ampdb(_eqconst);
		const int len = get_fftlen();
		for (int i = 0; i < len; i++)
			_fft_buf[i] *= eq;
	}
	else {
		for (int i = 0; i < _half_fftlen; i++) {
			const float eq = _ampdb(_eqtable[_bin_groups[i]]);
			const int index = i << 1;
			_fft_buf[index] *= eq;			// real
			_fft_buf[index + 1] *= eq;		// imag
		}
	}
	_fft_buf[1] = 0.0f;	// clear Nyquist real value
}


