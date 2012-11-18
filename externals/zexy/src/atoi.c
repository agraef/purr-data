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


#include "zexy.h"
#include <stdlib.h>

/*
 * atoi : ascii to integer
 */

/* atoi ::  ascii to integer */

static t_class *atoi_class;

typedef struct _atoi
{
  t_object x_obj;
  int i;
} t_atoi;
static void atoi_bang(t_atoi *x)
{
  outlet_float(x->x_obj.ob_outlet, (t_float)x->i);
}
static void atoi_float(t_atoi *x, t_floatarg f)
{
  x->i = f;
  outlet_float(x->x_obj.ob_outlet, (t_float)x->i);
}
static void atoi_symbol(t_atoi *x, t_symbol *s)
{
  int base=10;
  const char* c = s->s_name;
  if(c[0]=='0'){
    base=8;
    if (c[1]=='x')base=16;
  }
  x->i=strtol(c, 0, base);
  outlet_float(x->x_obj.ob_outlet, (t_float)x->i);
}
static void atoi_list(t_atoi *x, t_symbol *s, int argc, t_atom *argv)
{
  int base=10;
  const char* c;
  ZEXY_USEVAR(s);

  if (argv->a_type==A_FLOAT){
    x->i=atom_getfloat(argv);
    outlet_float(x->x_obj.ob_outlet, (t_float)x->i);
    return;
  }

  if (argc>1){
    base=atom_getfloat(argv+1);
    if (base<2) {
      error("atoi: setting base to 10");
      base=10;
    }
  }
  c=atom_getsymbol(argv)->s_name;
  x->i=strtol(c, 0, base);
  outlet_float(x->x_obj.ob_outlet, (t_float)x->i);
}

static void *atoi_new(void)
{
  t_atoi *x = (t_atoi *)pd_new(atoi_class);
  outlet_new(&x->x_obj, &s_float);
  return (x);
}

void atoi_setup(void)
{
  atoi_class = class_new(gensym("atoi"), (t_newmethod)atoi_new, 0,
			 sizeof(t_atoi), 0, A_DEFFLOAT, 0);

  class_addbang(atoi_class, (t_method)atoi_bang);
  class_addfloat(atoi_class, (t_method)atoi_float);
  class_addlist(atoi_class, (t_method)atoi_list);
  class_addsymbol(atoi_class, (t_method)atoi_symbol);
  class_addanything(atoi_class, (t_method)atoi_symbol);

  zexy_register("atoi");
}
