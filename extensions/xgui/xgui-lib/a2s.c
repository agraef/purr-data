/* Copyright (c) 2002 Damien HENRY.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* code for a2s pd class */

#include "m_pd.h"
#include <string.h>

typedef struct s_pd_obj_a2s
{
  t_object x_obj;
} t_pd_obj_a2s;


void a2s_help(t_pd_obj_a2s *x)
{
  post(" ");
  post("a2s v001");
  post("+ selector list :");
  post("++ help : this help !!!");
  post("++ symbol : will return the canvas & the obj");
  post(" ");  
}

void a2s_list(t_pd_obj_a2s *x, t_symbol *s, int argc, t_atom *argv)
{
  char      buffer[MAXPDSTRING] ;
  char     *a_string    ;
  int       a_string_l  ;
  t_binbuf *bbuf        ;
  int       i,l=0,k=0   ;
  
  bbuf = binbuf_new() ;
  for (i=0;i<argc;i++)
  {
    binbuf_clear(bbuf);
    binbuf_add(bbuf, 1, argv++);
    binbuf_gettext(bbuf, &a_string, &a_string_l);
    memcpy(&buffer[k],a_string,a_string_l) ;
    freebytes(a_string,a_string_l);
    k+=a_string_l ;
  }  
  buffer[k]=0;
  outlet_symbol(x->x_obj.ob_outlet,gensym(&buffer[0]));
  binbuf_free(bbuf);  
}

void a2s_free(void) { }

t_class *a2s_class;

void *a2s_new(void)
{
    t_pd_obj_a2s *x = (t_pd_obj_a2s *)pd_new(a2s_class);
    outlet_new(&x->x_obj, &s_float);
    return (void *)x;
}

void a2s_setup(void)
{
    a2s_class = class_new(gensym("a2s"), (t_newmethod)a2s_new,(t_method)a2s_free, sizeof( t_pd_obj_a2s), 0,A_DEFSYMBOL, 0);
    class_addlist(a2s_class, (t_method)a2s_list);
    class_addmethod(a2s_class, (t_method)a2s_help, gensym("help"), 0);
    class_sethelpsymbol(a2s_class, gensym("xgui/help_a2s"));
}
