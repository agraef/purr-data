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



/* calculates a series of freq_bands Mel filters for multiplication by an fft power spectrum */ 

#include "flib.h"
static t_class *melf_class;

typedef struct _melf {
  t_object  x_obj;
  t_float f;
  t_int sr;
  t_float nyquist;
  t_int freq_bands;
  t_int n_filters;
  t_int fft_size;
  char style;
  t_float freq_min;
  t_float freq_max;
  t_float freq_min_mel;
  t_float freq_max_mel;
  t_float freq_bw_mel;
  t_float norm;
  t_float *mel_peak;
  t_float *lin_peak;
  t_int *fft_peak;
  t_float *height_norm;
  t_float **fft_tables; /* tables to hold the filters */
  t_outlet *graph_outlet; /*outlet for list of coeffs */
} t_melf;

static t_int *melf_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_melf *x = (t_melf *)(w[3]);
  t_int n_filter  = x->n_filters; 
  t_int N = (t_int)(w[2]), n;
  t_float out = 0;
  t_atom outs[x->n_filters];

  while(n_filter--){
	  t_float *filter = x->fft_tables[n_filter];
	  n = N;
	  while(n--){
		  out += in[N - n - 1] * *filter++;
	  }
	  SETFLOAT(&outs[n_filter], out);
  } 
	  outlet_list(x->graph_outlet, &s_list, x->n_filters, outs);

  return (w+4);
}

static void melf_dsp(t_melf *x, t_signal **sp)
{
  int n;
  dsp_add(melf_perform, 3,
          sp[0]->s_vec, sp[0]->s_n, x);
}

static void *melf_new(t_symbol *s, t_int argc, t_atom *argv)
{
	t_int n,i; 
 	t_float norm_fact, height, inc, val;
    t_melf *x = (t_melf *)pd_new(melf_class);
	x->n_filters = 0;
 
	x->sr = (t_int)sys_getsr();
	x->nyquist = x->sr * .5;
	
	for(i = 0; i < argc; i++, argv++){
		if(argv->a_type == A_FLOAT)
			switch(i){
				case 5: x->norm = argv->a_w.w_float;
						break;
				case 4: x->fft_size = argv->a_w.w_float;
						break;
				case 3: post("arg 4 should always be 'a' or 'g'");
						break;
				case 2: x->freq_max = argv->a_w.w_float;
						break;
				case 1: x->freq_min = argv->a_w.w_float;
						break;
				case 0: x->freq_bands = argv->a_w.w_float;
						break;
			}
		else if(argv->a_type == A_SYMBOL){
			x->style = (char)argv->a_w.w_symbol->s_name[0];
		}
	}
	
	if (!x->freq_bands) 
	 x->freq_bands = 60;

	if(!x->freq_min || x->freq_min < 0)
		x->freq_min = 0;
	
	if(!x->freq_max || x->freq_max > x->nyquist)
		x->freq_max_mel = x->nyquist;
	else
		x->freq_max_mel = x->freq_max;

	if(x->style != 'g' && x->style != 'a')
		x->style = 'g';
	
	if(!x->fft_size || (x->fft_size - 1) & x->fft_size)
		x->fft_size = 512;
	post("fft size = %d", x->fft_size);
	
	if(!x->norm || x->norm < 0 || x->norm > 20)
		x->norm = 1;

	x->freq_min_mel = 1127 * log10(1 + x->freq_min / 700);
	x->freq_bw_mel = (x->freq_max_mel - x->freq_min_mel) / x->freq_bands;

	x->mel_peak = getbytes((x->freq_bands + 2) * sizeof(t_float)); /* +2 for zeros at start and end */
	x->lin_peak = getbytes((x->freq_bands + 2) * sizeof(t_float));
	x->fft_peak = getbytes((x->freq_bands + 2) * sizeof(t_int));
	x->height_norm = getbytes(x->freq_bands * sizeof(t_float));
	
		x->mel_peak[0] = x->freq_min_mel;
		x->lin_peak[0] = (700 * (exp(x->mel_peak[0]/1127) - 1));
		x->fft_peak[0] = (t_int)rint(x->lin_peak[0]/x->nyquist * (x->fft_size / 2));
	
	for (n = 1; n <= x->freq_bands; n++) 	/*roll out peak locations - mel, linear and linear on fft window scale */
		{
		x->mel_peak[n] = x->mel_peak[n - 1] + x->freq_bw_mel;
		x->lin_peak[n] = (700 * (exp(x->mel_peak[n]/1127) - 1));
		x->fft_peak[n] = (t_int)rint(x->lin_peak[n]/x->nyquist *  (x->fft_size / 2));
	}
	
	for (n = 0; n <= x->freq_bands && x->lin_peak[n + 1] < x->freq_max_mel; n++) 	/*roll out normalised gain of each peak*/
		{
		if (x->style == 'g'){
			height = 1;	
			norm_fact = x->norm;
		}
		else{
			height = 2 / (x->lin_peak[n + 2] - x->lin_peak[n]);
			norm_fact = x->norm / (2 / (x->lin_peak[2] - x->lin_peak[0]));
			}
		x->height_norm[n] = height * norm_fact;
		x->n_filters = n;
	}

	post("Number of linear mel-spaced filters = %d", x->n_filters);
	
	x->fft_tables = (float **)getbytes(x->n_filters * sizeof(float *));
	for(n = 0; n < x->n_filters; n++)
		x->fft_tables[n] = (float *)getbytes(x->fft_size * sizeof(float));
	
	i = 0;
	
	for(n = 0; n < x->n_filters; n++){
		if(n > 0)/*calculate the rise increment*/
				inc = x->height_norm[n] / (x->fft_peak[n] - x->fft_peak[n - 1]);
			else
				inc = x->height_norm[n] / x->fft_peak[n];
		val = 0;	
		for(; i <= x->fft_peak[n]; i++){ /*fill in the 'rise' */
			x->fft_tables[n][i] = val;
			val += inc;
		}
		inc = x->height_norm[n] / (x->fft_peak[n + 1] - x->fft_peak[n]);/*calculate the fall increment */
		val = 0;
		for(i = x->fft_peak[n + 1]; i > x->fft_peak[n]; i--){ /*reverse fill the 'fall' */
			x->fft_tables[n][i] = val;
			val += inc;
		}
	}
	  x->graph_outlet = outlet_new(&x->x_obj, &s_list);
	

  return (void *)x;
}

static void melf_free(t_melf *x){
	int N;
	N = x->n_filters;
	while(N--)
		freebytes(x->fft_tables[N], sizeof(t_float) * x->fft_size);
	freebytes(x->fft_tables, sizeof(t_float *) * x->n_filters);
	freebytes(x->mel_peak,(x->freq_bands + 2) * sizeof(t_float)); 
	freebytes(x->lin_peak, (x->freq_bands + 2) * sizeof(t_float));
	freebytes(x->fft_peak, (x->freq_bands + 2) * sizeof(t_int));
	freebytes(x->height_norm, x->freq_bands * sizeof(t_float));
}



void melf_tilde_setup(void) {
  melf_class = class_new(gensym("melf~"),
        (t_newmethod)melf_new,
        (t_method)melf_free, sizeof(t_melf),
        CLASS_DEFAULT, A_GIMME, 0);

  class_addmethod(melf_class,
        (t_method)melf_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(melf_class, t_melf,f);
  class_sethelpsymbol(melf_class, gensym("help-mfcc"));
}
