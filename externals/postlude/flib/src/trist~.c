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



/* calculates tristimulus of one frame according to method outlined by Pollard and Jansson. First argument determines method used (x,y, or z) */

#include "flib.h"
static t_class *trist_class;

typedef struct _trist {
  t_object  x_obj;
  t_float f,x;
  t_symbol *t;
} t_trist;

static t_int *trist_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_trist *x = (t_trist *)(w[2]);
  t_int N = (t_int)(w[3]),M = (N >> 1) - 1,n,i = 0;
  t_float T, A = 0,p1 = 0,p2 = 0,p3 = 0,p4 = 0,p5 = 0;

  for(n = 0; n < M; n++){
	  A += in[n];
	  if (in[n] != 0 && i < 5){
		  switch(i){
			  case 4: p5 = in[n]; i++; 
					  break;
			  case 3: p4 = in[n]; i++;
					  break;
			  case 2: p3 = in[n]; i++;
					  break;
			  case 1: p2 = in[n]; i++;
					  break;
			  case 0: p1 = in[n]; i++;
					  break;
		  }
	  }
  }
  if (*x->t->s_name == 'z')
	  T = p1 / A;
	  else if (*x->t->s_name == 'y')
		  T = (p2 + p3 + p4) / A;
		  else
			  T = (A - p1 + p2 + p3 + p4 + p5) / A;

  outlet_float(x->x_obj.ob_outlet, T);
  return (w+4);
}

static void trist_dsp(t_trist *x, t_signal **sp)
{
  dsp_add(trist_perform, 3,
          sp[0]->s_vec, x, sp[0]->s_n);
}

static void *trist_new(t_symbol *s)
{
  t_trist *x = (t_trist *)pd_new(trist_class);
  x->t = s;
  outlet_new(&x->x_obj, &s_float);
  return (void *)x;
}


void trist_tilde_setup(void) {
  trist_class = class_new(gensym("trist~"),
        (t_newmethod)trist_new,
        0, sizeof(t_trist),
        CLASS_DEFAULT, A_DEFSYMBOL, 0);

  class_addmethod(trist_class,
        (t_method)trist_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(trist_class, t_trist,f);
  class_sethelpsymbol(trist_class, gensym("help-flib"));
}
