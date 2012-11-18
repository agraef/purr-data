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


/* ------------------------ pack~ ----------------------------- */
/* pack the signal-vector to a float-package */

static t_class *sigpack_class;

typedef struct _sigpack
{
  t_object x_obj;

  int vector_length;
  t_atom *buffer;
  t_clock*x_clock;
  int x_outputindsp;
} t_sigpack;

static void sigpack_tick(t_sigpack*x)
{
  outlet_list(x->x_obj.ob_outlet, &s_list, x->vector_length, x->buffer);
}

static t_int *sigpack_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sigpack *x = (t_sigpack *)w[2];
  int n = (int)(w[3]), i = 0;
  t_atom *buf = x->buffer;

  while (n--) {
    t_float f=*in++;
    SETFLOAT(&buf[i], f);
    i++;
  }
  if(x->x_outputindsp) {
    sigpack_tick(x);
  } else {
    clock_delay(x->x_clock, 0);
  }

  return (w+4);
}

static void sigpack_dsp(t_sigpack *x, t_signal **sp)
{
  if (x->vector_length != sp[0]->s_n) {
    freebytes(x->buffer, x->vector_length * sizeof(t_atom));
    x->vector_length = sp[0]->s_n;
    x->buffer = (t_atom *)getbytes(x->vector_length * sizeof(t_atom));
  }
  dsp_add(sigpack_perform, 3, sp[0]->s_vec, x, sp[0]->s_n);
}

static void sigpack_free(t_sigpack*x)
{
  clock_free(x->x_clock);
}

static void *sigpack_new(void)
{
  t_sigpack *x = (t_sigpack *)pd_new(sigpack_class);
  x->vector_length = 0;
  x->buffer = 0;
  outlet_new(&x->x_obj, gensym("list"));
  x->x_clock=clock_new(x, (t_method)sigpack_tick);

  x->x_outputindsp=0;

  return (x);
}

static void sigpack_help(void)
{
  post("pack~\t:: outputs the signal-vectors as float-packages");
}

void pack_tilde_setup(void)
{
  sigpack_class = class_new(gensym("pack~"), (t_newmethod)sigpack_new, (t_method)sigpack_free,
                            sizeof(t_sigpack), 0, A_DEFFLOAT, 0);
  class_addmethod(sigpack_class, nullfn, gensym("signal"), 0);
  class_addmethod(sigpack_class, (t_method)sigpack_dsp, gensym("dsp"), 0);

  class_addmethod(sigpack_class, (t_method)sigpack_help, gensym("help"), 0);

  zexy_register("pack~");
}
