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



/* calculates the spectral sc of one frame, given peak frequency and amplitude to first and second inputs respectively */

#include "flib.h"

static t_class *sc_class;

typedef struct _sc {
  t_object  x_obj;
  t_float f, x;
} t_sc;

static t_int *sc_perform(t_int *w)
{
  t_sample *in1 = (t_sample *)(w[1]);
  t_sample *in2 = (t_sample *)(w[2]);
  t_sc *x = (t_sc *)(w[3]);
  t_int N = (t_int)(w[4]),M = N >> 1,n;
  t_float FA = 0,A = 0,C;
  for (n = 0; n < M; n++){
	  if (in2[n] != 0){
		  FA += in1[n] * in2[n];
		  A += in2[n];
	  }
  }
  C = FA / A;
  outlet_float(x->x_obj.ob_outlet, C);
  return (w+5);
}

static void sc_dsp(t_sc *x, t_signal **sp)
{
  dsp_add(sc_perform, 4,
          sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void *sc_new(void)
{
  t_sc *x = (t_sc *)pd_new(sc_class);
 
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, &s_float);
  return (void *)x;
}


void sc_tilde_setup(void) {
  sc_class = class_new(gensym("sc~"),
        (t_newmethod)sc_new,
        0, sizeof(t_sc),
        CLASS_DEFAULT, 
        A_DEFFLOAT, 0);

  class_addmethod(sc_class,
        (t_method)sc_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(sc_class, t_sc,f);
  class_sethelpsymbol(sc_class, gensym("help-flib"));
}
