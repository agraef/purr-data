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

#include "zexy.h"
#include <string.h>

/* ------------------------- a2l ------------------------------- */

/* convert anythings to lists, pass through the rest 
 * nowadays you can use [list] for this
 */

static t_class *a2l_class;

typedef struct _a2l
{
  t_object x_obj;
} t_a2l;

static void a2l_anything(t_a2l *x, t_symbol *s, int argc, t_atom *argv)
{
  int n = argc+1;
  t_atom *cur, *alist = (t_atom *)getbytes(n * sizeof(t_atom));

  cur = alist;
  SETSYMBOL(cur, s);
  cur++;

  memcpy(cur, argv, argc * sizeof(t_atom));

  outlet_list(x->x_obj.ob_outlet, gensym("list"), n, alist);

  freebytes(alist, n * sizeof(t_atom));

}

static void a2l_list(t_a2l *x, t_symbol *s, int argc, t_atom *argv)
{ outlet_list(x->x_obj.ob_outlet, s, argc, argv);}

static void a2l_float(t_a2l *x, t_floatarg f)
{ outlet_float(x->x_obj.ob_outlet, f);}

static void a2l_symbol(t_a2l *x, t_symbol *s)
{  outlet_symbol(x->x_obj.ob_outlet, s);}

static void a2l_pointer(t_a2l *x, t_gpointer *gp)
{  outlet_pointer(x->x_obj.ob_outlet, gp);}

static void a2l_bang(t_a2l *x)
{  outlet_bang(x->x_obj.ob_outlet);}

static void *a2l_new(void)
{
  t_a2l *x = (t_a2l *)pd_new(a2l_class);
  outlet_new(&x->x_obj, 0);
  return (x);
}

void a2l_setup(void)
{
  
  a2l_class = class_new(gensym("a2l"), (t_newmethod)a2l_new, 
			      0, sizeof(t_a2l), 0, 0);
  class_addcreator((t_newmethod)a2l_new, gensym("any2list"), 0);


  class_addbang    (a2l_class, a2l_bang);
  class_addfloat   (a2l_class, a2l_float);
  class_addsymbol  (a2l_class, a2l_symbol);
  class_addpointer (a2l_class, a2l_pointer);
  class_addlist    (a2l_class, a2l_list);
  class_addanything(a2l_class, a2l_anything);

  zexy_register("any2list");
}

void any2list_setup(void)
{
  a2l_setup();
}
