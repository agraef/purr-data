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
 ******************************************************/

/* route_tilde :: get the route_tilde of a list */


#include "zexy.h"

static t_class *route_tilde_class;
typedef struct _route_tilde
{
  t_object x_obj;

  t_outlet*x_sigout, *x_msgout;
} t_route_tilde;

static void route_tilde_any(t_route_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  outlet_anything(x->x_msgout, s, argc, argv);
}

t_int *route_tilde_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    while (n--) *out++ = *in++;
    return (w+4);
}

static void route_tilde_dsp(t_route_tilde *x, t_signal **sp)
{
  if(sp)
    dsp_add(route_tilde_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
  else {
    route_tilde_any(x, gensym("dsp"), 0, 0);
  }
}

static void route_tilde_free(t_route_tilde *x)
{
  outlet_free(x->x_sigout);
  outlet_free(x->x_msgout);
}
static void *route_tilde_new(void)
{
  t_route_tilde *x = (t_route_tilde *)pd_new(route_tilde_class);
  x->x_sigout=outlet_new(&x->x_obj, &s_signal);
  x->x_msgout=outlet_new(&x->x_obj, 0);
  return (x);
}

void route_tilde_setup(void)
{
  route_tilde_class = class_new(gensym("route~"), (t_newmethod)route_tilde_new, (t_method)route_tilde_free,
			 sizeof(t_route_tilde), 0, A_NULL);

  class_addanything(route_tilde_class, (t_method)route_tilde_any);
  class_addmethod(route_tilde_class, nullfn, gensym("signal"), 0);
  class_addmethod(route_tilde_class, (t_method)route_tilde_dsp, gensym("dsp"), A_CANT, 0);

  zexy_register("route~");
}
