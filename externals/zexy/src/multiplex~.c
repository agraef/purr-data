/******************************************************
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


/* ------------------------------------------------------------------------------ */

/* mux~ : multiplex a specified signal to the output */

static t_class *mux_class;

typedef struct _mux {
  t_object x_obj;

  int input;

  int n_in;
  t_sample **in;
} t_mux;

static void mux_input(t_mux *x, t_floatarg f)
{
  if ((f>=0)&&(f<x->n_in)){
    x->input=f;
  } else
    error("multiplex: %d is channel out of range (0..%d)", (int)f, x->n_in);

}

static t_int *mux_perform(t_int *w)
{
  t_mux *x = (t_mux *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  int n = (int)(w[3]);
  
  t_sample *in = x->in[x->input];

  while(n--)*out++=*in++;

  return (w+4);
}

static void mux_dsp(t_mux *x, t_signal **sp)
{
  int n = 0;
  t_sample **dummy=x->in;

  for(n=0;n<x->n_in;n++)*dummy++=sp[n]->s_vec;

  dsp_add(mux_perform, 3, x, sp[n]->s_vec, sp[0]->s_n);
}

static void mux_helper(void)
{
  post("\n%c mux~\t:: multiplex a one of various signals to one outlet", HEARTSYMBOL);
  post("<#out>\t : the inlet-number (counting from 0) witch is routed to the outlet"
       "'help'\t : view this");
  post("creation : \"mux~ [arg1 [arg2...]]\"\t: the number of arguments equals the number of inlets\n");
}

static void mux_free(t_mux *x)
{
  freebytes(x->in, x->n_in * sizeof(t_sample *));
}

static void *mux_new(t_symbol *s, int argc, t_atom *argv)
{
  t_mux *x = (t_mux *)pd_new(mux_class);
  int i;
  ZEXY_USEVAR(s);
  ZEXY_USEVAR(argv);

  if (!argc)argc=2;
  x->n_in=argc;
  x->input=0;

  argc--;
  while(argc--)inlet_new(&x->x_obj,&x->x_obj.ob_pd,&s_signal,&s_signal);

  x->in = (t_sample **)getbytes(x->n_in * sizeof(t_sample *));
  i=x->n_in;
  while(i--)x->in[i]=0;

  outlet_new(&x->x_obj, gensym("signal"));

  return (x);
}

void multiplex_tilde_setup(void)
{
  mux_class = class_new(gensym("multiplex~"), (t_newmethod)mux_new, (t_method)mux_free, sizeof(t_mux), 0, A_GIMME, 0);
  class_addcreator((t_newmethod)mux_new, gensym("mux~"), A_GIMME, 0);

  class_addfloat(mux_class, mux_input);
  class_addmethod(mux_class, (t_method)mux_dsp, gensym("dsp"), 0);
  class_addmethod(mux_class, nullfn, gensym("signal"), 0);

  class_addmethod(mux_class, (t_method)mux_helper, gensym("help"), 0);
  zexy_register("multiplex~");
}
void mux_tilde_setup(void)
{
  multiplex_tilde_setup();
}

