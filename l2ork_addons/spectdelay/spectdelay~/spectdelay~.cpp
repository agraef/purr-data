/*
 *  Copyright (C) 2008 John Gibson
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

// Documentation, in ref manual format
//
// Arguments
//
//    int      Optional. FFT length (power of 2, usually 1024)
//             [if missing, fftlen=1024, windowlen=2048, overlap=2, maxdel=6]
//
//    int      Optional. Window length (power of 2, usually FFT length * 2)
//             [if missing, windowlen=2048, overlap=2, maxdel=6]
//
//    int      Optional. FFT window overlap (positive power of 2)
//             1: no overlap, 2: hopsize=FFTlen/2, 4: hopsize=FFTlen/4, etc.
//             2 or 4 is usually fine; 1 is fluttery; 4 gives fewer artifacts;
//             higher overlaps use more CPU; 
//             [if missing, overlap=2, maxdel=6]
//
//    float    Optional. Maximum delay time (seconds)
//             [if missing, maxdel=6]
//
// Input
//
//    signal   Audio signal in left inlet to be processed by jg.spectdelay~.
//
//    binmap   In left inlet: The word binmap, followed by a list of ints,
//             which gives the number of adjacent FFT bins to affect for each
//             of the elements of the EQ table. The binmap table and the EQ
//             table must be the same size. Giving the word binmap alone
//             cancels the use of a binmap table. For example, the message
//             "binmap 1 2 10 20 50" assumes a previous eq message of five 
//             floats and treats the first of those floats as the gain of the 
//             lowest FFT bin, the second as the gain of the next two FFT bins,
//             the third as the gain of the next ten FFT bins, the fourth as
//             the gain of the next 20 FFT bins, and the fifth as the gain of
//             the next 50 FFT bins (and any leftover bins above these).
//
//     clear   In left inlet: The word clear. This fills all the spectral delay
//             lines with zero, resulting in silence.
//
//    dbinmap  In left inlet: The word dbinmap, followed by a list of ints,
//             which gives the number of adjacent FFT bins to affect for each
//             of the elements of the delay time and feedback tables. The delay
//             binmap table and these tables must be the same size. Giving the
//             word dbinmap alone cancels the use of a delay binmap table. The
//             mapping scheme functions in the way described for the binmap
//             message.
//
//    drange   In left inlet: The word drange, followed by two floats (min
//             and max), which define the frequency range within which the delay
//             time (del) and feedback (feed) tables function. Giving 0 for max
//             sets the maximum frequency to Nyquist. By default, the range is
//             0 Hz to Nyquist. Note that this range is ignored when using the
//             binmap table.
//
//        dt   In left inlet: The word dt, followed by a list of floats, which
//             define the delay times (in seconds) for individual frequency
//             bands. Effect depends on delay frequency range or dbinmap table.
//             If the list contains just one number, then that is applied to
//             all bands.
//
//        dx   In left inlet: The word dx, followed by an index (int) and 
//             delay time (float) to replace the value at that index in the
//             currently defined delay time table. The table must already have
//             at least index + 1 values.
//
//        eq   In left inlet: The word eq, followed by a list of floats,
//             which define the amplitude scaling for individual frequency
//             bands, in decibels. (0 dB means no change, + dB boost, - dB cut.)
//             Effect depends on EQ frequency range or binmap table. If the list
//             contains just one number, then that is applied to all bands.
//
//        ex   In left inlet: The word ex, followed by an index (int) and EQ
//             amplitude in dB (float) to replace the value at that index in the
//             currently defined EQ table. The table must already have
//             at least index + 1 values.
//
//        fb   In left inlet: The word fb, followed by a list of floats, which
//             define the delay feedback multiplier (-1 to 1, inclusive) for
//             individual frequency bands. Effect depends on delay frequency
//             range or dbinmap table. If the list contains just one number,
//             then that is applied to all bands. If the feedback list has more
//             than one item, the size of the list must match the size of the
//             delay time (dt) list. If changing both list sizes, change the
//             delay time list first.
//
//        fx   In left inlet: The word fx, followed by an index (int) and 
//             delay feedback (float) to replace the value at that index in the
//             currently defined delay feedback table. The table must already
//             have at least index + 1 values.
//
//       fft   In left inlet: The word fft, followed by three ints, gives
//             the FFT length, window length, and overlap (respectively).
//             (See explanation above in Arguments secion.)
//             Note: sending this message will halt processing while object
//             reconfigures itself and will cause an audio glitch.
//
//      hold   In left inlet: The word hold, followed by a 0 or a 1.  If 0,
//             play normally.  If 1, stop accepting input and allow the sound
//             already in delay lines to recirculate.  (If instead you let the
//             input decay to zeros, without holding, you may hear the delay
//             lines resonate pitches corresponding to the fundamental frequency
//             of analysis and its harmonic partials.)
//
//    maxdel   In left inlet: The word maxdel, followed by a float giving the
//             maximum number of seconds for the delay lines.
//
//    posteq   In left inlet: The word posteq, followed by a 0 or a 1.  If 0,
//             EQ is applied before delay.  If 1, EQ is applied after delay,
//             which means it affects sound that's feeding back through the
//             delay lines.
//
//     range   In left inlet: The word range, followed by two floats (min
//             and max), which define the frequency range within which the EQ
//             table functions. Giving 0 for max sets the maximum frequency to
//             Nyquist. By default, the range is 0 Hz to Nyquist. Note that this
//             range is ignored when using the binmap table.
//
// NOTE: It takes a certain, fixed amount of time for the sound to enter
// jg.spectdelay~ and emerge processed. If you want to delay your dry signal so
// that it synchronizes with the processed signal, delay it by the window length
// (i.e., the second argument to jg.spectdelay~).

// Changes
//
// v0.9		- fixed crashes seen on Snow Leopard, due to uninitialized pointers. 


//extern "C" {
	//#include "ext.h"gedi
	//#include "z_dsp.h"
	//#include "ext_strings.h"
	//#include "/home/ico/Downloads/PureData/pure-data/pd/src/m_pd.h"
//}

//#include <math.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

#include "Spectacle.h"
#include "../../../pd/src/m_pd.h"

// conform out-of-range values passed to dt, dx, fb, and fx
#define CONFORM_INPUT

const int kDefaultFFTLen = 1024;
const int kDefaultWindowLen = 2048;
const int kDefaultOverlap = 2;
const float kDefaultMaxDelTime = 6.0f;
const float kMinMaxDelTime = 0.1f;
const float kMaxMaxDelTime = 20.0f;
const int kMaxTableLen = 512;

//void *spectdelay_class;

static t_class *spectdelay_class;

typedef struct _spectdelay
{
	// header
	t_object x_obj;
    
	// variables specific to this object
	int fftlen, windowlen, overlap;
	float srate, maxdeltime;
	Spectacle *spectdelay, *newspectdelay;
	float eqtable[kMaxTableLen];
	float dttable[kMaxTableLen];
	float fbtable[kMaxTableLen];
	int eqbinmap[kMaxTableLen];
	int delaybinmap[kMaxTableLen];
	int eqtablen, dttablen, fbtablen, eqbinmaplen, delaybinmaplen;
	float minfreq, maxfreq, dminfreq, dmaxfreq;
	bool in_connected, hold, posteq;

	t_float x_f;

} t_spectdelay;

// prototypes
void *spectdelay_new(float fftlen, float windowlen, float overlap, 
	float maxdeltime);
void spectdelay_free(t_spectdelay *x);
void spectdelay_dsp(t_spectdelay *x, t_signal **sp); 
t_int *spectdelay_perform(t_int *w);
void spectdelay_assist(t_spectdelay *x, void *b, long m, long a, char *s);
// our message handlers
void spectdelay_binmap_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_clear_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_dbinmap_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_drange_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_dt_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_dx_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_eq_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_ex_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_fb_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_fx_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_fft_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_hold_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_posteq_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_maxdel_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);
void spectdelay_range_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv);


// utilities

inline float _fclamp(float min, float val, float max)
{
	return (val < min) ? min : ((val > max) ? max : val);
}

inline long _atom_getlong(int index, short argc, t_atom *argv)
{
	/*if (argv[index].a_type == A_LONG)
		return argv[index].a_w.w_long;
	else*/ if (argv[index].a_type == A_FLOAT)
		return (long) argv[index].a_w.w_float;
	else return 0L;
}

