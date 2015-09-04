/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2012 */

#include "m_pd.h"
#include "iemlib.h"
#include <math.h>

#define IEMSQRT4TAB1SIZE 256
#define IEMSQRT4TAB2SIZE 1024

/* ------------------------ iem_sqrt4~ ----------------------------- */

static t_float iem_rsqrt4_tilde_exptab[IEMSQRT4TAB1SIZE];
static t_float iem_rsqrt4_tilde_mantissatab[IEMSQRT4TAB2SIZE];

static t_class *iem_sqrt4_tilde_class;

typedef struct _iem_sqrt4_tilde
{
  t_object  x_obj;
  t_float   x_float_sig_in;
} t_iem_sqrt4_tilde;

static t_int *iem_sqrt4_tilde_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_int n = (t_int)(w[3])/4;
  
  while(n--)
  {
    t_sample f = *in;
    t_sample g, h;
    union tabfudge_f tf;
    
    if(f <= 0.0)
    {
      *out++ = 0.0;
      *out++ = 0.0;
      *out++ = 0.0;
      *out++ = 0.0;
    }
    else
    {
      tf.tf_f = f;
      g = iem_rsqrt4_tilde_exptab[((tf.tf_l) >> 23) & 0xff] * iem_rsqrt4_tilde_mantissatab[((tf.tf_l) >> 13) & 0x3ff];
      h = f * (1.5f * g - 0.5f * g * g * g * f);
      *out++ = h;
      *out++ = h;
      *out++ = h;
      *out++ = h;
    }
    in += 4;
  }
  return(w+4);
}

static void iem_sqrt4_tilde_dsp(t_iem_sqrt4_tilde *x, t_signal **sp)
{
  dsp_add(iem_sqrt4_tilde_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void iem_sqrt4_tilde_init(void)
{
  int i;
  t_float f;
  union tabfudge_f tf;
  
  for(i=0; i<IEMSQRT4TAB1SIZE; i++)
  {
    tf.tf_l = (i ? (i == IEMSQRT4TAB1SIZE-1 ? IEMSQRT4TAB1SIZE-2 : i) : 1)<< 23;
    iem_rsqrt4_tilde_exptab[i] = 1.0/sqrt(tf.tf_f); 
  }
  for(i=0; i<IEMSQRT4TAB2SIZE; i++)
  {
    f = 1.0 + (1.0/(t_float)IEMSQRT4TAB2SIZE) * (t_float)i;
    iem_rsqrt4_tilde_mantissatab[i] = 1.0/sqrt(f);  
  }
}

static void *iem_sqrt4_tilde_new(void)
{
  t_iem_sqrt4_tilde *x = (t_iem_sqrt4_tilde *)pd_new(iem_sqrt4_tilde_class);
  
  outlet_new(&x->x_obj, gensym("signal"));
  x->x_float_sig_in = 0;
  return (x);
}

void iem_sqrt4_tilde_setup(void)
{
  iem_sqrt4_tilde_init();
  iem_sqrt4_tilde_class = class_new(gensym("iem_sqrt4~"), (t_newmethod)iem_sqrt4_tilde_new, 0,
    sizeof(t_iem_sqrt4_tilde), 0, 0);
  CLASS_MAINSIGNALIN(iem_sqrt4_tilde_class, t_iem_sqrt4_tilde, x_float_sig_in);
  class_addmethod(iem_sqrt4_tilde_class, (t_method)iem_sqrt4_tilde_dsp, gensym("dsp"), A_CANT, 0);
}
