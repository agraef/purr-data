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

// Base class for the various spectacle externals, derived from
// my SPECTACLE2 RTcmix instruments.
// John Gibson <johgibso at indiana dot edu>, 11/23/07.

//#define DEBUG
//#define DEBUG_BINGROUPS
//#define CHECK_BINGROUPS

#include "SpectacleBase.h"
#include <float.h>

#ifndef cosf
	#define cosf(x) cos((x))
#endif
#ifndef sinf
	#define sinf(x) sin((x))
#endif
#ifndef hypotf
	#define hypotf(x, y) hypot((x), (y))
#endif
#ifndef atan2f
	#define atan2f(y, x) atan2((y), (x))
#endif

const int kMaxWindowLen = kMaxFFTLen * 8;
const int kMinOverlap = 1;
const int kMaxOverlap = 64;


// ------------------------------------------------------------ SpectacleBase --
SpectacleBase::SpectacleBase() :
	_control_table_size(0),
	_bin_groups(NULL),
	_binmaptable(NULL),
	_print_stats(false),
   _reading_input(true),
   _posteq(false),
	_prev_bg_ignorevals(-1),
	_srate(-FLT_MAX),
	_minfreq(-FLT_MAX), _maxfreq(-FLT_MAX),
	_anal_window(NULL), _synth_window(NULL),
	_input(NULL), _output(NULL),
	_outbuf(NULL),
	_cursamp(0L),
	_fft(NULL),
	_bucket(NULL)
{
}


// ----------------------------------------------------------- ~SpectacleBase --
SpectacleBase::~SpectacleBase()
{
	delete [] _outbuf;
	delete [] _input;
	delete [] _output;
	delete [] _anal_window;
	delete [] _synth_window;
	delete [] _bin_groups;
	delete _bucket;
	delete _fft;
}


// ------------------------------------------------------------- make_windows --
int SpectacleBase::make_windows()
{
	// Hamming window
	for (int i = 0; i < _window_len; i++)
		_anal_window[i] = _synth_window[i] = 0.54f - 0.46f
								 * cosf(2.0f * M_PI * i / (_window_len - 1));

	// When _window_len > _fftlen, also apply interpolating (sinc) windows to
	// ensure that window is 0 at increments of _fftlen away from the center
	// of the analysis window and of decimation away from the center of the
	// synthesis window.

	if (_window_len > _fftlen) {
		float x = -(_window_len - 1) / 2.0;
		for (int i = 0; i < _window_len; i++, x += 1.0f)
			if (x != 0.0f) {
				_anal_window[i] *= _fftlen * sin(M_PI * x / _fftlen) / (M_PI * x);
				if (_decimation)
					_synth_window[i] *= _decimation * sin(M_PI * x / _decimation)
					                                               / (M_PI * x);
			}
	}

	// Normalize windows for unity gain across unmodified
	// analysis-synthesis procedure.

	float sum = 0.0f;
	for (int i = 0; i < _window_len; i++)
		sum += _anal_window[i];

	for (int i = 0; i < _window_len; i++) {
		float afac = 2.0f / sum;
		float sfac = _window_len > _fftlen ? 1.0f / afac : afac;
		_anal_window[i] *= afac;
		_synth_window[i] *= sfac;
	}

	if (_window_len <= _fftlen && _decimation) {
		sum = 0.0f;
		for (int i = 0; i < _window_len; i += _decimation)
			sum += _synth_window[i] * _synth_window[i];
		sum = 1.0f / sum;
		for (int i = 0; i < _window_len; i++)
			_synth_window[i] *= sum;
	}

	return 0;
}


// ---------------------------------------------------------------- set_srate --
// Must call this before anything that updates bin groups in subclass.
void SpectacleBase::set_srate(float srate)
{
	if (srate != _srate) {
		_srate = srate;
		if (_maxfreq == _nyquist)
			_maxfreq = 0.0f;
		_nyquist = srate / 2;
		if (_maxfreq == 0.0f)
			_maxfreq = _nyquist;
		_fund_anal_freq = srate / float(_fftlen);
		update_bin_groups(_bin_groups, NULL, _minfreq, _maxfreq,
		                                               _control_table_size);
	}
}


