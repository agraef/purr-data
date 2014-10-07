/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2011 */

#include "m_pd.h"
#include "iemlib.h"

/* --- sin_freq~ - output the frequency of a --- */
/* --- sinewave in Hz ----- */
/* --- as a signal ----------------------------- */

typedef struct _sin_freq_tilde
{
  t_object x_obj;
  t_sample x_prev;
  t_sample x_cur_out;
  t_sample x_counter;
  t_sample x_sr;
  t_float  x_float_sig_in;
} t_sin_freq_tilde;

static t_class *sin_freq_tilde_class;

static t_int *sin_freq_tilde_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_sin_freq_tilde *x = (t_sin_freq_tilde *)(w[3]);
  int i, n = (t_int)(w[4]);
  t_sample prev=x->x_prev;
  t_sample cur_out=x->x_cur_out;
  t_sample counter=x->x_counter;
  t_sample sr=x->x_sr;
  t_sample delta_x=0.0;
  
  for(i=0; i<n; i++)
  {
    if((in[i] >= 0.0) && (prev < 0.0)) /* begin of counting, pos. zero cross of sig_in */
    {
      delta_x = prev / (prev - in[i]);  /* dx = y1 / (y1 - y2) */
      counter += delta_x;
      cur_out = sr / counter;
      
      counter = 1.0 - delta_x;
    }
    else
    {
      counter += 1.0;
    }
    
    prev = in[i];
    out[i] = cur_out;
  }
  
  x->x_prev = prev;
  x->x_cur_out = cur_out;
  x->x_counter = counter;
  
  return(w+5);
}

static void sin_freq_tilde_dsp(t_sin_freq_tilde *x, t_signal **sp)
{
  x->x_sr = (t_sample)sp[0]->s_sr;
  dsp_add(sin_freq_tilde_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void *sin_freq_tilde_new(void)
{
  t_sin_freq_tilde *x = (t_sin_freq_tilde *)pd_new(sin_freq_tilde_class);
  
  outlet_new(&x->x_obj, &s_signal);
  
  x->x_prev = 0.0;
  x->x_cur_out = 0.0;
  x->x_counter = 0.0;
  x->x_sr = 44100.0;
  x->x_float_sig_in = 0.0;
  
  return (x);
}

void sin_freq_tilde_setup(void)
{
  sin_freq_tilde_class = class_new(gensym("sin_freq~"), (t_newmethod)sin_freq_tilde_new,
        0, sizeof(t_sin_freq_tilde), 0, 0);
  CLASS_MAINSIGNALIN(sin_freq_tilde_class, t_sin_freq_tilde, x_float_sig_in);
  class_addmethod(sin_freq_tilde_class, (t_method)sin_freq_tilde_dsp, gensym("dsp"), 0);
}

/*
geradengleichung:

y - y1 = ((y2 - y1) / (x2 - x1)) * (x - x1)
y = ((y2 - y1) / (x2 - x1)) * (x - x1) + y1 = 0
x1 = 0
x2 = 1
0 = ((y2 - y1) / 1) * (x) + y1
-y1 = (y2 - y1) * x
x = y1 / (y1 - y2)
*/