inline float _atom_getfloat(int index, short argc, t_atom *argv)
{
	/*if (argv[index].a_type == A_LONG)
		return (float) argv[index].a_w.w_long;
	else */ if (argv[index].a_type == A_FLOAT)
		return argv[index].a_w.w_float;
	else return 0.0;
}


// Called the first time object is inserted or read from a file
// during a Max/MSP run.  It is *not* called on subsequent object
// creation during a run.

//extern "C" int main(void)
extern "C" void spectdelay_tilde_setup(void)
{
	// The three optional A_DEFLONG arguments give the FFT length, window length
	// and window overlap.
	//setup((struct messlist **) &spectdelay_class, (method) spectdelay_new,
	//			(method) spectdelay_free, (short) sizeof(t_spectdelay), 0L,
	//			A_DEFLONG, A_DEFLONG, A_DEFLONG, A_DEFFLOAT, 0);
	spectdelay_class = class_new(gensym("spectdelay~"), (t_newmethod)
				spectdelay_new, (t_method) spectdelay_free, (short) sizeof
				(t_spectdelay), 0, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
	CLASS_MAINSIGNALIN(spectdelay_class, t_spectdelay, x_f);

	// standard messages; don't change these
	class_addmethod(spectdelay_class, (t_method)spectdelay_dsp, gensym("dsp"), A_CANT, 0);
	//class_addmethod(spectdelay_class, spectdelay_assist, gensym("assist"), 0);

	// our own messages
	class_addmethod(spectdelay_class, (t_method)spectdelay_binmap_msg, gensym("binmap"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_clear_msg, gensym("clear"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_dbinmap_msg, gensym("dbinmap"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_drange_msg, gensym("drange"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_dt_msg, gensym("dt"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_dx_msg, gensym("dx"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_eq_msg, gensym("eq"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_ex_msg, gensym("ex"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_fb_msg, gensym("fb"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_fx_msg, gensym("fx"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_fft_msg, gensym("fft"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_hold_msg, gensym("hold"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_posteq_msg, gensym("posteq"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_maxdel_msg, gensym("maxdel"), A_GIMME, 0);
	class_addmethod(spectdelay_class, (t_method)spectdelay_range_msg, gensym("range"), A_GIMME, 0);

	//dsp_initclass();

	post("spectdelay~ v0.9 - Copyright (C) 2008-2010 by John Gibson\nPd port by Ivica Ico Bukvic 2010");
}


// Called when the object is created.  Note that every time the user edits
// arguments, Max calls _free and then _new to recreate the object.

void *spectdelay_new(float fftlen, float windowlen, float overlap,
	float maxdeltime)
{
	// create our object
	t_spectdelay *x = (t_spectdelay *)pd_new(spectdelay_class);

	// setup up audio signal inputs and outputs
	//dsp_setup((t_object *) x, 1);
	//inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	//outlet_new((t_object *) x, "signal");
	outlet_new(&x->x_obj, gensym("signal"));

	// Normally, an out vector points to the same memory as an in
	// vector, and code must be designed to write an output sample
	// only after reading the corresponding input sample.  If this
	// is not possible, uncomment the following line, which makes the
	// in and out buffers independent.
	// x->x_obj.z_misc = Z_NO_INPLACE;

	// init our struct fields
	x->fftlen = fftlen ? (int)fftlen : kDefaultFFTLen;
	x->windowlen = windowlen ? (int)windowlen : kDefaultWindowLen;
	x->overlap = overlap ? (int)overlap : kDefaultOverlap;
	x->maxdeltime = maxdeltime ? maxdeltime : kDefaultMaxDelTime;
	x->maxdeltime = _fclamp(kMinMaxDelTime, x->maxdeltime, kMaxMaxDelTime);
	x->srate = sys_getsr();		// possibly overridden in spectdelay_dsp
	if (x->srate == 0.0)
		x->srate = 44100.0;
	x->spectdelay = NULL;
	float *del = x->dttable;
	float *eq = x->eqtable;
	float *feed = x->fbtable;
	int *bm = x->eqbinmap;
	int *dbm = x->delaybinmap;
	for (int i = 0; i < kMaxTableLen; i++) {
		*del++ = 0.0;
		*eq++ = 0.0;
		*feed++ = 0.0;
		*bm++ = 0;
		*dbm++ = 0;
	}
	x->dttablen = x->eqtablen = x->fbtablen = 0;
	x->eqbinmaplen = x->delaybinmaplen = 0;
	x->dminfreq = x->dmaxfreq = x->minfreq = x->maxfreq = 0.0;
	x->in_connected = true;
	x->hold = false;
	x->posteq = false;
	x->newspectdelay = NULL;

	x->spectdelay = new Spectacle();
	x->spectdelay->init(x->fftlen, x->windowlen, x->overlap, x->srate,
	                                                        x->maxdeltime);
	x->x_f = 0;

	return x;
}


// Called when an object is destroyed

void spectdelay_free(t_spectdelay *x)
{
	// Must call this *before* you free other resources!
	//dsp_free((t_object *) x);
	
	delete x->spectdelay;
	delete x->newspectdelay;	// in case this happens to be alive
}


// Called every time audio is started.  Even when audio is running, if the
// user changes anything (like deletes a patch cord), audio will be turned off
// and then on again, calling this func.
// This adds the "perform" method to the DSP chain, and also tells us
// where the audio vectors are and how big they are.

#define NUM_DSP_ADD_ARGS 4

void spectdelay_dsp(t_spectdelay *x, t_signal **sp)
{
	int i;

	// set sample rate vars, in case srate is different from global rate
	const float newsrate = sp[0]->s_sr;
	if (newsrate != x->srate) {
		x->srate = newsrate;
		x->spectdelay->set_srate(x->srate);
	}

	// check to see if there are signals connected to the input
	//x->in_connected = (bool) count[0];
	//x->in_connected = (???te_inlet->i_symfrom ? 1 : 0);

	/*t_int* dsp_add_args[4];
	dsp_add_args[0] = (t_int*)x;			// pointer to self
	dsp_add_args[1] = (t_int*)sp[0]->s_n;	// vector size

	for (i=0;i < NUM_DSP_ADD_ARGS-2;i++)
	    dsp_add_args[2 + i] = (t_int*)sp[i]->s_vec;

	//dsp_add_args[2] = (t_int*)sp[0]->s_vec;	// input vector
	//dsp_add_args[3] = (t_int*)sp[1]->s_vec;	// output vector

	// add vectors to signal chain
	dsp_addv(spectdelay_perform, NUM_DSP_ADD_ARGS, (t_int*)dsp_add_args);
	//freebytes(dsp_add_args,sizeof(t_int)*NUM_DSP_ADD_ARGS); */
	dsp_add(spectdelay_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}


// Called to process or generate a block of samples

t_int *spectdelay_perform(t_int *w)
{
	t_spectdelay *x = (t_spectdelay *) w[1];

	// Don't do anything else if patcher is "muted"
	if (/*x->x_obj.z_disabled ||*/ x->in_connected)
	{

		// in and out vectors
		float *in = (float *) w[2];
		float *out = (float *) w[3];

		// number of samples per vector
		const int vsize = (int) w[4];

		// If spectdelay_fft_msg has created a replacement Spectacle, use it.
		if (x->newspectdelay) {
			Spectacle *tmp = x->spectdelay;
			x->spectdelay = x->newspectdelay;
			x->newspectdelay = NULL;
			delete tmp;
		}

		// process a block of samples
		x->spectdelay->run(in, out, vsize);

	}

	// return a pointer to the next object in the signal chain.
	return w + NUM_DSP_ADD_ARGS + 1;
}	


// Called when user mouses over inlets/outlets, which shows help blurbs.

void spectdelay_assist(t_spectdelay *x, void *b, long m, long a, char *s)
{
	/*if (m == ASSIST_INLET) {
		if (a == 0) sprintf(s, "input (signal), messages");
	}
	else {
		if (a == 0) sprintf(s, "output (signal)");
	}*/
}


// === our message handlers ====================================================

void spectdelay_binmap_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc == 0) {
		post("spectdelay~: cancelling EQ binmap table");
		for (int i = 0; i < x->eqbinmaplen; i++)
			x->eqbinmap[i] = 0;
		x->eqbinmaplen = 0;
		x->spectdelay->set_binmap_table(NULL, 0);
		return;
	}
	else if (argc > kMaxTableLen) {
		post("spectdelay~ warning: EQ binmap list can have no more than "
				"%d values", kMaxTableLen);
		argc = kMaxTableLen;
	}
	if (x->eqtablen != 1 && argc != x->eqtablen) {
		error("spectdelay~: EQ binmap list must have same number of values "
				"as the eq list");
		return;
	}
	x->eqbinmaplen = argc;

	int *tab = (int *) &x->eqbinmap;
	for (int i = 0; i < argc; i++)
		tab[i] = _atom_getlong(i, argc, argv);

	x->spectdelay->set_binmap_table(x->eqbinmap, x->eqbinmaplen);

	const float nyquist = x->srate / 2;
	if (x->minfreq != 0 || (x->maxfreq != 0 && x->maxfreq != nyquist))
		post("spectdelay~ warning: use of EQ binmap table ignores min "
           "and max EQ frequencies");
}


void spectdelay_clear_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
   x->spectdelay->clear();
}


void spectdelay_dbinmap_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc == 0) {
		post("spectdelay~: cancelling delay binmap table");
		for (int i = 0; i < x->delaybinmaplen; i++)
			x->delaybinmap[i] = 0;
		x->delaybinmaplen = 0;
		x->spectdelay->set_delay_binmap_table(NULL, 0);
		return;
	}
	else if (argc > kMaxTableLen) {
		post("spectdelay~ warning: delay binmap list can have no more "
           "than %d values", kMaxTableLen);
		argc = kMaxTableLen;
	}
	if ((x->dttablen != 1 && argc != x->dttablen)
	    || (x->fbtablen != 1 && argc != x->fbtablen)) {
		error("spectdelay~: delay binmap list must have same number of values "
            "as the delay time and feedback lists");
		return;
	}
	x->delaybinmaplen = argc;

	int *tab = (int *) &x->delaybinmap;
	for (int i = 0; i < argc; i++)
		tab[i] = _atom_getlong(i, argc, argv);

	x->spectdelay->set_delay_binmap_table(x->delaybinmap, x->delaybinmaplen);

	const float nyquist = x->srate / 2;
	if (x->dminfreq != 0 || (x->dmaxfreq != 0 && x->dmaxfreq != nyquist))
		post("spectdelay~ warning: use of delay binmap table ignores min "
           "and max delay frequencies");
}


void spectdelay_drange_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc != 2) {
		error("spectdelay~: drange min max (delay time frequency range "
				"in Hz)");
		return;
	}
	const float nyquist = x->srate / 2;
	float max = _atom_getfloat(1, argc, argv);
	if (max == 0)
		max = nyquist;
	else
		max = _fclamp(0.0, max, nyquist);
	float min = _fclamp(0.0, _atom_getfloat(0, argc, argv), max);

	// changing range is expensive, so avoid when params are same
	if (min != x->dminfreq || max != x->dmaxfreq) {
		x->dminfreq = min;
		x->dmaxfreq = max;
		x->spectdelay->set_delay_freqrange(min, max);
	}
}


void spectdelay_dt_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc < 1) {
		error("spectdelay~: delay time list must have at least 1 value");
		return;
	}
	if (argc > kMaxTableLen) {
		post("spectdelay~ warning: delay time list can have no more than %d "
				"values", kMaxTableLen);
		argc = kMaxTableLen;
	}
	x->dttablen = argc;

	float *tab = (float *) &x->dttable;
#ifdef CONFORM_INPUT
	const float maxdeltime = x->maxdeltime;
	for (int i = 0; i < argc; i++)
		tab[i] = _fclamp(0.0f, _atom_getfloat(i, argc, argv), maxdeltime);
#else
	for (int i = 0; i < argc; i++)
		tab[i] = _atom_getfloat(i, argc, argv);
#endif
	x->spectdelay->set_deltable(x->dttable, x->dttablen);
}


void spectdelay_dx_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc != 2) {
		error("spectdelay~: dx index value");
		return;
	}
	const int index = _atom_getlong(0, argc, argv);
	if (index >= x->dttablen) {
		error("spectdelay~: dx index out of range");
		return;
	}
#ifdef CONFORM_INPUT
	float value = _fclamp(0.0f, _atom_getfloat(1, argc, argv), x->maxdeltime);
#else
	float value = _atom_getfloat(1, argc, argv);
#endif
	x->dttable[index] = value;	// update array read by Spectacle object
}


void spectdelay_eq_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc < 1) {
		error("spectdelay~: eq list must have at least 1 value");
		return;
	}
	if (argc > kMaxTableLen) {
		post("spectdelay~ warning: eq list can have no more than %d values",
				kMaxTableLen);
		argc = kMaxTableLen;
	}
	x->eqtablen = argc;

	float *tab = (float *) &x->eqtable;
	for (int i = 0; i < argc; i++)
		tab[i] = _atom_getfloat(i, argc, argv);

	x->spectdelay->set_eqtable(x->eqtable, x->eqtablen);
}


void spectdelay_ex_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc != 2) {
		error("spectdelay~: ex index value");
		return;
	}
	const int index = _atom_getlong(0, argc, argv);
	if (index >= x->eqtablen) {
		error("spectdelay~: ex index out of range");
		return;
	}
	float value = _atom_getfloat(1, argc, argv);
	x->eqtable[index] = value;	// update array read by Spectacle object
}


