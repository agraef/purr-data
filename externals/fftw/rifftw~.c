/* Copyright (c) 1997-1999 Miller Puckette and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "fftw~.h"

#ifdef HAVE_LIBFFTW3F

/* Support for fftw3 by Tim Blechmann                                       */

/* ----------------------- rifft~ -------------------------------- */

static t_class *sigrifftw_class;

typedef struct rifftw
{
  t_object x_obj;   
  float x_f;

  fftwf_plan plan;  
  fftwf_iodim dim;
} t_sigrifftw;

static void *sigrifftw_new(void)
{
  t_sigrifftw *x = (t_sigrifftw *)pd_new(sigrifftw_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, gensym("signal"));
  x->x_f = 0;
  return (x);
}

static void sigrifftw_free(t_sigrifftw *x)
{
  fftwf_destroy_plan(x->plan);
}

static t_int *sigrifftw_perform(t_int *w)
{
  fftw_siginvert((t_sample *)w[2],w[3]); 
  fftwf_execute(*(fftwf_plan*)w[1]);
  return (w+4);
}

static void sigrifftw_dsp(t_sigrifftw *x, t_signal **sp)
{
  int n = sp[0]->s_n, n2 = (n>>1);
  float *in1 = sp[0]->s_vec;
  float *in2 = sp[1]->s_vec;
  float *out = sp[2]->s_vec;

  if (n < 4)
    {
      error("fft: minimum 4 points");
      return;
    }

  else    
    {
      x->dim.n=n;
      x->dim.is=1;
      x->dim.os=1;
      x->plan = fftwf_plan_guru_split_dft_c2r(1, &(x->dim), 0, 
                                              NULL, 
                                              in1, in2, out, 
                                              FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
      dsp_add_zero(in1+ n/2, n/2);
      dsp_add(sigrifftw_perform,3,&x->plan,in2,n2);
    }
}

EXTERNAL_SETUP rifftw_tilde_setup(void)
{
  sigrifftw_class = class_new(gensym("rifftw~"), sigrifftw_new, 
                              (t_method)sigrifftw_free,
                              sizeof(t_sigrifftw), 0, 0);
  CLASS_MAINSIGNALIN(sigrifftw_class, t_sigrifftw, x_f);
  class_addmethod(sigrifftw_class, (t_method)sigrifftw_dsp, 
                  gensym("dsp"), 0);
}
#endif
