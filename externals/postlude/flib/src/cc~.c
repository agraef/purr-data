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


/*Calculate the cc correlation of two signal vectors*/

/*The time domain implementation is based on code by Phil Bourke 
 * the frequency domain version is based on code by Charles Henry 
 * 
 * Specify a time delay as an argument for the time domain implemenation, for example an argument of 32 will give the 
 * correlation coefficients  for delays from -32 to 32 samples between the two input vectors
 *
 * Specify an argument of 'f' for the frequency domain implementation,   
 * or 'r' for the running cross covariance (not normalized)              instead of the numerical delay argument
 * these two methods both have got positive delays on 0,N/2-1 and the negative delays (-N/2, -1) are indexed on N/2,N-1 */


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <m_pd.h>
#define SQ(a) (a * a)

static t_class *cc_class;

typedef struct _cc {
    t_object  x_obj;
    t_float f;
    t_int delay;
    t_int is_freq_domain;
    t_float *bufferNsig1, *bufferNsig2;
    t_float *output_prev_block;
    t_int is_new_or_rszd;
    t_int n;
} t_cc;

static t_int *cc_perform_time_domain(t_int *w)
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
	post("cc~: invalid maxdelay, must be <= blocksize/2");
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


static t_int *cc_perform_freq_domain(t_int *w)
{
  t_cc *x = (t_cc *)(w[1]);

  t_sample *sig1 = (t_sample *)(w[2]);
  t_sample *sig2 = (t_sample *)(w[3]);
  t_sample *out  = (t_sample *)(w[4]);
  t_int size  = (int) w[5];
  t_int size2 = size*2;
  t_int half = size/2;
  t_int thrhalf = 3*half;
  t_float *expsig1 = NULL;
  t_float *revsig2 = NULL;
  t_float temp, temp2;
  t_int i=0;

  if (x->n!=size)
  {
    x->n = size;
    x->is_new_or_rszd=1;
  }
  
// This stuff here sets up two buffers to hold the previous N samples
// To get the usual overlapping block (2) design on each input

  if (x->is_new_or_rszd)
  {
    if (x->bufferNsig1!=NULL)
    {
      freebytes(x->bufferNsig1,size*sizeof(t_float));
      freebytes(x->bufferNsig2,size*sizeof(t_float));
      freebytes(x->output_prev_block,size*sizeof(t_float));
    }
    x->bufferNsig1=getbytes(size*sizeof(t_float));
    x->bufferNsig2=getbytes(size*sizeof(t_float));
    x->output_prev_block=getbytes(size*sizeof(t_float));
    for(i=0; i<size; i++)
    {
      x->bufferNsig1[i]=0;
      x->bufferNsig2[i]=0;
      x->output_prev_block[i]=0;
    }
    x->is_new_or_rszd=0;
  }

// The two signals are created, using a block size of 2N, --size2
// expsig1 is the expanded signal1 x->bufferNsig1 + sig1
// revsig2 is the reversed signal2 (reversed about i=0)
// it is made 0.0 on 0 to half and thrhalf to size2


  expsig1=(t_float *) getbytes(size2*sizeof(t_float));
  revsig2=(t_float *) getbytes(size2*sizeof(t_float));

// Loops for assignment of old values in new block + buffer

  for (i=0; i < half ; i++)
  {
    expsig1[i]=x->bufferNsig1[i];
    revsig2[i]=0.0;
  }
  for (i=half; i < size ; i++)
  {
    expsig1[i]=x->bufferNsig1[i];
    revsig2[i]=sig2[size-i];
  }
  expsig1[size]=sig1[0];
  revsig2[size]=sig2[0];
  for (i=size+1; i < thrhalf ; i++)
  {
    expsig1[i]=sig1[i-size];
    revsig2[i]=x->bufferNsig2[size2-i];
  }
  for (i=thrhalf; i < size2 ; i++)
  {
    expsig1[i]=sig1[i-size];
    revsig2[i]=0.0;
  }
//  Here we set the buffers for the next round
  for(i=0; i < size; i++)
  {
    x->bufferNsig1[i]=(t_float) sig1[i];
    x->bufferNsig2[i]=(t_float) sig2[i];
  }
//  fft the two blocks and multiply them
  mayer_realfft(size2, expsig1);
  mayer_realfft(size2, revsig2);
  
  expsig1[0]*=revsig2[0];
  expsig1[size]*=revsig2[size];
  
  
  for(i=1; i < size; i++)  
  {
    temp=expsig1[i];
    temp2=expsig1[size2-i];
    expsig1[i]=temp*revsig2[i]-temp2*revsig2[size2-i];
    expsig1[size2-i]=-1.0*(temp*revsig2[size2-i]+temp2*revsig2[i]);
  }
//  ifft
  
  mayer_realifft(size2, expsig1);
//  format the output:  this section formats the ouptut either as
//  a simple cc or as a running cc
  if (x->is_freq_domain == 1)
  {
    for(i=0; i < half; i++)
    {
      out[i]=expsig1[thrhalf+i]/size2;
      out[half + i]=expsig1[i]/size2;
    }
  }
  else
  {
    for(i=0; i < half; i++)
    {
      out[i]=x->output_prev_block[i] + expsig1[thrhalf+i]/size2;
      out[half + i]=x->output_prev_block[half + i] + expsig1[i]/size2;
      x->output_prev_block[i] = out[i];
      x->output_prev_block[half + i] = out[half + i];
    }
  }

  freebytes(expsig1, size2*sizeof(t_float));
  freebytes(revsig2, size2*sizeof(t_float));
  return(w+6);
}

