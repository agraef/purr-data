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

/* Takes fundamental through right inlet and % allowed deviation from actual harmonic to be considered harmonic as argument. Outputs number of partials, number of harmonics, number of even harmonics  */

#include "flib.h"

static t_class *hca_class;

typedef struct _hca {
  t_object  x_obj;
  t_float f;
  t_float fund, deviation;
  t_outlet *p_out, *h_out, *e_out;
} t_hca;

static t_int *hca_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_int N = (t_int)(w[2]),n;
  t_hca *x = (t_hca *)(w[3]);
  t_int npartials, nharmonics, neven, harmonic;
  t_float partial, harmonicity;
  
  neven = nharmonics = npartials = 0; 
  
  while(N--){
	  if(in[N]){
		npartials++;
		partial = in[N] / x->fund;
		harmonic = (t_int)rintf(partial);
		if(partial <= harmonic + x->deviation && 
				partial >= harmonic - x->deviation){
			nharmonics++;
			if(harmonic / 2.0f == rint(harmonic / 2.0f))
				neven++;
		}
	  }
  }

  if(!npartials)
	  post("hca~: No partials found.");
  
  
  outlet_float(x->p_out, npartials);	
  outlet_float(x->h_out, nharmonics);	
  outlet_float(x->e_out, neven);	
  
  return (w+4);
}

static void hca_dsp(t_hca *x, t_signal **sp)
{
  dsp_add(hca_perform, 3,
          sp[0]->s_vec, sp[0]->s_n, x);

}

static void *hca_new(t_floatarg f)
{
  t_hca *x = (t_hca *)pd_new(hca_class);
  if(!f)
	  x->deviation = 0;
  else
	  x->deviation = f * .01;
  
  post("deviation = %.2f", x->deviation);

  if(!x->fund)
	  x->fund = 0;
  
  floatinlet_new(&x->x_obj, &x->fund);
  x->p_out = outlet_new(&x->x_obj, &s_float);
  x->h_out = outlet_new(&x->x_obj, &s_float);
  x->e_out = outlet_new(&x->x_obj, &s_float);
  return (void *)x;
}

void hca_tilde_setup(void) {
  hca_class = class_new(gensym("hca~"),
        (t_newmethod)hca_new,
        0, sizeof(t_hca),
        CLASS_DEFAULT, A_DEFFLOAT, 0);

  class_addmethod(hca_class,
        (t_method)hca_dsp, gensym("dsp"), 0);
 
  CLASS_MAINSIGNALIN(hca_class, t_hca,f);
  
  class_sethelpsymbol(hca_class, gensym("help-flib"));
}
