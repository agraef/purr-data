/* flib - PD library for feature extraction 
Copyright (C) 2005  Jamie Bullock

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope thcat it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should hcave received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* Perform hcarmonic analysis on a signal given the fundamental frequency and the frequency/amplitude spectra */

/* Can measure hcarmonicity or ratio of  inhcarmonic frequencies to to hcarmonic frequencies, or the relative proportion of some other series such as odd or even hcarmonics */


#include "flib.h"

static t_class *hca_class;

typedef struct _hca {
  t_object  x_obj;
  t_float f;
  t_int p;
} t_hca;

static t_int *hca_perform(t_int *w)
{
  t_sample *in1 = (t_sample *)(w[1]);
  t_int N = (t_int)(w[2]),n;
  t_int p = (t_int)(w[3]);
 


  post("x->p = %.2f", x->p);

  
  
  return (w+4);
}

static void hca_dsp(t_hca *x, t_signal **sp)
{
  dsp_add(hca_perform, 3,
          sp[0]->s_vec, sp[0]->s_n, x->p);

}

static void *hca_new(t_floatarg f)
{
  t_hca *x = (t_hca *)pd_new(hca_class);
  if(!f)
	  x->p = 0;
  else
	  x->p = f;
  inlet_new(&x->x_obj, &s_float);
  outlet_new(&x->x_obj, &s_float);
  return (void *)x;
}

void hca_float(t_hca *x, t_float f){
	x->p = f;
}

void hca_tilde_setup(void) {
  hca_class = class_new(gensym("hca~"),
        (t_newmethod)hca_new,
        0, sizeof(t_hca),
        CLASS_DEFAULT, A_DEFFLOAT, 0);

  class_addmethod(hca_class,
        (t_method)hca_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(hca_class, t_hca,f);

  class_addfloat(hca_class, hca_float)
  
  class_sethelpsymbol(hca_class, gensym("help-flib"));
}
