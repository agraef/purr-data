/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2011 */

#include "m_pd.h"
#include "iemlib.h"
#include <math.h>

/* -- lp1_t~ - slow dynamic lowpass-filter 1. order controlled by time constant tau input --- */
/* -- now with double precision; for low-frequency filters it is important to calculate the filter in double precision -- */

typedef struct _lp1_t_tilde
{
  t_object  x_obj;
  double    yn1;
  double    c0;
  double    c1;
  double    sr;
  double    cur_t;
  double    delta_t;
  double    end_t;
  t_float   ticks_per_interpol_time;
  t_float   rcp_ticks;
  t_float   interpol_time;
  int       ticks;
  int       counter_t;
  t_float   x_float_sig_in;
} t_lp1_t_tilde;

static t_class *lp1_t_tilde_class;

static void lp1_t_tilde_dsp_tick(t_lp1_t_tilde *x)
{
  if(x->counter_t)
  {
    if(x->counter_t <= 1)
    {
      x->cur_t = x->end_t;
      x->counter_t = 0;
    }
    else
    {
      x->counter_t--;
      x->cur_t += x->delta_t;
    }
    if(x->cur_t == 0.0)
      x->c1 = 0.0;
    else
      x->c1 = exp((x->sr)/x->cur_t);
    x->c0 = 1.0 - x->c1;
  }
}

static t_int *lp1_t_tilde_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_lp1_t_tilde *x = (t_lp1_t_tilde *)(w[3]);
  int i, n = (t_int)(w[4]);
  double yn0, yn1=x->yn1;
  double c0=x->c0, c1=x->c1;
  
  lp1_t_tilde_dsp_tick(x);
  for(i=0; i<n; i++)
  {
    yn0 = (double)(*in++)*c0 + yn1*c1;
    *out++ = (t_sample)yn0;
    yn1 = yn0;
  }
  /* NAN protect */
  //if(IEM_DENORMAL(yn1))
  //  yn1 = 0.0;
  x->yn1 = yn1;
  return(w+5);
}

static t_int *lp1_t_tilde_perf8(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_lp1_t_tilde *x = (t_lp1_t_tilde *)(w[3]);
  int i, n = (t_int)(w[4]);
  double ynn[9];
  double c0=x->c0, c1=x->c1;
  
  lp1_t_tilde_dsp_tick(x);
  ynn[0] = x->yn1;
  for(i=0; i<n; i+=8, in+=8, out+=8)
  {
    ynn[1] = (double)in[0]*c0 + ynn[0]*c1;
    out[0] = (t_sample)ynn[1];
    ynn[2] = (double)in[1]*c0 + ynn[1]*c1;
    out[1] = (t_sample)ynn[2];
    ynn[3] = (double)in[2]*c0 + ynn[2]*c1;
    out[2] = (t_sample)ynn[3];
    ynn[4] = (double)in[3]*c0 + ynn[3]*c1;
    out[3] = (t_sample)ynn[4];
    ynn[5] = (double)in[4]*c0 + ynn[4]*c1;
    out[4] = (t_sample)ynn[5];
    ynn[6] = (double)in[5]*c0 + ynn[5]*c1;
    out[5] = (t_sample)ynn[6];
    ynn[7] = (double)in[6]*c0 + ynn[6]*c1;
    out[6] = (t_sample)ynn[7];
    ynn[8] = (double)in[7]*c0 + ynn[7]*c1;
    out[7] = (t_sample)ynn[8];
    ynn[0] = ynn[8];
  }
  /* NAN protect */
  //if(IEM_DENORMAL(ynn[0]))
  //  ynn[0] = 0.0f;
  
  x->yn1 = ynn[0];
  return(w+5);
}

static void lp1_t_tilde_ft2(t_lp1_t_tilde *x, t_floatarg interpol)
{
  int i = (int)((x->ticks_per_interpol_time)*interpol);
  
  x->interpol_time = interpol;
  if(i <= 0)
    i = 1;
  x->ticks = i;
  x->rcp_ticks = 1.0 / (t_float)i;
}

