/*
  Copyright (c) 2016 Marco Matteo Markidis
  mm.markidis@gmail.com

  For information on usage and redistribution, and for a DISCLAIMER OF ALL
  WARRANTIES, see the file, "LICENSE.txt," in this distribution.

  Made while listening:
  Evan Parker Electro-Acoustic Ensemble -- Hasselt
*/
#include "m_pd.h"
#include <math.h>

#define BAD -999 

inline t_float badout(t_float f)
{
  return ((f) > (BAD) ? (f) : (BAD));
}

static t_class *atodb_class;

typedef struct _atodb {
  t_object x_obj;
  t_inlet *x_inlet;
  t_outlet *x_outlet;
} t_atodb;


void *atodb_new(void);
static t_int * atodb_perform(t_int *w);
static void atodb_dsp(t_atodb *x, t_signal **sp);

static t_int * atodb_perform(t_int *w)
{
  /*  t_atodb *x = (t_atodb *)(w[1]); */
  int n = (int)(w[2]);
  t_float *in = (t_float *)(w[3]);
  t_float *out = (t_float *)(w[4]);
    

  while(n--)
    *out++ = badout(20*log10(*in++));
  return (w + 5);
}


static void atodb_dsp(t_atodb *x, t_signal **sp)
{
  dsp_add(atodb_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *atodb_new(void)
{
  t_atodb *x = (t_atodb *)pd_new(atodb_class); 
  x->x_inlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  x->x_outlet = outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}


void atodb_tilde_setup(void) {
  atodb_class = class_new(gensym("atodb~"),
			  (t_newmethod) atodb_new,
			  0,
			  sizeof (t_atodb),
			  CLASS_NOINLET,
			  0);
    
  class_addmethod(atodb_class, (t_method) atodb_dsp, gensym("dsp"), 0);
}

