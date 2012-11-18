/* Copyright (c) 2002 Damien HENRY.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* message : NOT WORKING YET */


/*#include "m_pd.h"*/

t_class *message_class ;
typedef struct s_message
{   t_pd o_pd;
    int o_refcount;
    t_message ??? o_message; 
} t_message;

t_class *pd_obj_message_class ;
typedef struct s_pd_obj_message
{
  t_object  x_obj;
  t_symbol *x_sym;
  t_float  *x_message;
} t_pd_obj_message;

t_float *message_get(t_symbol *s) 
{ t_message *num = (t_message *)pd_findbyclass(s,message_class);
  if (!num) {
    num = (t_message *)pd_new(message_class);
    num->o_message = 0;
    num->o_refcount = 0;
    pd_bind(&num->o_pd, s);
  }
  num->o_refcount++;
  return (&num->o_message);
}

void message_release(t_symbol *s)
{ t_message *num = (t_message *)pd_findbyclass(s,message_class);
  if (num) {
    if (!--num->o_refcount) {
      pd_unbind(&num->o_pd, s);
      pd_free(&num->o_pd);
    }
  }
  else bug("value_release");
}

void message_help(t_seg *x)
{
   post(" ");
   post("message v001");
   post("+ symbol list :");
   post("++ help : this help !!!");
   post("++ float : set the float ");
   post("++ bang : send the message");
   post(" ");
}

void message_float(t_pd_obj_message *x, t_floatarg f)
{ *x->x_message = f ;}

void message_q_x(t_pd_obj_message *x)
{
  t_atom my_atom ;
  t_atom *my_pointer = &my_atom;
  SETFLOAT(my_pointer, *x->x_message);
  outlet_anything(x->x_obj.ob_outlet, gensym("x="), 1,my_pointer);
}

void message_bang(t_pd_obj_message *x)
{ outlet_float(x->x_obj.ob_outlet, *x->x_message);}

void message_get_(t_pd_obj_message *x,t_symbol *s)
{*x->x_message = *message_get(s);} 

void message_set_(t_pd_obj_message *x,t_symbol *s)
{ x->x_message = message_get(s);}    

void message_to(t_pd_obj_message *x,t_floatarg f1,t_floatarg f2)
{ post("not implemented yet"); }

void message_free(t_pd_obj_message *x){ message_release(x->x_sym); }

void *message_new(t_symbol *s,t_floatarg f)
{
    t_pd_obj_message *x = (t_pd_obj_message *)pd_new(pd_obj_message_class);
    x->x_sym = s;
    x->x_message = message_get(s);
    outlet_new(&x->x_obj, &s_float);
    return (void *)x;
}

void message_setup(void)
{
    pd_obj_message_class = class_new(gensym("message"), (t_newmethod)message_new,
     (t_method)message_free, sizeof(t_pd_obj_message), 0,A_DEFSYMBOL, 0);
    class_addcreator((t_newmethod)message_new, gensym("x"), A_DEFSYM, 0);    
    class_addfloat(pd_obj_message_class,message_float);
    class_addbang(pd_obj_message_class,message_bang);
    class_addmethod(pd_obj_message_class, (t_method)message_float, gensym("!x"), A_FLOAT, 0);
    class_addmethod(pd_obj_message_class, (t_method)message_q_x,   gensym("?x"), 0);
    class_addmethod(pd_obj_message_class, (t_method)message_add,   gensym("add"),A_FLOAT, 0);
    class_addmethod(pd_obj_message_class, (t_method)message_sub,   gensym("sub"),A_FLOAT, 0);
    class_addmethod(pd_obj_message_class, (t_method)message_mult,  gensym("mult"),A_FLOAT, 0);
    class_addmethod(pd_obj_message_class, (t_method)message_div,   gensym("div"),A_FLOAT, 0);
    class_addmethod(pd_obj_message_class, (t_method)message_inv,   gensym("inv"),0);
    class_addmethod(pd_obj_message_class, (t_method)message_if,    gensym("if"),A_DEFSYMBOL,A_FLOAT,A_FLOAT,0);
    class_addmethod(pd_obj_message_class, (t_method)message_to,    gensym("to"),A_FLOAT,A_FLOAT, 0);
    class_addmethod(pd_obj_message_class, (t_method)message_set_,  gensym("set"),A_DEFSYMBOL, 0);
    class_addmethod(pd_obj_message_class, (t_method)message_get_,  gensym("get"),A_DEFSYMBOL, 0);
    class_addmethod(pd_obj_message_class, (t_method)message_help,  gensym("help"), 0);
    class_sethelpsymbol(pd_obj_message_class, gensym("xgui/help_message"));
    /* Declare the class that will contain the value */     
    message_class = class_new(gensym("message"), 0, 0, sizeof(t_message), CLASS_PD, 0);
         
}
