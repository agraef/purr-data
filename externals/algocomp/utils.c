/*
* Utilites to be used for algorithmic composition
*/

#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static t_class *map_class;

/*
 * linear mapping
 */
typedef struct _map {
  t_object  x_obj;
  t_float x;
  t_float min1,min2,max1,max2;
  t_float scale,translate;
  t_outlet *mapped_out;
} t_map;

void map_list(t_map *x,t_symbol *s, int argc, t_atom *argv)
{
	int i;
	float out;
	x->scale = (x->max2 - x->min2)/(x->max1 - x->min1);
	for (i=0;i<argc;i++) {
	out = (atom_getfloat(&argv[i]) - x->min1)*x->scale+x->min1+x->translate;
	SETFLOAT(&argv[i],out);
}
	outlet_list(x->mapped_out, &s_list, argc, argv);
}


void map_float(t_map *x,t_floatarg f)
{
float out;
x->scale = (x->max2 - x->min2)/(x->max1 - x->min1);
x->translate = x->min2 - x->min1;
out = (f-x->min1)*x->scale+x->min1+x->translate;
outlet_float(x->mapped_out, out);
}

void *map_new(t_floatarg min1, t_floatarg max1, t_floatarg min2,t_floatarg max2)
{
  t_map *x = (t_map *)pd_new(map_class);
  x->min1 = min1;
  x->min2 = min2;
  x->max1 = max1;
  x->max2 = max2;
  
  x->scale = (max2 - min2)/(max1 - min1);
  x->translate = min2 - min1;
  x->mapped_out = outlet_new(&x->x_obj,&s_float);
  floatinlet_new(&x->x_obj, &x->min2);
  floatinlet_new(&x->x_obj, &x->max2);
  return (void *)x;
}