// --------------------------------------------------------------------- init --
int SpectacleBase::init(int fftlen, int windowlen, int overlap, float srate)
{
	_fftlen = fftlen;
	_window_len = windowlen;
	_overlap = overlap;

	// Make sure FFT length is a power of 2 <= kMaxFFTLen.
	bool valid = false;
	for (int x = 1; x <= kMaxFFTLen; x *= 2) {
		if (_fftlen == x) {
			valid = true;
			break;
		}
	}
	if (!valid) {
		post("%s: FFT length must be a power of two <= %d. Setting to 1024...",
			instname(), kMaxFFTLen);
		_fftlen = 1024;
	}

	_half_fftlen = _fftlen / 2;

	// Make sure window length is a power of 2 >= FFT length.
	valid = false;
	for (int x = _fftlen; x <= kMaxWindowLen; x *= 2) {
		if (_window_len == x) {
			valid = true;
			break;
		}
	}
	if (!valid) {
		post("%s: Window length must be a power of two >= FFT length (%d)\n"
		           "and <= %d. Setting to 2048...",
				instname(), _fftlen, kMaxWindowLen);
		_window_len = _fftlen * 2;
	}

	// Make sure _overlap is a power of 2 in allowed range.
	valid = false;
	for (int x = kMinOverlap; x <= kMaxOverlap; x *= 2) {
		if (_overlap == x) {
			valid = true;
			break;
		}
	}
	if (!valid) {
		post("%s: Overlap must be a power of two between %d and %d. "
				"Setting to 2...",
		      instname(), kMinOverlap, kMaxOverlap);
		_overlap = 2;
	}

	_bin_groups = new int [_half_fftlen];

	set_srate(srate);
	set_freqrange(0.0f, 0.0f);

	// derive decimation from overlap
	_decimation = int(_fftlen / _overlap);

	_window_len_minus_decimation = _window_len - _decimation;

	DPRINT2("_fftlen=%d, _decimation=%d", _fftlen, _decimation);

	_input = new float [_window_len];            // interior input buffer
	_output = new float [_window_len];           // interior output buffer
	if (_input == NULL || _output == NULL)
		return -1;
	for (int i = 0; i < _window_len; i++)
		_input[i] = _output[i] = 0.0f;

	// Read index chases write index by _decimation; add 2 extra locations to
	// keep read point from stepping on write point.  Verify with asserts in
	// increment_out_*_index().
	_outframes = _decimation + 2;
	_out_read_index = _outframes - _decimation;
	_out_write_index = 0;
	_outbuf = new float [_outframes];
	if (_outbuf == NULL)
		return -1;
	for (int i = 0; i < _outframes; i++)
		_outbuf[i] = 0.0f;
	DPRINT1("_outframes: %d", _outframes);

	_anal_window = new float [_window_len];
	_synth_window = new float [_window_len];
	if (_anal_window == NULL || _synth_window == NULL)
		return -1;
	if (make_windows() != 0)
		return -1;

	_bucket = new Obucket(_decimation, process_wrapper, (void *) this);

	_fft = new Offt(_fftlen);
	_fft_buf = _fft->getbuf();

	return 0;
}


// -------------------------------------------------------------------- clear --
void SpectacleBase::clear()
{
	for (int i = 0; i < _fftlen; i++)
		_fft_buf[i] = 0.0f;
	for (int i = 0; i < _window_len; i++)
		_input[i] = _output[i] = 0.0f;
	for (int i = 0; i < _outframes; i++)
		_outbuf[i] = 0.0f;
	_bucket->clear();
}


// ------------------------------------------------------------ prepare_input --
// <buf> contains <_decimation> samples (mono) from most recent input.

void SpectacleBase::prepare_input(const float buf[])
{
	DPRINT("prepare_input");

	// Shift samples in <_input> from right to left by <_decimation>,
	// leaving a hole of <_decimation> slots at right end.

	for (int i = 0; i < _window_len_minus_decimation; i++)
		_input[i] = _input[i + _decimation];

	// Copy <_decimation> samples from <buf> to right end of <_input>.

	for (int i = _window_len_minus_decimation, j = 0; i < _window_len; i++, j++)
		_input[i] = buf[j];

	// Multiply input array by analysis window, both of length <_window_len>.
	// Fold and rotate windowed real input into FFT buffer.

	for (int i = 0; i < _fftlen; i++)
		_fft_buf[i] = 0.0f;
	int j = _cursamp % _fftlen;
	for (int i = 0; i < _window_len; i++) {
		_fft_buf[j] += _input[i] * _anal_window[i];
		if (++j == _fftlen)
			j = 0;
	}
}


