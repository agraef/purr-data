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



/* calculates the sum of the samples in each frame*/

#include "flib.h"
static t_class *pp_class;

typedef struct _pp {
  t_object  x_obj;
  t_float f,x;
} t_pp;

static t_int *pp_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_pp *x = (t_pp *)(w[2]);
  t_int N = (t_int)(w[3]);
  float sum = 0;
  
  while(N--)
	  sum += *in++;
  
  outlet_float(x->x_obj.ob_outlet, sum);
  return (w+4);
}

static void pp_dsp(t_pp *x, t_signal **sp)
{
  dsp_add(pp_perform, 3,
          sp[0]->s_vec, x, sp[0]->s_n);
}

static void *pp_new(void)
{
  t_pp *x = (t_pp *)pd_new(pp_class);
  outlet_new(&x->x_obj, &s_float);
  return (void *)x;
}


void pp_tilde_setup(void) {
  pp_class = class_new(gensym("pp~"),
        (t_newmethod)pp_new,
        0, sizeof(t_pp),
        CLASS_DEFAULT,0);

  class_addcreator((t_newmethod)pp_new, gensym("++~"), A_DEFFLOAT, 0);
  class_addmethod(pp_class,
        (t_method)pp_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(pp_class, t_pp,f);
  class_sethelpsymbol(pp_class, gensym("help-flib"));
}
