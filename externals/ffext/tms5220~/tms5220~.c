/* (C) 2010 Federico Ferri <mescalinum@gmail.com>
 * this software is gpl'ed software, read the file "README.txt" for details
 */

#include "tms5220/tms5220.c"

#define USE_LIBSAMPLERATE
//#define DEBUG_TMS5220_TILDE

// rate at which tms5220 emulator produces samples (according to datasheet)
#define TMS5220_SAMPLE_RATE 8000

#ifdef USE_LIBSAMPLERATE
#define SRC_TYPE SRC_LINEAR //fastest
//#define SRC_TYPE SRC_SINC_BEST_QUALITY
#include <samplerate.h>
#endif

#include "m_pd.h"

static t_class *tms5220_tilde_class;

typedef struct _tms5220_tilde {
	t_object x_obj;

	// sample rate
	t_float sr;
	t_float resample_factor;

#ifdef USE_LIBSAMPLERATE
	SRC_STATE* src;
#else
#error Missing alternative implementation of sample rate converter. Please use libsamplerate.
#endif

	// status outlets
	t_int status;
	t_outlet *x_status;
	t_int ready;
	t_outlet *x_ready;
	t_int interrupt;
	t_outlet *x_interrupt;

	t_float dummy;
} t_tms5220_tilde;



void tms5220_tilde_reset(t_tms5220_tilde *x) {
	tms5220_reset();
}

void *tms5220_tilde_new(t_symbol *s, int argc, t_atom *argv) {
	t_tms5220_tilde *x = (t_tms5220_tilde *)pd_new(tms5220_tilde_class);

	x->status = x->ready = x->interrupt = 0;

	outlet_new(&x->x_obj, &s_signal);
	x->x_status = outlet_new(&x->x_obj, &s_float);
	x->x_ready = outlet_new(&x->x_obj, &s_float);
	x->x_interrupt = outlet_new(&x->x_obj, &s_float);

	tms5220_tilde_reset(x);

#ifdef USE_LIBSAMPLERATE
	if(x->src) src_delete(x->src);

	int err = -1;
	x->src = src_new(SRC_TYPE, 1, &err);

	if(x->src == NULL) {
		error("Failed libsamplerate initialization.");
		post("libsamplerate returned error code %d (%s).", err, src_strerror(err));
	} else {
#ifdef DEBUG_TMS5220_TILDE
		post("tms5220~: DEBUG: Initialized sample rate converter '%s'", src_get_name(SRC_TYPE));
#endif
	}
#endif

	return (void *)x;
}

void tms5220_tilde_free(t_tms5220_tilde *x) {
#ifdef USE_LIBSAMPLERATE
	// deallocate sample rate converter object
	if(x->src) {
		src_delete(x->src);
#ifdef DEBUG_TMS5220_TILDE
		post("tms5220~: DEBUG: Destroyed sample rate converter '%s'", src_get_name(SRC_TYPE));
#endif
	}
#endif
}

void tms5220_tilde_write(t_tms5220_tilde *x, t_floatarg data) {
	tms5220_data_write((int)data);
}

#define CHECK_ARGS(argcvar, num, errmsg) if(argcvar != num) { error(errmsg); return; }
#define CLIP_VAL(v, min, max) (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))

