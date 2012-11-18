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
 ******************************************************/

/* length :: get the length of a list */


#include "zexy.h"

static t_class *length_class;
typedef struct _length
{
  t_object x_obj;
} t_length;

static void length_list(t_length *x, t_symbol *s, int argc, t_atom *argv)
{
  ZEXY_USEVAR(s);
  ZEXY_USEVAR(argv);
  outlet_float(x->x_obj.ob_outlet, (t_float)argc);
}
static void length_any(t_length *x, t_symbol *s, int argc, t_atom *argv)
{
  ZEXY_USEVAR(s);
  ZEXY_USEVAR(argv);
  outlet_float(x->x_obj.ob_outlet, (t_float)argc+1);
}

static void *length_new(void)
{
  t_length *x = (t_length *)pd_new(length_class);
  outlet_new(&x->x_obj, &s_float);
  return (x);
}

void length_setup(void)
{
  length_class = class_new(gensym("length"), (t_newmethod)length_new, 0,
			 sizeof(t_length), 0, A_DEFFLOAT, 0);

  class_addlist(length_class, (t_method)length_list);
  class_addanything(length_class, (t_method)length_any);

  zexy_register("length");
}
