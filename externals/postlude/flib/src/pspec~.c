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



/* Compute the phase (in radians) spectrum of a signal given the real and imaginary components of an fft */

#include "flib.h"
#define SQ(a) (a * a)

static t_class *pspec_class;

typedef struct _pspec {
  t_object  x_obj;
  t_float f;
} t_pspec;

static t_int *pspec_perform(t_int *w)
{
  t_sample *in1 = (t_sample *)(w[1]);
  t_sample *in2 = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  t_int N = (t_int)(w[4]),n;
  t_float piv2 = M_PI * .5;
  for (n = 0; n < N; n++){
	  if(in1[n] == 0 && in2[n] == 0 || in1[n] == 0)
		  *out++ = 0;
	  else if(in2[n] == 0) 
		  *out++ = piv2;
	  else
	  *out++ = atanf(in1[n] / in2[n]);
  }
  return (w+5);
}

static void pspec_dsp(t_pspec *x, t_signal **sp)
{
  dsp_add(pspec_perform, 4,
          sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

static void *pspec_new(void)
{
  t_pspec *x = (t_pspec *)pd_new(pspec_class);
 
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}


void pspec_tilde_setup(void) {
  pspec_class = class_new(gensym("pspec~"),
        (t_newmethod)pspec_new,
        0, sizeof(t_pspec),
        CLASS_DEFAULT, 0);

  class_addmethod(pspec_class,
        (t_method)pspec_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(pspec_class, t_pspec,f);
  class_sethelpsymbol(pspec_class, gensym("help-flib"));
}
