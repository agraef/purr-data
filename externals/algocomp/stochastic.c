//TODO Replace with classes fot stochastic

#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _logistic {
  t_object  x_obj;
  t_float x;
  t_float a;
  t_outlet *note_out;
} t_logistic;

void logistic_bang(t_logistic *x)
{
outlet_float(x->note_out, x->x);
x->x = x->a*x->x*(1-x->x);
}

void *logistic_new(t_floatarg f)
{
  t_logistic *x = (t_logistic *)pd_new(logistic_class);
  x->a = f;
  x->x = 0.5;
  x->note_out = outlet_new(&x->x_obj,&s_float);
  floatinlet_new(&x->x_obj, &x->a);
  return (void *)x;
}