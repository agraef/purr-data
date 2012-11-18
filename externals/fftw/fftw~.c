/* Copyright (c) 1997-1999 Miller Puckette and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "fftw~.h"

#ifdef HAVE_LIBFFTW3F

/* Support for fftw3 by Tim Blechmann                                       */

/* ------------------------ fft~ -------------------------------- */
static t_class *sigfftw_class;

typedef struct fftw
{
  t_object x_obj;
  float x_f;

  fftwf_plan plan;
  fftwf_iodim dim;
} t_sigfftw;

static void *sigfftw_new(void)
{
  t_sigfftw *x = (t_sigfftw *)pd_new(sigfftw_class);
  outlet_new(&x->x_obj, gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  x->x_f = 0;
  return (x);
}

static void sigfftw_free(t_sigfftw * x)
{
  fftwf_destroy_plan(x->plan);
}

static t_int *sigfftw_perform(t_int *w)
{
  fftwf_execute(*(fftwf_plan *)w[1]);

  return (w+2);
}

static void sigfftw_dsp(t_sigfftw *x, t_signal **sp)
{
  int n = sp[0]->s_n;
  float *in1 = sp[0]->s_vec;
  float *in2 = sp[1]->s_vec;
  float *out1 = sp[2]->s_vec;
  float *out2 = sp[3]->s_vec;
    
  x->dim.n=n;
  x->dim.is=1;
  x->dim.os=1;
  x->plan = fftwf_plan_guru_split_dft(1, &(x->dim), 0, 
                                      NULL, 
                                      in1, in2, out1, out2, 
                                      FFTW_ESTIMATE);
  dsp_add(sigfftw_perform, 1, &x->plan);
}

EXTERNAL_SETUP fftw_tilde_setup(void)
{
  sigfftw_class = class_new(gensym("fftw~"), sigfftw_new, 
                            (t_method) sigfftw_free,
                            sizeof(t_sigfftw), 0, 0);
  CLASS_MAINSIGNALIN(sigfftw_class, t_sigfftw, x_f);
  class_addmethod(sigfftw_class, (t_method)sigfftw_dsp, 
                  gensym("dsp"), 0);
}
#endif
