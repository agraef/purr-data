/* Copyright (c) 1997-1999 Miller Puckette and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "fftw~.h"

#ifdef HAVE_LIBFFTW3F

/* Support for fftw3 by Tim Blechmann                                       */

/* ----------------------- rfft~ --------------------------------- */

static t_class *sigrfftw_class;

typedef struct rfftw
{
  t_object x_obj;
  float x_f;
    
  fftwf_plan plan;  
  fftwf_iodim dim;
} t_sigrfftw;

static void *sigrfftw_new(void)
{
  t_sigrfftw *x = (t_sigrfftw *)pd_new(sigrfftw_class);
  outlet_new(&x->x_obj, gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
  x->x_f = 0;
  return (x);
}

static void sigrfftw_free(t_sigrfftw *x)
{
  fftwf_destroy_plan(x->plan);
}

static t_int *sigrfftw_perform(t_int *w)
{
  fftwf_execute(*(fftwf_plan*)w[1]);
  fftw_siginvert((t_sample*)w[2],(t_int)w[3]);

  return (w+4);
}

static void sigrfftw_dsp(t_sigrfftw *x, t_signal **sp)
{
  int n = sp[0]->s_n, n2 = (n>>1);
  float *in = sp[0]->s_vec;
  float *out1 = sp[1]->s_vec;
  float *out2 = sp[2]->s_vec;

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
      x->plan = fftwf_plan_guru_split_dft_r2c(1, &(x->dim), 0, 
                                              NULL,
                                              in, out1, out2,
                                              FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
      dsp_add(sigrfftw_perform,3,&x->plan,out2+1,n2-1);
    }

  dsp_add_zero(out1 + n2, n2);
  dsp_add_zero(out2 + n2, n2);
}

EXTERNAL_SETUP rfftw_tilde_setup(void)
{
  sigrfftw_class = class_new(gensym("rfftw~"), sigrfftw_new, 
                             (t_method)sigrfftw_free,
                             sizeof(t_sigrfftw), 0, 0);
  CLASS_MAINSIGNALIN(sigrfftw_class, t_sigrfftw, x_f);
  class_addmethod(sigrfftw_class, (t_method)sigrfftw_dsp, 
                  gensym("dsp"), 0);
}
#endif
