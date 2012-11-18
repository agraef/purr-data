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



/* calculates the maximum sample value in each frame*/

#include "flib.h"
static t_class *bmax_class;

typedef struct _bmax {
  t_object  x_obj;
  t_float f,x;
  t_int I; /*instances of blockmax to find */
  t_outlet *M_out, *I_out;
} t_bmax;


static t_float max(t_float *array, t_int size){
	register t_float t = array[0];
	register t_int i;
	for(i = 1; i < size; i++){
		if (t < array[i])
			t = array[i];
	}
	return t;
}

static t_int *bmax_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_bmax *x = (t_bmax *)(w[2]);
  t_int N = (t_int)(w[3]);
  t_float blockmax;
  t_int maxloc[x->I], i, j;
  t_atom argv[x->I];
  
 /* argv = getbytes(x->I * sizeof(t_atom)); */
  
  blockmax = max(in,N);

  for(i = 0, j = 0; i < N; i++){
	  if (in[i] == blockmax && j < x->I)
		  maxloc[j++] = i;
  }

  if (j < x->I)
	  for(; j < x->I; maxloc[j++] = 0); /* zero out the rest of the array */
  
  for(i = 0; i < x->I; i++)
	SETFLOAT(argv+i, maxloc[i]);
  
  outlet_float(x->M_out, blockmax);
  outlet_list(x->I_out, &s_list, x->I, argv);

  return (w+5);
}

static void bmax_dsp(t_bmax *x, t_signal **sp)
{
  dsp_add(bmax_perform, 4,
          sp[0]->s_vec, x, sp[0]->s_n);
}

static void *bmax_new(t_floatarg f)
{
  t_bmax *x = (t_bmax *)pd_new(bmax_class);
  if (!f)
	  x->I = 1;
  else
	  x->I = (t_int)floor(f);
  x->M_out = outlet_new(&x->x_obj, &s_float);
  x->I_out = outlet_new(&x->x_obj, &s_list);
  return (void *)x;
}


void bmax_tilde_setup(void) {
  bmax_class = class_new(gensym("bmax~"),
        (t_newmethod)bmax_new,
        0, sizeof(t_bmax),
        CLASS_DEFAULT, A_DEFFLOAT, 0);

  class_addmethod(bmax_class,
        (t_method)bmax_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(bmax_class, t_bmax,f);
  class_sethelpsymbol(bmax_class, gensym("help-flib"));
}