void spectdelay_fb_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc < 1) {
		error("spectdelay~: feedback list must have at least 1 value");
		return;
	}
	if (argc > kMaxTableLen) {
		post("spectdelay~ warning: feedback list can have no more than "
				"%d values", kMaxTableLen);
		argc = kMaxTableLen;
	}
	x->fbtablen = argc;

	if (x->fbtablen != 1 && x->dttablen != 1 &&
	                                            x->fbtablen != x->dttablen) {
		error("spectdelay~: feedback list must have same number of values as "
				"delay time list (%d)", x->dttablen);
		return;
	}

	float *tab = (float *) &x->fbtable;
#ifdef CONFORM_INPUT
	for (int i = 0; i < argc; i++) {
		tab[i] = _fclamp(-1.0f, _atom_getfloat(i, argc, argv), 1.0f);
		//post("tab[%d] = %f", i, tab[i]);
	}
#else
	for (int i = 0; i < argc; i++)
		tab[i] = _atom_getfloat(i, argc, argv);
#endif

	x->spectdelay->set_feedtable(x->fbtable, x->fbtablen);
}


void spectdelay_fx_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc != 2) {
		error("spectdelay~: fx index value");
		return;
	}
	const int index = _atom_getlong(0, argc, argv);
	if (index >= x->fbtablen) {
		error("spectdelay~: fx index out of range");
		return;
	}
