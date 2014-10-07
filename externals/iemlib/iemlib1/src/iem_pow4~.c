/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2012 */

#include "m_pd.h"
#include "iemlib.h"
#include <math.h>

/* ------------------------ iem_pow4~ ----------------------------- */

static t_class *iem_pow4_tilde_class;

typedef struct _iem_pow4_tilde
{
  t_object  x_obj;
  t_sample  x_expo;
  t_float   x_float_sig_in;
} t_iem_pow4_tilde;

static void iem_pow4_tilde_ft1(t_iem_pow4_tilde *x, t_floatarg f)
{
  x->x_expo = (t_sample)f;
}

static t_int *iem_pow4_tilde_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_iem_pow4_tilde *x = (t_iem_pow4_tilde *)(w[3]);
  t_sample expo=x->x_expo;
  t_sample f, g;
  int n = (int)(w[4])/4;
  
  while (n--)
  {
    f = (t_sample)(*in);
    if(f > 0.0)
    {
      g = log(f);
      f = exp(g * expo);
      *out++ = f;
      *out++ = f;
      *out++ = f;
      *out++ = f;
      
      /*g = pow(f, expo);
      *out++ = g;
      *out++ = g;
      *out++ = g;
      *out++ = g;*/
    }
    else
    {
      *out++ = 0.0;
      *out++ = 0.0;
      *out++ = 0.0;
      *out++ = 0.0;
    }
    in += 4;
  }
  return (w+5);
}

static void iem_pow4_tilde_dsp(t_iem_pow4_tilde *x, t_signal **sp)
{
  dsp_add(iem_pow4_tilde_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void *iem_pow4_tilde_new(t_floatarg f)
{
  t_iem_pow4_tilde *x = (t_iem_pow4_tilde *)pd_new(iem_pow4_tilde_class);
  
  x->x_expo = (t_sample)f;
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
  outlet_new(&x->x_obj, gensym("signal"));
  x->x_float_sig_in = 0.0;
  return (x);
}

void iem_pow4_tilde_setup(void)
{
  iem_pow4_tilde_class = class_new(gensym("iem_pow4~"), (t_newmethod)iem_pow4_tilde_new, 0,
    sizeof(t_iem_pow4_tilde), 0, A_DEFFLOAT, 0);
  CLASS_MAINSIGNALIN(iem_pow4_tilde_class, t_iem_pow4_tilde, x_float_sig_in);
  class_addmethod(iem_pow4_tilde_class, (t_method)iem_pow4_tilde_dsp, gensym("dsp"), 0);
  class_addmethod(iem_pow4_tilde_class, (t_method)iem_pow4_tilde_ft1, gensym("ft1"), A_FLOAT, 0);
}
