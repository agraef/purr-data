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

/* mavg :: moving average filter */

static t_class *mavg_class;

typedef struct _mavg
{
  t_object x_obj;

  t_float n_inv;
  t_float avg;
  int size;
  t_float *buf, *wp;
} t_mavg;

static void mavg_resize(t_mavg *x, t_float f)
{
  int i;
  t_float *dumbuf;

  f = (int)f;
  if ((f<1) || (f == x->size)) return;

  freebytes(x->buf, sizeof(t_float)*x->size);
  x->n_inv = 1.0/f;
  x->size = f;
  x->buf = getbytes(sizeof(t_float)*x->size);

  dumbuf = x->wp = x->buf;
  i = x->size;
  while(i--) *dumbuf++ = x->avg;
}

static void mavg_set(t_mavg *x, t_symbol *s, int argc, t_atom *argv)
{
  int i = x->size;
  t_float *dummy = x->buf;
  t_float f=(argc)?atom_getfloat(argv):x->avg;
  ZEXY_USEVAR(s);

  while (i--) *dummy++=f;

  x->wp = x->buf;
}

static void mavg_float(t_mavg *x, t_float f)
{
  int i = x->size;
  t_float dummy = 0;
  t_float *dumb = x->buf;

  *x->wp++ = f;
  if (x->wp == x->buf + x->size) x->wp = x->buf;

  while (i--) dummy += *dumb++;

  x->avg = dummy*x->n_inv;

  outlet_float(x->x_obj.ob_outlet,x->avg);
}

static void *mavg_new(t_floatarg f)
{
  t_mavg *x = (t_mavg *)pd_new(mavg_class);
  int i = (f<1)?2:f;
  t_float *dumbuf;

  outlet_new(&x->x_obj, &s_float);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym(""));

  x->buf = x->wp = (t_float *)getbytes(sizeof(t_float) * i);
  x->size = i;
  x->n_inv = 1.0f/(t_float)i;

  dumbuf = x->buf;
  while (i--) *dumbuf++=0;

  return (x);
}

static void mavg_help(void)
{
  post("mavg\t:: moving average filter");
}

void mavg_setup(void)
{
  mavg_class = class_new(gensym("mavg"), (t_newmethod)mavg_new, 0,
			 sizeof(t_mavg), 0, A_DEFFLOAT, 0);

  class_addfloat(mavg_class, (t_method)mavg_float);

  class_addmethod(mavg_class, (t_method)mavg_help, gensym("help"), 0);
  class_addmethod(mavg_class, (t_method)mavg_set, gensym("set"), A_GIMME, 0);
  class_addmethod(mavg_class, (t_method)mavg_resize, gensym(""), A_DEFFLOAT, 0);

  zexy_register("mavg");
}