#ifdef CONFORM_INPUT
	float value = _fclamp(-1.0f, _atom_getfloat(1, argc, argv), 1.0f);
#else
	float value = _atom_getfloat(1, argc, argv);
#endif
	x->fbtable[index] = value;	// update array read by Spectacle object
}


void spectdelay_fft_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc != 3) {
		error("spectdelay~: fft fftlen windowlen overlap");
		return;
	}
	int fftlen = _atom_getlong(0, argc, argv);
	int windowlen = _atom_getlong(1, argc, argv);
	int overlap = _atom_getlong(2, argc, argv);

	// If any of these params is updated, must rebuild object, restoring all
	// non-default settings.
	if (fftlen != x->fftlen || windowlen != x->windowlen
			|| overlap != x->overlap) {
		Spectacle *newsp = new Spectacle();
		newsp->init(fftlen, windowlen, overlap, x->srate, x->maxdeltime);
		newsp->set_eqtable(x->eqtable, x->eqtablen);
		newsp->set_deltable(x->dttable, x->dttablen);
		newsp->set_feedtable(x->fbtable, x->fbtablen);
		newsp->set_freqrange(x->minfreq, x->maxfreq);
		newsp->set_delay_freqrange(x->dminfreq, x->dmaxfreq);
		newsp->set_binmap_table(x->eqbinmap, x->eqbinmaplen);
		newsp->set_delay_binmap_table(x->delaybinmap, x->delaybinmaplen);
		newsp->set_hold(x->hold);
		newsp->set_posteq(x->posteq);
		x->fftlen = fftlen;
		x->windowlen = windowlen;
		x->overlap = overlap;

		// The perform function is running in a different thread, and it
      // accesses the Spectacle object.  We tell perform to delete the old
		// object and swap in the new one, when it's ready.  If we were to
		// do it here, perform could try to use a deleted Spectacle.
      x->newspectdelay = newsp;
	}
}


