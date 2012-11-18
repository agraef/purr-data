/* RTcmix  - Copyright (C) 2004  The RTcmix Development Team
   See ``AUTHORS'' for a list of contributors. See ``LICENSE'' for
   the license to this software and for a DISCLAIMER OF ALL WARRANTIES.
*/

// Classes for generating random numbers   JGG, 6/22/04
// Most of this code is based on C functions written by Luke Dubois.
// One class is based on code written by Mara Helmuth.

#ifndef _RANDOM_H_
#define _RANDOM_H_

enum {
	kLinearRandom = 0,
	kLowLinearRandom,
	kHighLinearRandom,
	kTriangleRandom,
	kGaussianRandom,
	kCauchyRandom,
	kProbRandom
};

// Base class

class RandGen {
public:
	RandGen(double min, double max, int seed)
		: _min(min), _max(max), _mid(0), _tight(0) { setseed(seed); }
	RandGen(double min, double max, double mid, double tight, int seed)
		: _min(min), _max(max), _mid(mid), _tight(tight) { setseed(seed); }
	virtual ~RandGen();
	virtual double value() = 0;	// NB: not const, because it changes _randx
	void setseed(const int aseed) { _randx = (long) aseed; }
	void setmin(const double min) { _min = min; }
	void setmax(const double max) { _max = max; }
	void setmid(const double mid) { _mid = mid; }
	void settight(const double tight) { _tight = tight; }
protected:
	double rawvalue();
	double fitrange(double num) const;
	double getmin() const { return _min; }
	double getmax() const { return _max; }
	double getmid() const { return _mid; }
	double gettight() const { return _tight; }
private:
	long _randx;
	double _min;
	double _max;
	double _mid;
	double _tight;
};


// Subclasses, one for each distribution type

class LinearRandom : public RandGen {
public:
	LinearRandom(double min = 0.0, double max = 1.0, int seed = 1);
	virtual double value();
protected:
	virtual ~LinearRandom();
};

class LowLinearRandom : public RandGen {
public:
	LowLinearRandom(double min = 0.0, double max = 1.0, int seed = 1);
	virtual double value();
protected:
	virtual ~LowLinearRandom();
};

class HighLinearRandom : public RandGen {
public:
	HighLinearRandom(double min = 0.0, double max = 1.0, int seed = 1);
	virtual double value();
protected:
	virtual ~HighLinearRandom();
};

class TriangleRandom : public RandGen {
public:
	TriangleRandom(double min = 0.0, double max = 1.0, int seed = 1);
	virtual double value();
protected:
	virtual ~TriangleRandom();
};

class GaussianRandom : public RandGen {
public:
	GaussianRandom(double min = 0.0, double max = 1.0, int seed = 1);
	virtual double value();
protected:
	virtual ~GaussianRandom();
};

class CauchyRandom : public RandGen {
public:
	CauchyRandom(double min = 0.0, double max = 1.0, int seed = 1);
	virtual double value();
protected:
	virtual ~CauchyRandom();
};

class ProbRandom : public RandGen {
public:
	ProbRandom(double min = 0.0, double max = 1.0, double mid = 0.5,
												double tight = 1.0, int seed = 1);
	virtual double value();
protected:
	virtual ~ProbRandom();
};


class RandomOscil
{
public:
	// NB: RandomOscil takes ownership of gen, deleting it on destruction.
	RandomOscil(RandGen *gen, double srate, double freq);
	~RandomOscil();
	void setfreq(double freq);
	void setseed(const int aseed) { _gen->setseed(aseed); }
	void setmin(const double min) { _gen->setmin(min); }
	void setmax(const double max) { _gen->setmax(max); }
	void setmid(const double mid) { _gen->setmid(mid); }
	void settight(const double tight) { _gen->settight(tight); }
	double next();
private:
	RandGen *_gen;
	double _srate;
	double _curval;
	int _counter;
	int _limit;
};

#endif  // _RANDOM_H_

