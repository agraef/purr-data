/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2006 */

#include "m_pd.h"
#include "iemlib.h"

/* --- sin_phase~ - output the phase-difference between --- */
/* --- 2 sinewaves with the same frequency in samples ----- */
/* --- as a signal ---------------------------------------- */

typedef struct _sin_phase_tilde
{
  t_object x_obj;
  t_sample x_prev1;
  t_sample x_prev2;
  t_sample x_cur_out;
  t_sample x_counter1;
  t_sample x_counter2;
  int      x_state1;
  int      x_state2;
  t_float  x_float_sig_in;
} t_sin_phase_tilde;

static t_class *sin_phase_tilde_class;

static t_int *sin_phase_tilde_perform(t_int *w)
{
  t_sample *in1 = (t_sample *)(w[1]);
  t_sample *in2 = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  t_sin_phase_tilde *x = (t_sin_phase_tilde *)(w[4]);
  int i, n = (t_int)(w[5]);
  t_sample prev1=x->x_prev1;
  t_sample prev2=x->x_prev2;
  t_sample cur_out=x->x_cur_out;
  t_sample counter1=x->x_counter1;
  t_sample counter2=x->x_counter2;
  int state1=x->x_state1;
  int state2=x->x_state2;
  
  for(i=0; i<n; i++)
  {
    if((in1[i] >= 0.0) && (prev1 < 0.0))
    {/* pos. zero cross of sig_in_1 */
      state1 = 1;
      counter1 = prev1 / (prev1 - in1[i]);  /* x = y1 / (y1 - y2) */
    }
    else if((in1[i] < 0.0) && (prev1 >= 0.0))
    {/* neg. zero cross of sig_in_1 */
      state2 = 1;
      counter2 = prev1 / (prev1 - in1[i]);  /* x = y1 / (y1 - y2) */
    }
    
    if((in2[i] >= 0.0) && (prev2 < 0.0))
    {/* pos. zero cross of sig_in_2 */
      state1 = 0;
      cur_out = counter1 + prev2 / (prev2 - in2[i]) - 1.0;
      counter1 = 0.0;
    }
    else if((in2[i] < 0.0) && (prev2 >= 0.0))
    {/* neg. zero cross of sig_in_2 */
      state2 = 0;
      cur_out = counter2 + prev2 / (prev2 - in2[i]) - 1.0;
      counter2 = 0.0;
    }
    
    if(state1)
      counter1 += 1.0;
    if(state2)
      counter2 += 1.0;
    
    prev1 = in1[i];
    prev2 = in2[i];
    out[i] = cur_out;
  }
  
  x->x_prev1 = prev1;
  x->x_prev2 = prev2;
  x->x_cur_out = cur_out;
  x->x_counter1 = counter1;
  x->x_counter2 = counter2;
  x->x_state1 = state1;
  x->x_state2 = state2;
  
  return(w+6);
}

static void sin_phase_tilde_dsp(t_sin_phase_tilde *x, t_signal **sp)
{
  dsp_add(sin_phase_tilde_perform, 5, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, x, sp[0]->s_n);
}

static void *sin_phase_tilde_new(void)
{
  t_sin_phase_tilde *x = (t_sin_phase_tilde *)pd_new(sin_phase_tilde_class);
  
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, &s_signal);
  
  x->x_prev1 = 0.0;
  x->x_prev2 = 0.0;
  x->x_cur_out = 0.0;
  x->x_counter1 = 0.0;
  x->x_counter2 = 0.0;
  x->x_state1 = 0;
  x->x_state2 = 0;
  x->x_float_sig_in = 0.0;
  
  return (x);
}

void sin_phase_tilde_setup(void)
{
  sin_phase_tilde_class = class_new(gensym("sin_phase~"), (t_newmethod)sin_phase_tilde_new,
        0, sizeof(t_sin_phase_tilde), 0, 0);
  CLASS_MAINSIGNALIN(sin_phase_tilde_class, t_sin_phase_tilde, x_float_sig_in);
  class_addmethod(sin_phase_tilde_class, (t_method)sin_phase_tilde_dsp, gensym("dsp"), A_CANT, 0);
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