// ----------------------------------------------------------- prepare_output --
void SpectacleBase::prepare_output()
{
	DPRINT("prepare_output");

	// overlap-add <_fft_buf> real data into <_output>
	int j = _cursamp % _fftlen;
	for (int i = 0; i < _window_len; i++) {
		_output[i] += _fft_buf[j] * _synth_window[i];
		if (++j == _fftlen)
			j = 0;
	}

	// transfer samples from <_output> to outer output buffer.
	for (int i = 0; i < _decimation; i++) {
		_outbuf[_out_write_index] = _output[i];
		increment_out_write_index();
	}

	// shift samples in <_output> from right to left by <_decimation>
	for (int i = 0; i < _window_len_minus_decimation; i++)
		_output[i] = _output[i + _decimation];
	for (int i = _window_len_minus_decimation; i < _window_len; i++)  
		_output[i] = 0.0f;
}


// ---------------------------------------------------------- process_wrapper --
// Called by Obucket whenever the input bucket is full (i.e., has _decimation
// samps).  This static member wrapper function lets us call a non-static member
// function, which we can't pass directly as a callback to Obucket.
// See http://www.newty.de/fpt/callback.html for one explanation of this.

void SpectacleBase::process_wrapper(const float buf[], const int len,
                                                                  void *obj)
{
	SpectacleBase *myself = (SpectacleBase *) obj;
	myself->process(buf, len);
}


// ------------------------------------------------------------------ process --
void SpectacleBase::process(const float *buf, const int)
{
	DPRINT("SpectacleBase::process");

   if (_reading_input) {
      prepare_input(buf);
      _fft->r2c();
   }
	modify_analysis(_reading_input);
	_fft->c2r();
	prepare_output();
}


// ------------------------------------------------------------ set_freqrange --
// Update the frequency range within which control tables operate.  Return true
// if range has actually changed, resulting in an update_bin_groups call.
// Otherwise, return false.

bool SpectacleBase::set_freqrange(float min, float max)
{
	if (min != _minfreq || max != _maxfreq) {
		if (max == 0.0f)
			max = _nyquist;
		if (max < min)
			_fswap(&min, &max);
		if (max > _nyquist)
			max = _nyquist;
		_maxfreq = max;
		_minfreq = _fclamp(0.0f, min, max);
		update_bin_groups(_bin_groups, NULL, _minfreq, max, _control_table_size);
		return true;
	}
	return false;
}


// -------------------------------------------------------- update_bin_groups --
// Update a bin groups array.  (See .h for a description of bin groups.)  Note 
// that this is called from set_freqrange and any subclass equivalents.
// Three methods of updating bin groups array: (1) use a binmap table,
// (2) use a one-to-one mapping of bin groups to bins, or (3) use a non-linear
// mapping that starts as one-to-one, but combines bins in ever larger
// groups as bin frequency increases.  The <binmaptable> argument is an 
// optional binmap table to use in place of the base class <_binmaptable>. 
// If not overriding the base class table, pass NULL.

