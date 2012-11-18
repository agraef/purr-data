/* Copyright (c) 2002 Damien HENRY.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* number NOT FINISHED BUT FUNCTIONAL*/

#include "m_pd.h"

t_class *number_class ;
typedef struct s_number
{
  t_pd     o_pd;
  t_int    o_refcount;
  t_float  number;
  t_float  b_min;
  t_float  b_max;
  t_int    bound;
} t_number;

t_class *pd_obj_number_class ;
typedef struct s_pd_obj_number
{
  t_object  x_obj;
  t_symbol *x_sym;
  t_number *x_num;
  t_float  *x_number;
  t_float  *b_min;
  t_float  *b_max;
  t_int    *bound;
  t_float   min;
  t_float   max;
  t_float   out_min;
  t_float   out_max;
} t_pd_obj_number;

t_number *number_get(t_symbol *s)
{
  t_number *num = (t_number *)pd_findbyclass(s,number_class);
  if (!num) {
    num = (t_number *)pd_new(number_class);
    num->number = 0;
    num->bound = 0;
    num->b_min = 0;
    num->b_max = 0;
    num->o_refcount = 0;
    pd_bind(&num->o_pd, s);
  }
  num->o_refcount++;
  return (num);
}

void number_release(t_symbol *s)
{ t_number *num = (t_number *)pd_findbyclass(s,number_class);
  if (num) {
    if (!--num->o_refcount) {
      pd_unbind(&num->o_pd, s);
      pd_free(&num->o_pd);
    }
  }
  else bug("value_release");
}

void number_check_bound(t_pd_obj_number *x)
{
  if (*x->bound != 0)
    {
      if (*x->x_number>*x->b_max) *x->x_number = *x->b_max ;
      if (*x->x_number<*x->b_min) *x->x_number = *x->b_min ;
    }
}

void number_help(t_pd_obj_number *x)
{
   post(" ");
   post("number v001");
   post("+ use : number $variable_name");
   post("+ symbol list :");
   post("+l+ help : this help !!!");
   post("+g+ float $f: set the value to $f ");
   post("+l+ bang : send the number");
   post("+g+ !x $f : set the value to $f");
   post("+l+ ?x : ask for the value the number");
   post("+g+ if $condition $f1 $f2 : ex.: if => 10 0");
   post("+l+ do_if $condition $value $selector : ex.: do_if => 10 test");
   post("+g+ add,sub,mult,div,inv : do some basic calculation");
   post("+l+ get : get a value from another number");
   post("+l+ set : set the name of the number");
   post("+l+ subspace $internal1 $internal2 $external1 $external1");
   post("+gl?+ round $round_value");
   post("+g+ bound $min $max");
   post("+l+ post");
   post(" ");
}

void number_post(t_pd_obj_number *x)
{
  startpost("%s",*x->x_sym);
  postfloat(*x->x_number);
  endpost();
}

void number_x_equal(t_pd_obj_number *x, t_floatarg f)
{
  *x->x_number = f ;
  number_check_bound(x);
}
void number_q_x(t_pd_obj_number *x)
{
  t_atom my_atom ;
  t_atom *my_pointer = &my_atom;
  SETFLOAT(my_pointer, *x->x_number);
  outlet_anything(x->x_obj.ob_outlet, gensym("x="), 1,my_pointer);
}

void number_float(t_pd_obj_number *x, t_floatarg f)
{
  *x->x_number = (f - x->out_min)*(x->max-x->min)/(x->out_max-x->out_min)+x->min ;
  number_check_bound(x);
}

void number_bang(t_pd_obj_number *x)
{ t_float temp ;
  temp =  (*x->x_number-x->min)/(x->max-x->min)*(x->out_max-x->out_min)+x->out_min;
  outlet_float(x->x_obj.ob_outlet,temp) ;
}

void number_bound(t_pd_obj_number *x, t_floatarg fmin, t_floatarg fmax)
{
  *x->bound = 1 ;
  *x->b_min = fmin;
  *x->b_max = fmax;
}

void number_add(t_pd_obj_number *x, t_floatarg f)
{
  *(x->x_number) += f ;
  number_check_bound(x);  
}

void number_sub(t_pd_obj_number *x, t_floatarg f)
{
  *(x->x_number) -= f ;
  number_check_bound(x);  
}

void number_mult(t_pd_obj_number *x, t_floatarg f)
{ 
  *(x->x_number) *= f ;
  number_check_bound(x);  
}

void number_div(t_pd_obj_number *x, t_floatarg f) 
{ 
  *(x->x_number) /= f ;
  number_check_bound(x);  
}

void number_inv(t_pd_obj_number *x)
{ 
  *x->x_number = 1 / *(x->x_number) ;
  number_check_bound(x);  
}

void number_subspace(t_pd_obj_number *x, t_floatarg f1, t_floatarg f2, t_floatarg f3, t_floatarg f4)
{
  if ((f1==f2)|(f3==f4))
  {
    x->min = 0 ; x->max = 1 ; x->out_min = 0 ; x->out_max = 1 ;
    post("%s ERROR wrong Subspace",*x->x_sym);
  }
  else
  {
    x->min = f1 ; x->max = f2 ; x->out_min = f3 ; x->out_max = f4 ;
  }
}

