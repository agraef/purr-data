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


/* ------------------------- list2int ------------------------------- */

/* cast each float of a list (or anything) to integer */

static t_class *list2int_class;

static void list2int_any(t_mypdlist *x, t_symbol *s, int argc, t_atom *argv)
{
  t_atom *ap;
  if (x->x_n != argc) {
    freebytes(x->x_list, x->x_n * sizeof(t_atom));
    x->x_n = argc;
    x->x_list = copybytes(argv, argc * sizeof(t_atom));
  } else memcpy(x->x_list, argv, argc * sizeof(t_atom));
  ap = x->x_list;
  while(argc--){
    if(ap->a_type == A_FLOAT)ap->a_w.w_float=(int)ap->a_w.w_float;
    ap++;
  }
  outlet_anything(x->x_obj.ob_outlet, s, x->x_n, x->x_list);
}
static void list2int_bang(t_mypdlist *x)
{  outlet_bang(x->x_obj.ob_outlet);}
static void list2int_float(t_mypdlist *x, t_float f)
{  outlet_float(x->x_obj.ob_outlet, (int)f);}
static void list2int_symbol(t_mypdlist *x, t_symbol *s)
{  outlet_symbol(x->x_obj.ob_outlet, s);}
static void list2int_pointer(t_mypdlist *x, t_gpointer *p)
{  outlet_pointer(x->x_obj.ob_outlet, p);}

static void *list2int_new(t_symbol *s, int argc, t_atom *argv)
{
  t_mypdlist *x = (t_mypdlist *)pd_new(list2int_class);
  outlet_new(&x->x_obj, 0);
  x->x_n = 0;
  x->x_list = 0;

  list2int_any(x, s, argc, argv);
  return (x);
}


static void mypdlist_free(t_mypdlist *x)
{
  freebytes(x->x_list, x->x_n * sizeof(t_atom)); 
}

void list2int_setup(void)
{
  list2int_class = class_new(gensym("list2int"), (t_newmethod)list2int_new, 
			 (t_method)mypdlist_free, sizeof(t_mypdlist), 0, A_GIMME, 0);
  class_addcreator((t_newmethod)list2int_new, gensym("l2i"), A_GIMME, 0);
  class_addanything(list2int_class, list2int_any);
  class_addlist(list2int_class, list2int_any);
  class_addbang(list2int_class, list2int_bang);
  class_addfloat(list2int_class, list2int_float);
  class_addsymbol(list2int_class, list2int_symbol);
  class_addpointer(list2int_class, list2int_pointer);
  zexy_register("list2int");
}

void l2i_setup(void)
{
  list2int_setup();
}
