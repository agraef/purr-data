/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2010 */


#include "m_pd.h"
#include "iemlib.h"
#include <math.h>

/* ---------------- peakenv_hold~ - simple peak-envelope-converter with peak hold time and release time. ----------------- */
/* -- now with double precision; for low-frequency filters it is important to calculate the filter in double precision -- */

typedef struct _peakenv_hold_tilde
{
  t_object x_obj;
  double   x_sr;
  double   x_old_peak;
  double   x_c1;
  double   x_releasetime;
  double   x_holdtime;
  t_int    x_n_hold;
  t_int    x_counter;
  t_float  x_float_sig_in;
} t_peakenv_hold_tilde;

static t_class *peakenv_hold_tilde_class;

static void peakenv_hold_tilde_reset(t_peakenv_hold_tilde *x)
{
  x->x_old_peak = 0.0;
}

static void peakenv_hold_tilde_ft1(t_peakenv_hold_tilde *x, t_float t_hold)/* hold-time in ms */
{
  double dhold;
  
  if(t_hold < 0.0)
    t_hold = 0.0;
  x->x_holdtime = (double)t_hold;
  dhold = x->x_sr*0.001*x->x_holdtime;
  if(dhold > 2147483647.0)
    dhold = 2147483647.0;
  x->x_n_hold = (t_int)(dhold + 0.5);
}

static void peakenv_hold_tilde_ft2(t_peakenv_hold_tilde *x, t_float t_rel)/* release-time in ms */
{
  if(t_rel < 0.0)
    t_rel = 0.0;
  x->x_releasetime = (double)t_rel;
  x->x_c1 = exp(-1.0/(x->x_sr*0.001*x->x_releasetime));
}

static t_int *peakenv_hold_tilde_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_peakenv_hold_tilde *x = (t_peakenv_hold_tilde *)(w[3]);
  int n = (int)(w[4]);
  double peak = x->x_old_peak;
  double c1 = x->x_c1;
  double absolute;
  t_int i, counter;
  
  counter = x->x_counter;
  for(i=0; i<n; i++)
  {
    absolute = (double)fabs(*in++);
    if(counter > 0)
      counter--;// hold peride
    else
      peak *= c1;// release periode
    if(absolute > peak)
    {
      peak = absolute;
      counter = x->x_n_hold;// new hold initialisation
    }
    *out++ = (t_sample)peak;
  }
  /* NAN protect */
  //if(IEM_DENORMAL(peak))
  //  peak = 0.0f;
  x->x_old_peak = peak;
  x->x_counter = counter;
  return(w+5);
}

static void peakenv_hold_tilde_dsp(t_peakenv_hold_tilde *x, t_signal **sp)
{
  x->x_sr = (double)sp[0]->s_sr;
  peakenv_hold_tilde_ft1(x, x->x_holdtime);
  peakenv_hold_tilde_ft2(x, x->x_releasetime);
  dsp_add(peakenv_hold_tilde_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, (t_int)sp[0]->s_n);
}

static void *peakenv_hold_tilde_new(t_float t_hold, t_float t_rel)
{
  t_peakenv_hold_tilde *x = (t_peakenv_hold_tilde *)pd_new(peakenv_hold_tilde_class);
  
  x->x_sr = 44100.0;
  peakenv_hold_tilde_ft1(x, t_hold);
  peakenv_hold_tilde_ft2(x, t_rel);
  x->x_old_peak = 0.0;
  x->x_counter = 0;
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft2"));
  outlet_new(&x->x_obj, &s_signal);
  x->x_float_sig_in = 0.0;
  return(x);
}

void peakenv_hold_tilde_setup(void)
{
  peakenv_hold_tilde_class = class_new(gensym("peakenv_hold~"), (t_newmethod)peakenv_hold_tilde_new,
    0, sizeof(t_peakenv_hold_tilde), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
  CLASS_MAINSIGNALIN(peakenv_hold_tilde_class, t_peakenv_hold_tilde, x_float_sig_in);
  class_addmethod(peakenv_hold_tilde_class, (t_method)peakenv_hold_tilde_dsp, gensym("dsp"), A_CANT, 0);
  class_addmethod(peakenv_hold_tilde_class, (t_method)peakenv_hold_tilde_ft1, gensym("ft1"), A_FLOAT, 0);
   class_addmethod(peakenv_hold_tilde_class, (t_method)peakenv_hold_tilde_ft2, gensym("ft2"), A_FLOAT, 0);
  class_addmethod(peakenv_hold_tilde_class, (t_method)peakenv_hold_tilde_reset, gensym("reset"), 0);
}
