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



/* cleans 'nan' 'inf' '-inf' from an audio vector or control value 
 * If no argument is given 'dirty' values will be replaced by 0, otherwise values are replaced by the argument value. If an audio vector is given as input, a bang is sent to the right output if 'dirty' values are found */

#include "flib.h"

#define CLEAN (x->outval == x->inf ? 0 : x->outval)

static t_class *clean_class;

typedef struct _clean {
  t_object  x_obj;
  t_float f;
  t_outlet *out_r;
  t_float outval, inf;
} t_clean;

static t_int *clean_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_int N = (t_int)(w[3]);
  t_clean *x = (t_clean *)(w[4]);
  t_int dirty = 0;
	 
  while(N--){
 	if(!finite(*in)){
		dirty = 1;
		*out++ = CLEAN;
			
	}
	else
		*out++ = *in++;
  }
  if(dirty)
	  outlet_bang(x->out_r);

  return (w+5);
}

static void clean_dsp(t_clean *x, t_signal **sp)
{
  dsp_add(clean_perform, 4,
          sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n, x);
}

static void clean_float(t_clean *x, t_float f)  
{
	  x->f = f;
	 if(!finite(x->f))
		 outlet_float(x->out_r, CLEAN);
	 else
		 outlet_float(x->out_r, x->f);

}



static void *clean_new(t_symbol *s, t_int argc, t_atom *argv)
{
  t_clean *x = (t_clean *)pd_new(clean_class);
  outlet_new(&x->x_obj, &s_signal);
  x->out_r = outlet_new(&x->x_obj, &s_float);
  x->inf =  (t_float)HUGE_VAL;
  
  if(argc)
  	x->outval = atom_getfloatarg(0, argc, argv);
  else
	x->outval = x->inf;

  post("%.2f", x->outval);
  return (void *)x;
}


void clean_tilde_setup(void) {
  clean_class = class_new(gensym("clean~"),
        (t_newmethod)clean_new,
        0, sizeof(t_clean),
        CLASS_DEFAULT, A_GIMME, 0);

  CLASS_MAINSIGNALIN(clean_class, t_clean,f);
  class_addfloat(clean_class, clean_float);
  class_addmethod(clean_class,
        (t_method)clean_dsp, gensym("dsp"), 0);
  class_sethelpsymbol(clean_class, gensym("help-flib"));
}
