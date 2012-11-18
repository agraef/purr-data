/******************************************************
 *
 * zexy - implementation file
 *
 * copyleft (c) IOhannes m zm-bölnig-A
 *
 *   1999:forum::f-bür::umläute:2007-A
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/

/* based on tabread4~ which is part of pd */

#include "zexy.h"

/******************** tabread4~~ ***********************/


static t_class *tabread4_tilde_class;

typedef struct _tabread4_tilde
{
  t_object x_obj;
  int x_npoints;
  zarray_t *x_vec;
  t_symbol *x_arrayname;
  t_float x_f;
} t_tabread4_tilde;

static void *tabread4_tilde_new(t_symbol *s)
{
  t_tabread4_tilde *x = (t_tabread4_tilde *)pd_new(tabread4_tilde_class);
  x->x_arrayname = s;
  x->x_vec = 0;
  x->x_npoints=0;

  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, &s_signal);
  x->x_f = 0;
  return (x);
}

static t_int *tabread4_tilde_perform(t_int *w)
{
  t_tabread4_tilde *x = (t_tabread4_tilde *)(w[1]);
  t_sample *in = (t_sample *)(w[2]);
  t_sample *in1 = (t_sample *)(w[3]);
  t_sample *out = (t_sample *)(w[4]);
  int n = (int)(w[5]);    
  int maxindex;
  zarray_t *buf = x->x_vec, *wp;
  int i;
    
  maxindex = x->x_npoints - 3;
  if (!buf){
    while (n--) *out++ = 0;
    return (w+6);
  }
  for (i = 0; i < n; i++)
    {
      t_sample in0_s=*in++;
      t_sample in1_s=*in1++;
      double findex = (double)in0_s + (double)in1_s;
      long int index = findex;
      double frac;
      t_sample a,  b,  c,  d, cminusb;
      if (index < 1)
        index = 1, frac = 0;
      else if (index > maxindex)
        index = maxindex, frac = 1;
      else frac = findex - index;

      wp = buf + index;

      a = zarray_getfloat(wp,-1);
      b = zarray_getfloat(wp, 0);
      c = zarray_getfloat(wp, 1);
      d = zarray_getfloat(wp, 2);

      cminusb = c-b;
      *out++ = b + frac * (
                           cminusb - 0.1666667f * (1.-frac) * (
                                                               (d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
                                                               )
                           );
    }
  return (w+6);
}

static void tabread4_tilde_set(t_tabread4_tilde *x, t_symbol *s)
{
  t_garray *a;
    
  x->x_arrayname = s;
  if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
      if (*s->s_name)
        pd_error(x, "tabread4~~: %s: no such array", x->x_arrayname->s_name);
      x->x_vec = 0;
    }
  else if (!zarray_getarray(a, &x->x_npoints, &x->x_vec))
    {
      pd_error(x, "%s: bad template for tabread4~~", x->x_arrayname->s_name);
      x->x_vec = 0;
    }
  else garray_usedindsp(a);
}

static void tabread4_tilde_dsp(t_tabread4_tilde *x, t_signal **sp)
{
  tabread4_tilde_set(x, x->x_arrayname);

  dsp_add(tabread4_tilde_perform, 5, x,
          sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);

}

static void tabread4_tilde_free(t_tabread4_tilde *x)
{
}

void tabread4_tilde_tilde_setup(void)
{
  tabread4_tilde_class = class_new(gensym("tabread4~~"),
                                   (t_newmethod)tabread4_tilde_new, (t_method)tabread4_tilde_free,
                                   sizeof(t_tabread4_tilde), 0, A_DEFSYM, 0);
  CLASS_MAINSIGNALIN(tabread4_tilde_class, t_tabread4_tilde, x_f);
  class_addmethod(tabread4_tilde_class, (t_method)tabread4_tilde_dsp,
                  gensym("dsp"), 0);
  class_addmethod(tabread4_tilde_class, (t_method)tabread4_tilde_set,
                  gensym("set"), A_SYMBOL, 0);

  zexy_register("tabread4~~");
}

void setup_tabread40x7e_tilde(void)
{
  tabread4_tilde_tilde_setup();
}

void setup_tabread40x7e0x7e(void)
{
  tabread4_tilde_tilde_setup();
}
