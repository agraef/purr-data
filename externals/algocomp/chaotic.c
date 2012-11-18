/*
* Chaotic function objects
*/

#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static t_class *logistic_class;
static t_class *henon_class;
static t_class *lorenz_class;


typedef struct _logistic {
  t_object  x_obj;
  t_float x;
  t_float a;
  t_outlet *note_out;
} t_logistic;

void logistic_bang(t_logistic *x)
{
if (x->a > 4) x->a = 4;
if (x->a <= 0) x->a = 0.01;
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

typedef struct _henon {
  t_object  x_obj;
  t_float x,y,B,A;
  t_outlet *x_out, *y_out;
} t_henon;

void henon_bang(t_henon *x)
{
outlet_float(x->x_out, x->x);
outlet_float(x->y_out, x->y);
x->x = x->y+1-x->A*x->x*x->x;
x->y = x->B*x->x;
}

void *henon_new(t_floatarg f1,t_floatarg f2)
{
  t_henon *x = (t_henon *)pd_new(henon_class);
  x->A = f1;
  x->B = f2;
  x->x = 0;
  x->y = 0;
  x->x_out = outlet_new(&x->x_obj,&s_float);
  x->y_out = outlet_new(&x->x_obj,&s_float);
  
  floatinlet_new(&x->x_obj, &x->A);
  floatinlet_new(&x->x_obj, &x->B);
  
  return (void *)x;
}

typedef struct _lorenz {
  t_object  x_obj;
  t_float x,y,z,B,R,S,xdot,ydot,zdot,dt;
  t_outlet *x_out, *y_out, *z_out;
} t_lorenz;

void lorenz_bang(t_lorenz *x)
{
outlet_float(x->x_out, x->x);
outlet_float(x->y_out, x->y);
outlet_float(x->z_out, x->z);
x->xdot = x->S * (x->y - x->x);
x->ydot = x->R * x->x - x->y  - (x->x * x->z);
x->zdot = x->x * x->y - x->B * x->z;
x->x = x->x + x->xdot * x->dt;
x->y = x->y + x->ydot * x->dt;
x->z = x->z + x->zdot * x->dt;

}

void *lorenz_new(t_floatarg f1,t_floatarg f2,t_floatarg f3,t_floatarg f4)
{
  t_lorenz *x = (t_lorenz *)pd_new(lorenz_class);
  x->S = f1;
  x->R = f2;
  x->B = f3;
  x->x = 0.1;
  x->y = 0.1;
  x->z = 0.1;
  x->dt = 0.01;
  if (f4 != 0) x->dt = f4;
  x->x_out = outlet_new(&x->x_obj,&s_float);
  x->y_out = outlet_new(&x->x_obj,&s_float);
  x->z_out = outlet_new(&x->x_obj,&s_float);
  floatinlet_new(&x->x_obj, &x->S);  
  floatinlet_new(&x->x_obj, &x->R);
  floatinlet_new(&x->x_obj, &x->B);  
  return (void *)x;
}
