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



/* calculates the spectral smoothness of one frame according to McAdams 1999. Takes magnitude spectrum as input*/

#include "flib.h"
static t_class *ss_class;

typedef struct _ss {
  t_object  x_obj;
  t_float f,x;
} t_ss;

static t_int *ss_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_ss *x = (t_ss *)(w[2]);
  t_int N = (t_int)(w[3]),M = N >> 1,n;
  t_float I = 0, buf[M];
  for(n = 0; n < M; buf[n++] = *in++);
  for(n = 2; n < M - 1; n++){ 
	  if(buf[n] != 0 && buf[n-1] != 0 && buf[n+1] != 0)
		  I += (20 * log10(buf[n]) - (20 * log10(buf[n-1]) + 20 * log10(buf[n]) + 20 * log10(buf[n+1])) / 3);
  }

  outlet_float(x->x_obj.ob_outlet, I);
  return (w+4);
}

static void ss_dsp(t_ss *x, t_signal **sp)
{
  dsp_add(ss_perform, 3,
          sp[0]->s_vec, x, sp[0]->s_n);
}

static void *ss_new(void)
{
  t_ss *x = (t_ss *)pd_new(ss_class);
  outlet_new(&x->x_obj, &s_float);
  return (void *)x;
}


void ss_tilde_setup(void) {
  ss_class = class_new(gensym("ss~"),
        (t_newmethod)ss_new,
        0, sizeof(t_ss),
        CLASS_DEFAULT,0);

  class_addmethod(ss_class,
        (t_method)ss_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(ss_class, t_ss,f);
  class_sethelpsymbol(ss_class, gensym("help-flib"));
}
