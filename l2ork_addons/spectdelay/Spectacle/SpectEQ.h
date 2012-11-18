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

class SpectEQ : public SpectacleBase {

public:
	SpectEQ();
	virtual ~SpectEQ();
	int init(int fftlen, int windowlen, int overlap, float srate);
	void set_srate(float srate);
	void set_eqtable(float *table, int len);
	void set_binmap_table(int *table, int len);

protected:
	virtual void modify_analysis(bool reading_input);
	virtual const char *instname() { return "specteq~"; }

private:
	float _eqconst;
	float *_eqtable;
};

