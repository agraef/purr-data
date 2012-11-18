/******************************************************
 *
 * zexy - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   1999:forum::für::umläute:2004
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/

#include "zexy.h"

/* sum :: the sum of a list of floats */

static t_class *sum_class;

typedef struct _sum
{
  t_object x_obj;
} t_sum;

static void sum_list(t_sum *x, t_symbol *s, int argc, t_atom *argv)
{
  t_float sum = 0.f;
  ZEXY_USEVAR(s);

  while(argc--)sum+=atom_getfloat(argv++);

  outlet_float(x->x_obj.ob_outlet,sum);
}

static void *sum_new(void)
{
  t_sum *x = (t_sum *)pd_new(sum_class);

  outlet_new(&x->x_obj, &s_float);

  return (x);
}

static void sum_help(void)
{
  post("sum\t:: calculate the sum of a list of floats");
}

void sum_setup(void)
{
  sum_class = class_new(gensym("sum"), (t_newmethod)sum_new, 0,
			 sizeof(t_sum), 0, A_DEFFLOAT, 0);

  class_addlist(sum_class, (t_method)sum_list);
  class_addmethod(sum_class, (t_method)sum_help, gensym("help"), 0);

  zexy_register("sum");
}