void tms5220_tilde_framefunc(t_tms5220_tilde *x, t_symbol *s, int argc, t_atom *argv) {
	int energy, pitch, k[10], i;

	energy = argc > 0 ? argv[0].a_w.w_float : -1;
	pitch = argc > 1 ? argv[1].a_w.w_float : -1;
	for(i = 2; i < 12; i++) k[i - 2] = argc > i ? argv[i].a_w.w_float : -1;

	energy = CLIP_VAL(energy, 1, 14);
	pitch = CLIP_VAL(pitch, 0, 63);
	k[0] = CLIP_VAL(k[0], 0, 31);
	k[1] = CLIP_VAL(k[1], 0, 31);
	k[2] = CLIP_VAL(k[2], 0, 15);
	k[3] = CLIP_VAL(k[3], 0, 15);
	k[4] = CLIP_VAL(k[4], 0, 15);
	k[5] = CLIP_VAL(k[5], 0, 15);
	k[6] = CLIP_VAL(k[6], 0, 15);
	k[7] = CLIP_VAL(k[7], 0, 7);
	k[8] = CLIP_VAL(k[8], 0, 7);
	k[9] = CLIP_VAL(k[9], 0, 7);

	unsigned char data[8];
	memset(&data[0], 0, 8);

	data[0] = 0x60; //speak external
 
	if(energy == 0 || energy == 15) { error("illegal value for energy"); return; }

	if(s == gensym("silent")) {
		CHECK_ARGS(argc, 0, "usage: silent");
		data[1] = 0;
	} else if(s == gensym("stop")) {
		CHECK_ARGS(argc, 0, "usage: stop");
		data[1] = 0xf0;
	} else if(s == gensym("repeat")) {
		CHECK_ARGS(argc, 2, "usage: repeat <energy> <pitch>");
		data[1] = ((energy << 4) | (1 << 3) | ((pitch & 56) >> 3));
		data[2] = (((pitch & 7) << 5));
	} else if(s == gensym("unvoiced")) {
		CHECK_ARGS(argc, 6, "usage: repeat <energy> <pitch> <k1> <k2> <k3> <k4>");
		data[1] = ((energy << 4) | 0 | 0);
		data[2] = (k[0]);
		data[3] = ((k[1] << 3) | ((k[2] & 14) >> 1));
		data[4] = (((k[2] & 1) << 7) | (k[3] << 3));
	} else if(s == gensym("voiced")) {
		CHECK_ARGS(argc, 12, "usage: repeat <energy> <pitch> <k1> <k2> <k3> <k4> <k5> <k6> <k7> <k8> <k9> <k10>");
		data[1] = ((energy << 4) | 0 | 0);
		data[2] = (k[0]);
		data[3] = ((k[1] << 3) | ((k[2] & 14) >> 1));
		data[4] = (((k[2] & 1) << 7) | (k[3] << 3) | ((k[4] & 14) >> 1));
		data[5] = (((k[4] & 1) << 7) | (k[5] << 3) | ((k[6] & 14) >> 1));
		data[6] = (((k[6] & 1) << 7) | (k[7] << 4) | (k[8] << 1) | ((k[9] & 4) >> 2));
		data[7] = (((k[9] & 3) << 6));
	} else {
		error("unknown method called: %s", s->s_name);
		return;
	}
	//tms5220_reset();
	for(i = 0; i < 8; i++) tms5220_data_write(data[i]);
	//tms5220_data_write(0);
}

void tms5220_tilde_update_status(t_tms5220_tilde *x) {
	t_int new_status, new_ready, new_interrupt;

	new_status = tms5220_status_read();
	new_ready = tms5220_ready_read();
	new_interrupt = tms5220_int_read();

	if(new_interrupt != x->interrupt) {
		outlet_float(x->x_interrupt, new_interrupt);
		x->interrupt = new_interrupt;
	}

	if(new_ready != x->ready) {
		outlet_float(x->x_ready, new_ready);
		x->ready = new_ready;
	}

	if(new_status != x->status) {
		outlet_float(x->x_status, new_status);
		x->status = new_status;
	}
}

