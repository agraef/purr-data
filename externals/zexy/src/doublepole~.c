/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/*  "filters", both linear and nonlinear.
*/
#include "m_pd.h"
#include <math.h>

//* ---------------- doublepole~ - raw doublepole filter ----------------- */

typedef struct doublepolectl {
  t_sample c_x1;
  t_sample c_x2;
  t_sample c_fb1;
  t_sample c_fb2;
} t_doublepolectl;

typedef struct sigdoublepole {
  t_object x_obj;
  t_float x_f;
  t_doublepolectl x_cspace;
  t_doublepolectl *x_ctl;
} t_sigdoublepole;

static t_class *sigdoublepole_class = NULL;

static void sigdoublepole_list(t_sigdoublepole *x, t_symbol *s, int argc,
                               t_atom *argv);

static void *sigdoublepole_new(t_symbol *s, int argc, t_atom *argv)
{
  t_sigdoublepole *x = (t_sigdoublepole *)pd_new(sigdoublepole_class);
  outlet_new(&x->x_obj, &s_signal);
  x->x_ctl = &x->x_cspace;
  x->x_cspace.c_x1 = x->x_cspace.c_x2 = 0;
  sigdoublepole_list(x, s, argc, argv);
  x->x_f = 0;
  return (x);
}

static t_int *sigdoublepole_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_doublepolectl *c = (t_doublepolectl *)(w[3]);
  int n = (t_int)(w[4]);
  int i;
  t_sample last = c->c_x1;
  t_sample prev = c->c_x2;
  t_sample fb1 = c->c_fb1;
  t_sample fb2 = c->c_fb2;
  for (i = 0; i < n; i++) {
    t_sample output =  *in++ + fb1 * last + fb2 * prev;
    if (PD_BIGORSMALL(output)) {
      output = 0;
    }
    *out++ = output;
    prev = last;
    last = output;
  }
  c->c_x1 = last;
  c->c_x2 = prev;
  return (w+5);
}

static void sigdoublepole_list(t_sigdoublepole *x, t_symbol *s, int argc,
                               t_atom *argv)
{
  t_float fb1 = atom_getfloatarg(0, argc, argv);
  t_float fb2 = atom_getfloatarg(1, argc, argv);
  t_float discriminant = fb1 * fb1 + 4 * fb2;
  t_doublepolectl *c = x->x_ctl;
  if (discriminant < 0) { /* imaginary roots -- resonant filter */
    /* they're conjugates so we just check that the product
    is less than one */
    if (fb2 >= -1.0f) {
      goto stable;
    }
  } else { /* real roots */
    /* check that the parabola 1 - fb1 x - fb2 x^2 has a
        vertex between -1 and 1, and that it's nonnegative
        at both ends, which implies both roots are in [1-,1]. */
    if (fb1 <= 2.0f && fb1 >= -2.0f &&
        1.0f - fb1 -fb2 >= 0 && 1.0f + fb1 - fb2 >= 0) {
      goto stable;
    }
  }
  /* if unstable, just bash to zero */
  fb1 = fb2 = 0;
stable:
  c->c_fb1 = fb1;
  c->c_fb2 = fb2;
}

static void sigdoublepole_set(t_sigdoublepole *x, t_symbol *s, int argc,
                              t_atom *argv)
{
  t_doublepolectl *c = x->x_ctl;
  c->c_x1 = atom_getfloatarg(0, argc, argv);
  c->c_x2 = atom_getfloatarg(1, argc, argv);
}

static void sigdoublepole_dsp(t_sigdoublepole *x, t_signal **sp)
{
  dsp_add(sigdoublepole_perform, 4,
          sp[0]->s_vec, sp[1]->s_vec,
          x->x_ctl, (t_int)sp[0]->s_n);

}

void doublepole_tilde_setup(void)
{
  sigdoublepole_class = class_new(gensym("doublepole~"),
                                  (t_newmethod)sigdoublepole_new,
                                  0, sizeof(t_sigdoublepole), 0, A_GIMME, 0);
  CLASS_MAINSIGNALIN(sigdoublepole_class, t_sigdoublepole, x_f);
  class_addmethod(sigdoublepole_class, (t_method)sigdoublepole_dsp,
                  gensym("dsp"), A_CANT, 0);
  class_addlist(sigdoublepole_class, sigdoublepole_list);
  class_addmethod(sigdoublepole_class, (t_method)sigdoublepole_set,
                  gensym("set"),
                  A_GIMME, 0);
  class_addmethod(sigdoublepole_class, (t_method)sigdoublepole_set,
                  gensym("clear"),
                  A_GIMME, 0);
}