void SpectacleBase::update_bin_groups(
	int bin_groups[],
	int binmaptable[],
	const float minfreq,
	const float maxfreq,
	const int control_table_size)
{
	if (control_table_size == 0)				// no control table
		return;

	const int nbins = _half_fftlen;
	const int *bmtable = binmaptable ? binmaptable : _binmaptable;
	
	// The optional <bmtable> has <control_table_size> elements, each giving
	// the number of adjacent FFT bins controlled by that element in any of the
	// control tables (e.g., EQ).  Copy this information into the <bin_groups>
	// array, which is sized to <nbins>.  We ignore minfreq and maxfreq.

	if (bmtable != NULL) {
#ifdef DEBUG_BINGROUPS
		post("using binmap table -----------------------------");
		for (int i = 0; i < control_table_size; i++)
			post("[%d] %d", i, bmtable[i]);
#endif
		int bidx = 0;
		for (int i = 0; i < control_table_size; i++) {
			int bincount = bmtable[i];
			while (bincount-- > 0) {
				if (bidx == nbins)
					goto highcount;
				bin_groups[bidx++] = i;
			}
		}
highcount:
		if (bidx < nbins) {
			const int last = control_table_size - 1;
			while (bidx < nbins)
				bin_groups[bidx++] = last;
		}
	}

	// If there is no binmap table...
	// If freq. range is full bandwidth, and control table size is >= to the
	// number of bins, then use linear mapping of control table slots to bins,
	// as in SPECTACLE v1.  If table is larger than the number of bins, warn 
	// about ignoring the extra table slots.

	else if (control_table_size >= nbins
				&& minfreq == 0 && maxfreq == _nyquist) {
		for (int i = 0; i < nbins; i++)
			bin_groups[i] = i;

		if (control_table_size > nbins) {
			const int ignorevals = control_table_size - nbins;
			if (ignorevals != _prev_bg_ignorevals) {
				post("%s: Control table of size %d too large for "
			                    "frequency range...ignoring last %d values",
			                    instname(), control_table_size, ignorevals);
				_prev_bg_ignorevals = ignorevals;
			}
		}
	}

	// Otherwise, there is either a freq. range that is not full bandwidth, or
	// the control table size is less than the number of bins.  If the former,
	// we insure that the first array slot affects all bins whose frequencies
	// are below (and possibly a little above) the min. freq., and that the last
	// array slot affects all bins whose frequencies are above (and possibly a
	// little below) the max. freq.  If the control table is smaller than the
	// number of bins within the freq. range, then we construct a one-to-one or
	// one-to-many mapping of control table slots to bins, arranged so that we
	// can control lower frequencies with greater resolution.
	//
	// Depending on the difference between the control table size and the number
	// of FFT bins, we create a mapping that begins linearly and grows
	// arithmetically after a certain point.  The purpose is to get higher
	// resolution in the lower frequencies of the range, where it matters most.
	// So near the low end of the range, there is a one-to-one mapping of control
	// array slots to FFT bins.  Near the high end of the range, one control
	// array slot affects many FFT bins.  We attempt to transition smoothly
	// between these two extremes.  As an example, the number of FFT bins
	// controlled by each slot of a control table array might look like this:
	//
	//            number of bins spanned by each cntl slot
	// (lowshelf) 1 1 1 1 1 1 1 1 1 1 1 2 3 4 5 6 7 8 9 10 14 (highshelf)
	//
	// The scheme isn't perfect -- e.g., the penultimate slot might control more
	// than you would expect -- but it does guarantee that each control table
	// slot affects at least one FFT bin.

	else {
		const int lowshelfbin = closest_bin(minfreq);
		int highshelfbin = closest_bin(maxfreq);
		bool maxfreq_is_nyquist = false;
		if (highshelfbin == nbins) {
			maxfreq_is_nyquist = true;
			highshelfbin--;		// we don't control the nyquist bin
		}
		int cntltablen = control_table_size;
		const float endflatrange = minfreq + (cntltablen * _fund_anal_freq);
		bool linear_map = true;
		if (endflatrange > maxfreq - _fund_anal_freq) {
			const int ignorevals = int((endflatrange - maxfreq)
			                                                 / _fund_anal_freq);
			if (ignorevals > 0) {
				cntltablen = control_table_size - ignorevals;
				if (ignorevals != _prev_bg_ignorevals) {
					post("%s: Control table of size %d too large for "
			                       "frequency range...ignoring last %d values",
			                       instname(), control_table_size, ignorevals);
					_prev_bg_ignorevals = ignorevals;
				}
			}
		}
		else
			linear_map = false;
#ifdef DEBUG_BINGROUPS
		post("lowbin=%d, highbin=%d, endflatrange=%f, map=%s",
					lowshelfbin, highshelfbin, endflatrange,
					linear_map ? "linear" : "nonlinear");
#endif

		// assign low shelf group
		int bidx = 0;
		while (bidx <= lowshelfbin)
			bin_groups[bidx++] = 0;

		// assign interior groups
		if (linear_map) {
			const int lastbingroup = maxfreq_is_nyquist ? cntltablen - 1
			                                            : cntltablen - 2;
			int bingroup = 1;
			while (bidx < highshelfbin) {
				bin_groups[bidx++] = bingroup;
				if (bingroup < lastbingroup)
					bingroup++;
			}
		}
		else {	// higher array slots control more and more bins
			const int extrabins = (highshelfbin - lowshelfbin) - cntltablen;
			if (extrabins <= 0)
				error("%s: program error - contact johgibso@gmail.com", instname());
			const int nsums = int(sqrt(2 * extrabins) + 2);

			// Compute the sum of integers for n in [nsums-2, nsums);
			// formula is the closed-form equation: x = n * (n + 1) / 2.
			const int an = nsums - 2;
			const int bn = nsums - 1;
			const int a = (an * (an + 1)) / 2;
			const int b = (bn * (bn + 1)) / 2;
			int cntlspan = 0, binspan = 0;
			if (a > extrabins) {
				cntlspan = an;
				binspan = a;
			}
			else if (b > extrabins) {
				cntlspan = bn;
				binspan = b;
			}
			else		// this should never happen
				error("%s: program error - contact johgibso@gmail.com", instname());

#ifdef DEBUG_BINGROUPS
			post("nbins=%d, lowbin=%d, highbin=%d, tablen=%d, extrabins=%d, "
				"cntlspan=%d, binspan=%d, maxfreqisnyq=%d\n",
				nbins, lowshelfbin, highshelfbin, cntltablen, extrabins,
				cntlspan, binspan, maxfreq_is_nyquist);
#endif

			// first, linear mapping
			const int end = (cntltablen - cntlspan) - 1;
			int bingroup = 1;
			while (bingroup < end)
				bin_groups[bidx++] = bingroup++;

			// then map using sum-of-integers series
			const int lastbingroup = maxfreq_is_nyquist ? cntltablen - 1
			                                            : cntltablen - 2;
			int incr = 1;
			int count = 0;
			while (bidx < highshelfbin) {
				bin_groups[bidx++] = bingroup;
				if (++count == incr) {
					count = 0;
					if (bingroup < lastbingroup)
						bingroup++;
					incr++;
				}
			}
		}

		// assign high shelf group
		const int last = cntltablen - 1;
		while (bidx < nbins)
			bin_groups[bidx++] = last;
	}

#ifdef DEBUG_BINGROUPS
	post("bin_groups[] -----------------------------");
	for (int i = 0; i < nbins; i++)
		post("[%d] %d (%f)", i, bin_groups[i], i * _fund_anal_freq);
#endif
#ifdef CHECK_BINGROUPS
	for (int i = 1; i < nbins; i++) {
		int diff = bin_groups[i] - bin_groups[i - 1];
		if (diff < 0 || diff > 1)
			error("%s: bin group (%p) index %d not 0 or 1 greater than prev entry",
			       instname(), bin_groups, i);
	}
#endif
}


