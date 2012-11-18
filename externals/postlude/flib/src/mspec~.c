/* flib - PD library for feature extraction 
Copyright (C) 2005  Jamie Bullock

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/



/* Compute the magnitude spectrum of a signal given the real and imaginary components of an fft. The output maximum is normalised to 1. If an argument of 2 is given, the power spectrum is output */

#include "flib.h"
#define SQ(a) (a * a)

static t_class *mspec_class;

typedef struct _mspec {
  t_object  x_obj;
  t_float f;
  t_int p;
} t_mspec;

static t_int *mspec_perform(t_int *w)
{
  t_sample *in1 = (t_sample *)(w[1]);
  t_sample *in2 = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  t_int N = (t_int)(w[4]),n;
  t_int p = (t_int)(w[5]);
  if(p == 2){
	  for (n = 0; n < N; n++){
		  if(in1[n] != 0 && in2[n] != 0)
			  *out++ = (SQ(in1[n]) + SQ(in2[n])) / N;
		  else
			  *out++ = 0;
	  }
  }
  else{
	  for (n = 0; n < N; n++){
			  if(in1[n] != 0 && in2[n] != 0)
				  *out++ = q8_sqrt(SQ(in1[n]) + SQ(in2[n])) / N;
			  else
				  *out++ = 0;
	  }
  }
  return (w+6);
}

static void mspec_dsp(t_mspec *x, t_signal **sp)
{
  dsp_add(mspec_perform, 5,
          sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n, x->p);
}

static void *mspec_new(t_floatarg f)
{
  t_mspec *x = (t_mspec *)pd_new(mspec_class);
  if(f == 2)
	  x->p = 2;
  else
	  x->p = 0;
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}


void mspec_tilde_setup(void) {
  mspec_class = class_new(gensym("mspec~"),
        (t_newmethod)mspec_new,
        0, sizeof(t_mspec),
        CLASS_DEFAULT, A_DEFFLOAT, 0);

  class_addmethod(mspec_class,
        (t_method)mspec_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(mspec_class, t_mspec,f);
  class_sethelpsymbol(mspec_class, gensym("help-flib"));
}
