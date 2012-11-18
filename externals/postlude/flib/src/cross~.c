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


/*Calculate the cross correlation of two signal vectors*/

/*The time domain implementation is based on code by Phil Bourke 
 * the frequency domain version is based on code by Charles Henry 
 * 
 * Specify a time delay as an argument for the time domain implemenation, for example an argument of 32 will give the correlation coefficients  for delays from -32 to 32 samples between the two input vectors
 *
 * Specify an argument of 'f' for the frequency domain implementation*/


#include "flib.h"
#define SQ(a) (a * a)

static t_class *cross_class;

typedef struct _cross {
    t_object  x_obj;
    t_float f;
    t_int delay;
    t_int is_freq_domain;
} t_cross;

static t_int *cross_perform_time_domain(t_int *w)
{
    t_sample *x = (t_sample *)(w[1]);
    t_sample *y = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    t_int N = (t_int)(w[4]),
	  i, j, delay;
    t_int maxdelay = (t_int)(w[5]);
    t_float mx, my, sx, sy, sxy, denom, r;
  
   if(maxdelay > N * .5){
	maxdelay = N * .5;
	post("cross~: invalid maxdelay, must be <= blocksize/2");
   }
 
/* Calculate the mean of the two series x[], y[] */
   mx = 0;
   my = 0;   
   for (i=0;i<N;i++) {
      mx += x[i];
      my += y[i];
   }
   mx /= N;
   my /= N;

   /* Calculate the denominator */
   sx = 0;
   sy = 0;
   for (i=0;i<N;i++) {
      sx += (x[i] - mx) * (x[i] - mx);
      sy += (y[i] - my) * (y[i] - my);
   }
   denom = sqrt(sx*sy);

   /* Calculate the correlation series */
   for (delay=-maxdelay;delay<maxdelay;delay++) {
      sxy = 0;
      for (i=0;i<N;i++) {
         j = i + delay;

	/* circular correlation */
	 while (j < 0)
            j += N;
         j %= N;
         sxy += (x[i] - mx) * (y[j] - my);

     }
      r = sxy / denom;
	*out++ = r;
      /* r is the correlation coefficient at "delay" */

   }

  return (w+6);

}


t_int *cross_perform_freq_domain(t_int *w)
{
  t_cross *x = (t_cross *)(w[1]);

  t_sample *sig1 = (t_sample *)(w[2]);
  t_sample *sig2 = (t_sample *)(w[3]);
  t_sample *out  = (t_sample *)(w[4]);
  long int size  = (long int) w[5];
  long int k     = size/2;
  float *expsig1 = NULL;
  float *revsig2 = NULL;
  float temp, temp2;
  long int i=0;
  int well_defined=1;
  int qtr, thrqtr;

// The two signals are created, nonzero on 0 to N/4 and 3N/4 to N
// This will be revised

  expsig1=(float *) alloca(size*sizeof(float));
  revsig2=(float *) alloca(size*sizeof(float));
  qtr = size/4;
  thrqtr = 3*size/4;
  for (i=0; i < qtr ; i++)
  {
    expsig1[i]=sig1[i];
    revsig2[i]=0;
  }
  for (i=qtr; i < thrqtr ; i++)
  {
    expsig1[i]=sig1[i];
    revsig2[i]=sig2[size-i];
  }
  for (i=thrqtr; i < size ; i++)
  {
    expsig1[i]=sig1[i];
    revsig2[i]=0;
  }

  mayer_realfft(size, expsig1);
  mayer_realfft(size, revsig2);
  expsig1[0]*=revsig2[0];
  expsig1[k]*=revsig2[k];
  for(i=1; i < k; i++)
  {
    temp=expsig1[i];
    temp2=expsig1[size-i];
    expsig1[i]=temp*revsig2[i]-temp2*revsig2[size-i];
    expsig1[size-i]=temp*revsig2[size-i]+temp2*revsig2[i];
  }
  
  mayer_realifft(size, expsig1);
  for(i=0; i < size; i++)
  {
    out[i]=expsig1[i];
  }

  return(w+6);

}

static void cross_dsp(t_cross *x, t_signal **sp)
{
    if(!x->is_freq_domain)
	dsp_add(cross_perform_time_domain, 5,
		sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n, x->delay);
    else
	dsp_add(cross_perform_freq_domain, 5, x,
		sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

static void *cross_new(t_symbol *s, t_int argc, t_atom *argv)
{
    t_cross *x = (t_cross *)pd_new(cross_class);

    if(atom_getsymbol(argv) == gensym("f")){
	    x->is_freq_domain = 1;
	    post("flib: cross: Frequency domain selected");
    }
    else {
	x->delay = atom_getfloat(argv);
	post("flib: cross: Time domain selected");
    } 
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    return (void *)x;
}


void cross_tilde_setup(void) {
    cross_class = class_new(gensym("cross~"),
	    (t_newmethod)cross_new,
	    0, sizeof(t_cross),
	    CLASS_DEFAULT, A_GIMME, 0);

    class_addmethod(cross_class,
	    (t_method)cross_dsp, gensym("dsp"), 0);
    CLASS_MAINSIGNALIN(cross_class, t_cross,f);
    class_sethelpsymbol(cross_class, gensym("help-flib"));
}
