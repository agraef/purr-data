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



/*Outputs amplitude (in dB referenecd to a maximum of 100dB) and frequency of spectral peaks from outlets 1 and 2. A peak is defined as a bin that has a greater magnitude than either of its neighbouring bins. A peak detection threshold is given by the first argument (or right inlet as a float) as a percentage of the frame maximum, i.e. a setting of 100 finds peaks equal to the size of the highest peak, a setting of 0 will output all peaks. Default is 0*/

#include "flib.h"
#define SQ(a) (a * a)

static t_class *peak_class;

typedef struct _peak {
  t_object  x_obj;
  t_float f, thresh, *buf;
  t_int sr,M;
} t_peak;

static t_float max(t_float *array, t_int size){
	register t_float t = array[0];
	register t_int i;
	for(i = 1; i < size; i++){
		if (t < array[i])
			t = array[i];
	}
	return t;
}
	
static t_int *peak_perform(t_int *w)
{
  t_sample *in1 = (t_sample *)(w[1]);
  t_sample *out1 = (t_float *)(w[2]);
  t_sample *out2 = (t_float *)(w[3]);
  t_int N = (t_int)(w[4]), M = N >> 1,n;
  t_float thresh = *(t_float *)(w[5]);
  t_peak *x = (t_peak *)(w[6]);
  t_float width = (t_float)x->sr / (t_float)N,y,y2,y3,p,t;
  
  x->M = M;
  x->buf = getbytes(M * sizeof(t_float));

  for(n = 0; n < M; n++){
	  if ((t = in1[n] * 100000) >= 1) /* ensure we never take log10(0) */
		  x->buf[n] = t;
	  else
		  x->buf[n] = 1;
  }

  if(thresh < 0 || thresh > 100)
	  thresh = 0;
  else
	  thresh *= .01 * max(&x->buf[0],M);
  
  out1[0] = 0;
  out2[0] = 0;
  
  for(n = 1; n < M; n++){
	  if(x->buf[n] >= thresh){
		  if(x->buf[n] > x->buf[n - 1] && x->buf[n] > x->buf[n + 1]){
			  out1[n] = width * (n + (p = .5 * (y = 20 * log10(x->buf[n-1]) - (y3 = 20 * log10(x->buf[n+1]))) / (20 * log10(x->buf[n - 1]) - 2 * (y2 = 20 * log10(x->buf[n])) + 20 * log10(x->buf[n + 1]))));
				  out2[n] = y2 - .25 * (y - y3) * p;
		  }
		  else{
			  out1[n] = 0;
			  out2[n] = 0;
		  }
	  }
	  else{
		  out1[n] = 0;
		  out2[n] = 0;
	  }
  }	  
  
  for (;n < N; n++){
	  out1[n] = 0;
	  out2[n] = 0;
  }
  
  return (w+7);

}

static void peak_dsp(t_peak *x, t_signal **sp)
{
  dsp_add(peak_perform, 6, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n,&x->thresh, x);
}

static void *peak_new(t_symbol *s, t_int argc, t_atom *argv)
{
  t_peak *x = (t_peak *)pd_new(peak_class);
  floatinlet_new(&x->x_obj, &x->thresh);
  outlet_new(&x->x_obj, &s_signal);
  outlet_new(&x->x_obj, &s_signal);
  x->sr = (t_int)sys_getsr();
  x->thresh = atom_getfloatarg(0,argc, argv);
	
  return (void *)x;
}

static void peak_free(t_peak *x){
	freebytes(x->buf, sizeof(t_float) * x->M);
}

void peak_tilde_setup(void) {
  peak_class = class_new(gensym("peak~"),
        (t_newmethod)peak_new,
        (t_method)peak_free, sizeof(t_peak),
        CLASS_DEFAULT, A_GIMME, 0);

	class_addmethod(peak_class, (t_method)peak_dsp, gensym("dsp"), 0);
    CLASS_MAINSIGNALIN(peak_class, t_peak,f);
	class_sethelpsymbol(peak_class, gensym("help-flib"));
	
}
