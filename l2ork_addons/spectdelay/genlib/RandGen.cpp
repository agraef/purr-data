/* RTcmix  - Copyright (C) 2004  The RTcmix Development Team
   See ``AUTHORS'' for a list of contributors. See ``LICENSE'' for
   the license to this software and for a DISCLAIMER OF ALL WARRANTIES.
*/

#include "RandGen.h"
#include <math.h>
#include <assert.h>

// Classes for generating random numbers   JGG, 6/22/04
// Most of this code is based on C functions written by Luke Dubois.
// One class is based on code written by Mara Helmuth.

inline double dmax(double x, double y) { return (x > y) ? x : y; }
inline double dmin(double x, double y) { return (x < y) ? x : y; }


// Base class -----------------------------------------------------------------

RandGen::~RandGen() {}

// Return a random number in range [0, 1]
inline double RandGen::rawvalue()
{
	_randx = (_randx * 1103515245) + 12345;
	int k = (_randx >> 16) & 077777;
	return (double) k / 32768.0;
}

// Scale <num>, which must be in range [0, 1], to fit range [_min, _max]
inline double RandGen::fitrange(double num) const
{
	assert(num >= 0.0 && num <= 1.0);
	return _min + (num * (_max - _min));
}


// Linear distribution subclass -----------------------------------------------

LinearRandom::LinearRandom(double min, double max, int seed)
	: RandGen(min, max, seed) {}

LinearRandom::~LinearRandom() {}

double LinearRandom::value()
{
	return fitrange(rawvalue());
}


// Low-weighted linear distribution subclass ----------------------------------

LowLinearRandom::LowLinearRandom(double min, double max, int seed)
	: RandGen(min, max, seed) {}

LowLinearRandom::~LowLinearRandom() {}

double LowLinearRandom::value()
{
	double num1 = rawvalue();
	double num2 = rawvalue();
	return fitrange(dmin(num1, num2));
}


// High-weighted linear distribution subclass ---------------------------------

HighLinearRandom::HighLinearRandom(double min, double max, int seed)
	: RandGen(min, max, seed) {}

HighLinearRandom::~HighLinearRandom() {}

double HighLinearRandom::value()
{
	double num1 = rawvalue();
	double num2 = rawvalue();
	return fitrange(dmax(num1, num2));
}


// Triangle distribution subclass ---------------------------------------------

TriangleRandom::TriangleRandom(double min, double max, int seed)
	: RandGen(min, max, seed) {}

TriangleRandom::~TriangleRandom() {}

double TriangleRandom::value()
{
	double num1 = rawvalue();
	double num2 = rawvalue();
	double tmp = 0.5 * (num1 + num2);
	return fitrange(tmp);
}


// Gaussian distribution subclass ---------------------------------------------

GaussianRandom::GaussianRandom(double min, double max, int seed)
	: RandGen(min, max, seed) {}

GaussianRandom::~GaussianRandom() {}

double GaussianRandom::value()
{
	const int N = 12;
	const double halfN = 6.0;
	const double scale = 1.0;
	const double mu = 0.5;
	const double sigma = 0.166666;

	double num;
	do {
		num = 0.0;
		for (int j = 0; j < N; j++)
			num += rawvalue();
		num = sigma * scale * (num - halfN) + mu;
	} while (num < 0.0 || num > 1.0);

	return fitrange(num);
}


// Cauchy distribution subclass -----------------------------------------------

CauchyRandom::CauchyRandom(double min, double max, int seed)
	: RandGen(min, max, seed) {}

CauchyRandom::~CauchyRandom() {}

double CauchyRandom::value()
{
	const double alpha = 0.00628338;

	double num;
	do {
		do {
			num = rawvalue();
		} while (num == 0.5);
		num = (alpha * tan(num * M_PI)) + 0.5;
	} while (num < 0.0 || num > 1.0);

	return fitrange(num);
}


// Probability distribution subclass ------------------------------------------
// Based on code by Mara Helmuth
//
//    <min> and <max> set the range within which the random numbers fall.
//
//    <mid> sets the mid-point of the range, which has an effect whenever
//    <tight> is not 1.
//
//    <tight> governs the degree to which the random numbers adhere either
//    to the mid-point or to the extremes of the range:
//
//       tight         effect
//       -------------------------------------------------------------
//       0             only the <min> and <max> values appear
//       1             even distribution
//       > 1           numbers cluster ever more tightly around <mid>
//       100           almost all numbers are equal to <mid>

ProbRandom::ProbRandom(double min, double max, double mid, double tight,
	int seed)
	: RandGen(min, max, mid, tight, seed) {}

ProbRandom::~ProbRandom() {}

double ProbRandom::value()
{
	const double min = getmin();
	const double max = getmax();
	const double mid = getmid();
	const double tight = gettight();
	double hirange = max - mid;
	double lowrange = mid - min;
	double range = dmax(hirange, lowrange);

	double num;
	do {
		double sign;
		num = rawvalue();       // num is [0,1]
		if (num > 0.5)
			sign = 1.0;
		else
			sign = -1.0;
		num = mid + (sign * (pow(rawvalue(), tight) * range));
	} while (num < min || num > max);

	return num;
}

// RandomOscil

RandomOscil::RandomOscil(RandGen *gen, double srate, double freq)
	: _gen(gen), _srate(srate)
{
	setfreq(freq);
	_counter = _limit;		// trigger first _gen->value()
}

RandomOscil::~RandomOscil()
{
	delete _gen;
}

void RandomOscil::setfreq(double freq)
{
	_limit = (int) (_srate / freq);
}

double RandomOscil::next()
{
	if (_counter >= _limit) {
		_curval = _gen->value();
		_counter = 0;
	}
	else
		_counter++;
	return _curval;
}

