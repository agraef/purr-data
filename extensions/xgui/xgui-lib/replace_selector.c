/* Copyright (c) 2002 Damien HENRY.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* code for replace_selector pd class */

#include "m_pd.h"
#include <string.h>

typedef struct s_pd_obj_replace_selector
{
  t_object x_obj;
  t_symbol *selector ;  
} t_pd_obj_replace_selector;


void replace_selector_help(t_pd_obj_replace_selector *x)
{
   post(" ");
   post("replace_selector v001");
   post("+ selector list :");
   post("++ help : this help !!!");
   post("++ anything : will return anything with a diferent selector");
}

void replace_selector_any_method(t_pd_obj_replace_selector *x,t_symbol *s, int argc, t_atom *argv)
{
    t_atom *my_message  = (t_atom *)getbytes(argc * sizeof(t_atom));
    memcpy(my_message, argv, argc * sizeof(t_atom));
    outlet_anything(x->x_obj.ob_outlet,x->selector, argc,my_message);
    freebytes(my_message, argc * sizeof(t_atom));
}

void replace_selector_set2add(t_pd_obj_replace_selector *x, t_symbol *s)
{
  x->selector = s ;	
}

void replace_selector_free(void) { }

t_class *replace_selector_class;

void *replace_selector_new(t_symbol *s)
{
    t_pd_obj_replace_selector *x = (t_pd_obj_replace_selector *)pd_new(replace_selector_class);
    x->selector = s;	
    outlet_new(&x->x_obj, &s_float);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("symbol"), gensym("s2add"));
    return (void *)x;
}

void replace_selector_setup(void)
{
    replace_selector_class = class_new(gensym("replace_selector"), (t_newmethod)replace_selector_new,(t_method)replace_selector_free, sizeof( t_pd_obj_replace_selector), 0,A_DEFSYMBOL, 0);
    class_addanything(replace_selector_class, replace_selector_any_method);
    class_addmethod(replace_selector_class, (t_method)replace_selector_set2add, gensym("s2add"), A_SYMBOL, 0);    
    class_addmethod(replace_selector_class, (t_method)replace_selector_help, gensym("help"), 0);
    class_sethelpsymbol(replace_selector_class, gensym("xgui/help_replace_selector"));
}