// --------------------------------------------------------- print_bin_groups --
// print a user-readable listing of bin groups
void SpectacleBase::print_bin_groups(
	int bin_groups[],
	const char *type)		// to identify bin groups to user
{
	const int nbins = _half_fftlen;

	post("\n%s:  %s table bin groups", instname(), type);
	post("------------------------------------------");
	int cntltabslot = 0;
	int startbin = 0;
	for (int i = 0; i < nbins; i++) {
		const int thisslot = bin_groups[i];
		if (thisslot > cntltabslot) {
			// print stats for previous control table slot <cntltabslot>
			const int startfreq = int(_fund_anal_freq * startbin + 0.5f);
			if (i - startbin > 1) {
				const int endfreq = int(_fund_anal_freq * (i - 1) + 0.5f);
				post("  [%d]\t%d-%d Hz (%d bins)",
							 cntltabslot, startfreq, endfreq, i - startbin);
			}
			else
				post("  [%d]\t%d Hz", cntltabslot, startfreq);
			cntltabslot = thisslot;
			startbin = i;
		}
	}
}


// ---------------------------------------------------------------------- run --
void SpectacleBase::run(float in[], float out[], int nframes)
{
	if (_reading_input) {
		for (int i = 0; i < nframes; i++) {
			_bucket->drop(in[i]);	// may process <_decimation> input frames
			out[i] = _outbuf[_out_read_index];
			increment_out_read_index();
			_cursamp++;
		}
	}
	else {
		for (int i = 0; i < nframes; i++) {
			_bucket->drop(0.0f);		// may process <_decimation> input frames
			out[i] = _outbuf[_out_read_index];
			increment_out_read_index();
			_cursamp++;
		}
	}
}


