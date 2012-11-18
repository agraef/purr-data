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

/* 2305:forum::für::umläute:2001 */

/* connective objects */

#include "zexy.h"

/* ------------------------- repeat ------------------------------- */

/* a no-operation - just pass through what you get in */

static t_class *repeat_class;

typedef struct _repeat
{
  t_object x_obj;
  t_float fcount;
} t_repeat;

static void repeat_anything(t_repeat *x, t_symbol *s, int argc, t_atom *argv)
{ 
  int i;
  i=x->fcount;
  if (i<0)i=1;
  while(i--)outlet_anything(x->x_obj.ob_outlet, s, argc, argv);
}

static void *repeat_new(t_symbol*s, int argc, t_atom*argv)
{
  t_repeat *x = (t_repeat *)pd_new(repeat_class);
  ZEXY_USEVAR(s);
  if(argc){
    if(A_FLOAT==argv->a_type)
      x->fcount = atom_getfloat(argv);
    else return 0;
  } else x->fcount=2;
  floatinlet_new(&x->x_obj, &x->fcount);
  outlet_new(&x->x_obj, 0);
  return (x);
}

void repeat_setup(void)
{
  repeat_class = class_new(gensym("repeat"), (t_newmethod)repeat_new, 
			   0, sizeof(t_repeat), 0, A_GIMME, 0);
  class_addanything(repeat_class, repeat_anything);

  zexy_register("repeat");
}
