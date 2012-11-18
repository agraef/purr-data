/* Copyright (c) 2002 Damien HENRY.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* oname2l */

#include "m_pd.h"
#include <string.h>

typedef struct s_pd_obj_oname2l
{
  t_object x_obj;
} t_pd_obj_oname2l;


void oname2l_help(t_pd_obj_oname2l *x)
{
  post(" ");
  post("oname2l v001");
  post("+ selector list :");
  post("++ help : this help !!!");
  post("++ symbol : will return a list of symbol");
  post(" ");  
}

void oname2l_symbol(t_pd_obj_oname2l *x,t_symbol *s)
{
  int i,j,l,n=1,k=0;
  t_atom *my_message ;
  char *s2split ;
  t_symbol *a_symbol ;
  t_atom *an_atom ;
  for (l=0;s->s_name[l]!=0;l++)
  {
    if (s->s_name[l]=='/') {n++;} ;
  }
  s2split = (char *)getbytes(l+1) ;
  memcpy(s2split, s->s_name, l+1) ;
  my_message  = (t_atom *)getbytes(n * sizeof(t_atom));
  an_atom = my_message ;
  for (i=0;i<n;i++)
  {
    for (j=k;(s2split[j]!='/')&&(j<l);j++) {} ;
    s2split[j]=0 ;
    a_symbol = gensym(&(s2split[k]));
    SETSYMBOL(an_atom,a_symbol);
    an_atom++;
    k=j+1;
  }
  outlet_anything(x->x_obj.ob_outlet,gensym("list"), n,my_message);
  freebytes(my_message, n * sizeof(t_atom));
  freebytes(s2split, l );
}

void oname2l_free(void) { }

t_class *oname2l_class;

void *oname2l_new(void)
{
    t_pd_obj_oname2l *x = (t_pd_obj_oname2l *)pd_new(oname2l_class);
    outlet_new(&x->x_obj, &s_float);
    return (void *)x;
}

void oname2l_setup(void)
{
    oname2l_class = class_new(gensym("oname2l"), (t_newmethod)oname2l_new,(t_method)oname2l_free, sizeof( t_pd_obj_oname2l), 0,A_DEFSYMBOL, 0);
    class_addsymbol(oname2l_class, (t_method)oname2l_symbol);
    class_addmethod(oname2l_class, (t_method)oname2l_help, gensym("help"), 0);
    class_sethelpsymbol(oname2l_class, gensym("xgui/help_oname2l"));
}