void number_if(t_pd_obj_number *x,t_symbol *s,t_floatarg f1,t_floatarg f2)
{
  if (s == gensym("==")) {if(*x->x_number == f1) *x->x_number = f2 ;}
  if (s == gensym("!=")) {if(*x->x_number != f1) *x->x_number = f2 ;}
  if (s == gensym(">"))  {if(*x->x_number >  f1) *x->x_number = f2 ;}
  if (s == gensym(">=")) {if(*x->x_number >= f1) *x->x_number = f2 ;}
  if (s == gensym("<"))  {if(*x->x_number <  f1) *x->x_number = f2 ;}
  if (s == gensym("<=")) {if(*x->x_number <= f1) *x->x_number = f2 ;}
}

void number_do_if(t_pd_obj_number *x,t_symbol *cond,t_floatarg f1,t_symbol *sel)
{
  t_atom my_atom ;
  t_atom *my_pointer = &my_atom;
  SETFLOAT(my_pointer, *x->x_number);
  post("t1 %f",f1);
  if (cond == gensym("==")) {if(*x->x_number == f1) outlet_anything(x->x_obj.ob_outlet, sel, 1,my_pointer);}
  if (cond == gensym("!=")) {if(*x->x_number != f1) outlet_anything(x->x_obj.ob_outlet, sel, 1,my_pointer);}
  if (cond == gensym(">"))  {if(*x->x_number >  f1) outlet_anything(x->x_obj.ob_outlet, sel, 1,my_pointer);}
  if (cond == gensym(">=")) {if(*x->x_number >= f1)     post("test %f",f1);}
  if (cond == gensym("<"))  {if(*x->x_number <  f1) outlet_anything(x->x_obj.ob_outlet, sel, 1,my_pointer);}
  if (cond == gensym("<=")) {if(*x->x_number <= f1) outlet_anything(x->x_obj.ob_outlet, sel, 1,my_pointer);}
}

void number_get_(t_pd_obj_number *x,t_symbol *s)
{*x->x_num = *number_get(s);}

void number_set_(t_pd_obj_number *x,t_symbol *s)
{ x->x_num = number_get(s);}

void number_free(t_pd_obj_number *x){ number_release(x->x_sym); }

void *number_new(t_symbol *s)
{
    t_pd_obj_number *x = (t_pd_obj_number *)pd_new(pd_obj_number_class);
    x->x_sym = s;
    x->x_num = number_get(s);
    x->x_number=&(x->x_num->number) ;
    x->min = 0 ; x->max = 1 ; x->out_min = 0 ; x->out_max = 1 ;
    x->b_min =&x->x_num->b_min ;
    x->b_max =&x->x_num->b_max ;
    x->bound =&x->x_num->bound ;
    outlet_new(&x->x_obj, &s_float);
    return (void *)x;
}

void number_setup(void)
{
    pd_obj_number_class = class_new(gensym("number"), (t_newmethod)number_new,
     (t_method)number_free, sizeof(t_pd_obj_number), 0,A_DEFSYMBOL, 0);
    class_addcreator((t_newmethod)number_new, gensym("x"), A_DEFSYM, 0);
    class_addfloat(pd_obj_number_class,number_float);
    class_addbang(pd_obj_number_class,number_bang);
    class_addmethod(pd_obj_number_class, (t_method)number_x_equal, gensym("!x"), A_FLOAT, 0);
    class_addmethod(pd_obj_number_class, (t_method)number_q_x,   gensym("?x"), 0);
    class_addmethod(pd_obj_number_class, (t_method)number_post,  gensym("post"), 0);
    class_addmethod(pd_obj_number_class, (t_method)number_add,   gensym("add"),A_FLOAT, 0);
    class_addmethod(pd_obj_number_class, (t_method)number_sub,   gensym("sub"),A_FLOAT, 0);
    class_addmethod(pd_obj_number_class, (t_method)number_mult,  gensym("mult"),A_FLOAT, 0);
    class_addmethod(pd_obj_number_class, (t_method)number_div,   gensym("div"),A_FLOAT, 0);
    class_addmethod(pd_obj_number_class, (t_method)number_inv,   gensym("inv"),0);
    class_addmethod(pd_obj_number_class, (t_method)number_if,    gensym("if"),A_DEFSYMBOL,A_FLOAT,A_FLOAT,0);
    class_addmethod(pd_obj_number_class, (t_method)number_set_,  gensym("set"),A_DEFSYMBOL, 0);
    class_addmethod(pd_obj_number_class, (t_method)number_get_,  gensym("get"),A_DEFSYMBOL, 0);
    class_addmethod(pd_obj_number_class, (t_method)number_do_if, gensym("do_if"),A_DEFSYMBOL,A_FLOAT,A_DEFSYMBOL,0);
    class_addmethod(pd_obj_number_class, (t_method)number_subspace,   gensym("subspace"),A_FLOAT,A_FLOAT,A_FLOAT,A_FLOAT, 0);
    class_addmethod(pd_obj_number_class, (t_method)number_bound,   gensym("bound"),A_FLOAT,A_FLOAT, 0);
    class_addmethod(pd_obj_number_class, (t_method)number_help,  gensym("help"), 0);
    class_sethelpsymbol(pd_obj_number_class, gensym("xgui/help_number"));
    /* Declare the class that will contain the value */
    number_class = class_new(gensym("number"), 0, 0, sizeof(t_number), CLASS_PD, 0);

}
