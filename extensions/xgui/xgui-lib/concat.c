/* Copyright (c) 2002 Damien HENRY.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* code for concat pd class */

#include "m_pd.h"
#include <string.h>

typedef struct s_pd_obj_concat
{
  t_object x_obj;
  t_symbol *selector ;
} t_pd_obj_concat;


void concat_help(t_pd_obj_concat *x)
{
   post(" ");
   post("concat v001");
   post("+ selector list :");
   post("++ help : this help !!!");
   post("++ anything : will return anything + the $arg1 in first ");
}

void concat_any_method(t_pd_obj_concat *x,t_symbol *s, int argc, t_atom *argv)
{
  int n = argc+1;
  if ((s==gensym("float"))||(s==gensym("symbol"))||(s==gensym("list")))
  {
    outlet_anything(x->x_obj.ob_outlet,x->selector, argc,argv);
  } else {
    t_atom *my_message  = (t_atom *)getbytes(n * sizeof(t_atom));
    SETSYMBOL(my_message, s);
    my_message++;
    memcpy(my_message, argv, argc * sizeof(t_atom));
    outlet_anything(x->x_obj.ob_outlet,x->selector, n,--my_message);
    freebytes(my_message, n * sizeof(t_atom));
  }  
}

void concat_set2add(t_pd_obj_concat *x, t_symbol *s)
{
  x->selector = s ;	
}

void concat_free(void) { }

t_class *concat_class;

void *concat_new(t_symbol *s)
{
    t_pd_obj_concat *x = (t_pd_obj_concat *)pd_new(concat_class);
    x->selector = s;
    outlet_new(&x->x_obj, &s_float);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("symbol"), gensym("s2add"));
    return (void *)x;
}

void concat_setup(void)
{
    concat_class = class_new(gensym("concat"), (t_newmethod)concat_new,(t_method)concat_free, sizeof( t_pd_obj_concat), 0,A_DEFSYMBOL, 0);
    class_addanything(concat_class, concat_any_method);
    class_addmethod(concat_class, (t_method)concat_set2add, gensym("s2add"), A_SYMBOL, 0);
    class_addmethod(concat_class, (t_method)concat_help, gensym("help"), 0);
    class_sethelpsymbol(concat_class, gensym("xgui/help_concat"));
}

