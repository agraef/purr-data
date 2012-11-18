#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static t_class *distribute_class;

typedef struct _distribute {
  t_object x_obj;
  t_int counter;
  t_int max;
  t_outlet *out[128];//,*r_out,*n_out;
} t_distribute;

void distribute_float(t_distribute *x,t_float in)
{
outlet_float(x->out[x->counter],in);
x->counter++;
if (x->counter == x->max)
x->counter = 0;
//x->out[x->counter] = in;
}

void *distribute_new(t_floatarg f1)
{
	int i;
	t_distribute *x = (t_distribute *)pd_new(distribute_class);
	x->counter = 0;
	if ((f1>0) && (f1<128))
	x->max = (t_int) f1;
	else x->max = 8;
  	for (i=0;i<x->max;i++)
  	x->out[i] = outlet_new(&x->x_obj,&s_float);
  	return (void *)x;
}
