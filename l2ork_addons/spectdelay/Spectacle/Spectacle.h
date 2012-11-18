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

//#define DEBUG

#include "SpectacleBase.h"

#if defined(i386)
	#define ANTI_DENORM
#endif

class Spectacle : public SpectacleBase {

public:
	Spectacle();
	virtual ~Spectacle();
	int init(int fftlen, int windowlen, int overlap, float srate,
		float maxdeltime);
	void clear();
	void set_srate(float srate);
	void set_maxdeltime(float time);
	void set_eqtable(float *table, int len);
	void set_deltable(float *table, int len);
	void set_feedtable(float *table, int len);
	void set_binmap_table(int *table, int len);
	void set_delay_binmap_table(int *table, int len);
	bool set_delay_freqrange(float min, float max);

protected:
	virtual void modify_analysis(bool reading_input);
	virtual const char *instname() { return "spectacle~"; }

private:
	long _maxdelsamps;
	float _maxdeltime;
	float _eqconst, _deltimeconst, _feedbackconst;
	float _delay_minfreq, _delay_maxfreq;
	float *_eqtable, *_deltimetable, *_feedbacktable;
	int *_delay_bin_groups, *_delay_binmap_table, _delay_table_size;
	Odelay *_real_delay[kMaxFFTLen / 2];
	Odelay *_imag_delay[kMaxFFTLen / 2];
#ifdef ANTI_DENORM
	float _antidenorm_offset;
#endif
};