static void lp1_t_tilde_ft1(t_lp1_t_tilde *x, t_floatarg f_time_const)
{
  double d_time_const;
  
  if(f_time_const < 0.0)
    f_time_const = 0.0;
  d_time_const = (double)f_time_const;
  if(d_time_const != x->cur_t)
  {
    x->end_t = d_time_const;
    x->counter_t = x->ticks;
    x->delta_t = (d_time_const - x->cur_t) * (double)x->rcp_ticks;
  }
}

static void lp1_t_tilde_set(t_lp1_t_tilde *x, t_floatarg w1)
{
  x->yn1 = (double)w1;
}

static void lp1_t_tilde_dsp(t_lp1_t_tilde *x, t_signal **sp)
{
  int i, n=(int)sp[0]->s_n;
  
  x->sr = -1000.0 / (double)(sp[0]->s_sr);
  x->ticks_per_interpol_time = 0.001 * (t_float)(sp[0]->s_sr) / (t_float)n;
  i = (int)((x->ticks_per_interpol_time)*(x->interpol_time));
  if(i <= 0)
    i = 1;
  x->ticks = i;
  x->rcp_ticks = 1.0 / (t_float)i;
  if(x->cur_t == 0.0)
    x->c1 = 0.0;
  else
    x->c1 = exp((x->sr)/x->cur_t);
  x->c0 = 1.0 - x->c1;
  if(n&7)
    dsp_add(lp1_t_tilde_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, n);
  else
    dsp_add(lp1_t_tilde_perf8, 4, sp[0]->s_vec, sp[1]->s_vec, x, n);
}

static void *lp1_t_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_lp1_t_tilde *x = (t_lp1_t_tilde *)pd_new(lp1_t_tilde_class);
  int i;
  t_float interpol=0.0;
  double time_const=0.0;
  
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft2"));
  outlet_new(&x->x_obj, &s_signal);
  x->x_float_sig_in = 0.0;
  x->counter_t = 1;
  x->delta_t = 0.0;
  x->interpol_time = 0.0;
  x->yn1 = 0.0;
  x->sr = -1.0 / 44.1;
  if((argc >= 1)&&IS_A_FLOAT(argv,0))
    time_const = (double)atom_getfloatarg(0, argc, argv);
  if((argc >= 2)&&IS_A_FLOAT(argv,1))
    interpol = (t_float)atom_getfloatarg(1, argc, argv);
  if(time_const < 0.0)
    time_const = 0.0;
  x->cur_t = time_const;
  if(time_const == 0.0)
    x->c1 = 0.0;
  else
    x->c1 = exp((x->sr)/time_const);
  x->c0 = 1.0 - x->c1;
  if(interpol < 0.0)
    interpol = 0.0;
  x->interpol_time = interpol;
  x->ticks_per_interpol_time = 0.5;
  i = (int)((x->ticks_per_interpol_time)*(x->interpol_time));
  if(i <= 0)
    i = 1;
  x->ticks = i;
  x->rcp_ticks = 1.0 / (t_float)i;
  x->end_t = x->cur_t;
  return (x);
}

void lp1_t_tilde_setup(void)
{
  lp1_t_tilde_class = class_new(gensym("lp1_t~"), (t_newmethod)lp1_t_tilde_new,
        0, sizeof(t_lp1_t_tilde), 0, A_GIMME, 0);
  CLASS_MAINSIGNALIN(lp1_t_tilde_class, t_lp1_t_tilde, x_float_sig_in);
  class_addmethod(lp1_t_tilde_class, (t_method)lp1_t_tilde_dsp, gensym("dsp"), 0);
  class_addmethod(lp1_t_tilde_class, (t_method)lp1_t_tilde_ft1, gensym("ft1"), A_FLOAT, 0);
  class_addmethod(lp1_t_tilde_class, (t_method)lp1_t_tilde_ft2, gensym("ft2"), A_FLOAT, 0);
  class_addmethod(lp1_t_tilde_class, (t_method)lp1_t_tilde_set, gensym("set"), A_FLOAT, 0);
}
