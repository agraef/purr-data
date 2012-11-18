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



/* calculates spectral flatness measure: Geometric Mean/ Arithemtic Mean (In this case converted to a DB scale */

#include "m_pd.h"
#include <math.h>

static t_class *sfm_class;

typedef struct _sfm {
  t_object  x_obj;
  t_float f,x;
} t_sfm;

static t_int *sfm_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sfm *x = (t_sfm *)(w[2]);
  t_int N = (t_int)(w[3]),M = (N >> 1) - 1,n;
  t_float F, A = 0, G = 1;
  t_float iM = 1 / (float)M;
  for(n = 0; n < M; n++){
	  if(in[n] !=0){
		  G *= in[n];
	      A += in[n];
	  }
  }
  F = 10 * log10(pow(G, iM) / (iM * A));
  outlet_float(x->x_obj.ob_outlet, F);
  return (w+4);
}

static void sfm_dsp(t_sfm *x, t_signal **sp)
{
  dsp_add(sfm_perform, 3,
          sp[0]->s_vec, x, sp[0]->s_n);
}

static void *sfm_new(void)
{
  t_sfm *x = (t_sfm *)pd_new(sfm_class);
  outlet_new(&x->x_obj, &s_float);
  return (void *)x;
}


void sfm_tilde_setup(void) {
  sfm_class = class_new(gensym("sfm~"),
        (t_newmethod)sfm_new,
        0, sizeof(t_sfm),
        CLASS_DEFAULT,0);

  class_addmethod(sfm_class,
        (t_method)sfm_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(sfm_class, t_sfm,f);
  class_sethelpsymbol(sfm_class, gensym("help-flib"));
}