t_int *tms5220_tilde_perform(t_int *w) {
	t_tms5220_tilde *x = (t_tms5220_tilde *)(w[1]);
	t_sample       *in =        (t_sample *)(w[2]);
	t_sample      *out =        (t_sample *)(w[3]);
	int              n =               (int)(w[4]);

	int m = (int)(0.0 + n / x->resample_factor);

	unsigned char *bytebuf = (unsigned char *)malloc(sizeof(unsigned char) * m);
	if(!bytebuf) {error("FATAL: cannot allocate signal buffer (byte)"); return w;}

	float *floatbuf = (float *)malloc(sizeof(float) * m);
	if(!floatbuf) {error("FATAL: cannot allocate signal buffer (float)"); return w;}

	tms5220_process(bytebuf, m);
	unsigned char *pb = bytebuf;
	float *fb = floatbuf;
	int m1 = m;
	while (m1--) *fb++ = (0.5 + ((t_sample) *pb++)) / 127.5;
	free(bytebuf);

#ifdef USE_LIBSAMPLERATE
	if(x->src != NULL) {
		SRC_DATA src_data;
		src_data.data_in = floatbuf;
		src_data.input_frames = m;
		src_data.data_out = out;
		src_data.output_frames = n;
		src_data.src_ratio = x->resample_factor;
		src_data.end_of_input = 0;
		int err;
		if(0 != (err = src_process(x->src, &src_data))) {
			error("error during resample (libsamplerate)");
			post("error code: %d (%s)", err, src_strerror(err));
		}
#ifdef DEBUG_TMS5220_TILDE
		// check residual:
		if(src_data.input_frames_used < m) {
			error("used only %d input frames, instead of %d", src_data.input_frames_used, m);
		}
		if(src_data.output_frames_gen < n) {
			error("generated only %d output frames, instead of %d", src_data.output_frames_gen, n);
		}
#endif
	} else {
		error("libsamplerate not initialized.");
	}
#else
	// missing implementation of poor man's sample rate converter
	// use libsamplerate for now
#endif

	free(floatbuf);

	tms5220_tilde_update_status(x);
	
	return (w + 5);
}

void tms5220_tilde_dsp(t_tms5220_tilde *x, t_signal **sp) {
	// update sample rate & resample factor:
	x->resample_factor = sp[0]->s_sr / TMS5220_SAMPLE_RATE;
#ifdef DEBUG_TMS5220_TILDE
	if(sp[0]->s_sr != x->sr)
		post("tms5220~: DEBUG: samplerate has changed to %f; new resample factor is %f.", sp[0]->s_sr, (float)x->resample_factor);
#endif
	x->sr = sp[0]->s_sr;

	dsp_add(tms5220_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void tms5220_tilde_setup(void) {
	tms5220_tilde_class = class_new(gensym("tms5220~"),
		(t_newmethod)tms5220_tilde_new,
		(t_method)tms5220_tilde_free,
		sizeof(t_tms5220_tilde),
		CLASS_DEFAULT, A_GIMME, 0);

	CLASS_MAINSIGNALIN(tms5220_tilde_class, t_tms5220_tilde, dummy);

	class_addmethod(tms5220_tilde_class, (t_method)tms5220_tilde_dsp, gensym("dsp"), 0);
	//class_addfloat(tms5220_tilde_class, (t_method)tms5220_tilde_write);
	class_addmethod(tms5220_tilde_class, (t_method)tms5220_tilde_write, gensym("write"), A_DEFFLOAT, 0);
	class_addmethod(tms5220_tilde_class, (t_method)tms5220_tilde_reset, gensym("reset"), 0);
	// frame building commands:
	class_addmethod(tms5220_tilde_class, (t_method)tms5220_tilde_framefunc, gensym("silent"), A_GIMME, 0);
	class_addmethod(tms5220_tilde_class, (t_method)tms5220_tilde_framefunc, gensym("stop"), A_GIMME, 0);
	class_addmethod(tms5220_tilde_class, (t_method)tms5220_tilde_framefunc, gensym("repeat"), A_GIMME, 0);
	class_addmethod(tms5220_tilde_class, (t_method)tms5220_tilde_framefunc, gensym("unvoiced"), A_GIMME, 0);
	class_addmethod(tms5220_tilde_class, (t_method)tms5220_tilde_framefunc, gensym("voiced"), A_GIMME, 0);

	post("tms5220~: TSM5220 IC emulation");
	post("tms5220~:   external by Federico Ferri <mescalinum@gmail.com>");
	post("tms5220~:   based on code by Frank Palazzolo <palazzol@tir.com>");
}
