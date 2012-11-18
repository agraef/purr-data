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



/* calculates the spectral irreg of one frame according to Jensen  1999. Original formula by Krimphoff et al 1994*/

#include "flib.h"
static t_class *irreg_class;

typedef struct _irreg {
  t_object  x_obj;
  t_float f,x;
} t_irreg;

static t_int *irreg_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_irreg *x = (t_irreg *)(w[2]);
  t_int N = (t_int)(w[3]),M = N >> 1,n;
  t_float I = 0, X = 0,Y = 0;
  for(n = 1; n < M; n++){
	  X += in[n] - in[n+1];
	  Y += in[n] * in[n];
  }
  I = X / Y;
  outlet_float(x->x_obj.ob_outlet, I);
  return (w+4);
}

static void irreg_dsp(t_irreg *x, t_signal **sp)
{
  dsp_add(irreg_perform, 3,
          sp[0]->s_vec, x, sp[0]->s_n);
}

static void *irreg_new(void)
{
  t_irreg *x = (t_irreg *)pd_new(irreg_class);
  outlet_new(&x->x_obj, &s_float);
  return (void *)x;
}


void irreg_tilde_setup(void) {
  irreg_class = class_new(gensym("irreg~"),
        (t_newmethod)irreg_new,
        0, sizeof(t_irreg),
        CLASS_DEFAULT,0);

  class_addmethod(irreg_class,
        (t_method)irreg_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(irreg_class, t_irreg,f);
  class_sethelpsymbol(irreg_class, gensym("help-flib"));
}