static void cc_dsp(t_cc *x, t_signal **sp)
{
    if(!x->is_freq_domain)
	dsp_add(cc_perform_time_domain, 5,
		sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n, x->delay);
    else
	dsp_add(cc_perform_freq_domain, 5, x,
		sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}


//  For using with running calculation, send a bang to clear the buffer
//  and start over with calculations

static void cc_bang(t_cc *x)
{
  int i;
  for(i=0;i<x->n;i++)
    x->output_prev_block[i]=0;
}


static void *cc_new(t_symbol *s, t_int argc, t_atom *argv)
{
    t_cc *x = (t_cc *)pd_new(cc_class);
    if(atom_getsymbol(argv) == gensym("f")){
	    x->is_freq_domain = 1;
	    post("flib: cc: Frequency domain selected");
    }
    else if(atom_getsymbol(argv) == gensym("r")){
	    x->is_freq_domain = 2;
	    post("flib: cc: Running frequency domain selected");
    }
    else {
	x->delay = atom_getfloat(argv);
	post("flib: cc: Time domain selected");
    } 
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    x->is_new_or_rszd=1;
    x->bufferNsig1=NULL;
    x->bufferNsig2=NULL;
    x->output_prev_block=NULL;
    return (void *)x;
}

static void cc_free(t_cc *x)
{
  if     (x->bufferNsig1 != NULL)
    freebytes(x->bufferNsig1, x->n*sizeof(t_float));
  if     (x->bufferNsig2 != NULL)
    freebytes(x->bufferNsig2, x->n*sizeof(t_float));
  if     (x->output_prev_block != NULL)
    freebytes(x->output_prev_block, x->n*sizeof(t_float));
}


void cc_tilde_setup(void) {
    cc_class = class_new(gensym("cc~"),
	    (t_newmethod)cc_new,
	    (t_method)cc_free, sizeof(t_cc),
	    CLASS_DEFAULT, A_GIMME, 0);

    class_addbang(cc_class, (t_method)cc_bang);
    class_addmethod(cc_class,
	    (t_method)cc_dsp, gensym("dsp"), 0);
    CLASS_MAINSIGNALIN(cc_class, t_cc,f);
    class_sethelpsymbol(cc_class, gensym("help-flib"));
}