void spectdelay_hold_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc != 1) {
		error("spectdelay~: hold 0 or 1");
		return;
	}
   x->hold = (bool) _atom_getlong(0, argc, argv);
   x->spectdelay->set_hold(x->hold);
}


void spectdelay_posteq_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc != 1) {
		error("spectdelay~: posteq 0 or 1");
		return;
	}
   x->posteq = (bool) _atom_getlong(0, argc, argv);
   x->spectdelay->set_posteq(x->posteq);
}


void spectdelay_maxdel_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc != 1) {
		error("spectdelay~: maxdel time (max delay time in seconds)");
		return;
	}
	float maxdeltime = _fclamp(kMinMaxDelTime, _atom_getfloat(0, argc, argv),
	                                                          kMaxMaxDelTime);
	if (maxdeltime != x->maxdeltime) {
		if (maxdeltime < x->maxdeltime) {
			// pin existing delay times to new maximum
			for (int i = 0; i < x->dttablen; i++) {
				if (x->dttable[i] > maxdeltime)
					x->dttable[i] = maxdeltime;
			}
		}
		x->spectdelay->set_maxdeltime(maxdeltime);
		x->maxdeltime = maxdeltime;
	}
}


void spectdelay_range_msg(t_spectdelay *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc != 2) {
		error("spectdelay~: range min max (EQ frequency range in Hz)");
		return;
	}
	const float nyquist = x->srate / 2;
	float max = _atom_getfloat(1, argc, argv);
	if (max == 0)
		max = nyquist;
	else
		max = _fclamp(0.0, max, nyquist);
	float min = _fclamp(0.0, _atom_getfloat(0, argc, argv), max);

	// changing range is expensive, so avoid when params are same
	if (min != x->minfreq || max != x->maxfreq) {
		x->minfreq = min;
		x->maxfreq = max;
		x->spectdelay->set_freqrange(min, max);
	}
}


