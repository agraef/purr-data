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

/*
  the long waited for swap~-object that does a byte swap
  of course, we unfortunately have to quantize the float-signal to 16bit (to get bytes)

  1110:forum::für::umläute:1999
*/

#include "zexy.h"

/* ------------------------ swap~ ----------------------------- */
#define FLOAT2SHORT 32768.
#define SHORT2FLOAT 1./32768.

static t_class *swap_class;

typedef struct _swap
{
  t_object x_obj;
  int swapper;
} t_swap;

static void swap_float(t_swap *x, t_floatarg f)
{
  x->swapper = (f != 0);
}

static void swap_bang(t_swap *x)
{
  x->swapper ^= 1;
}

static t_int *swap_perform(t_int *w)
{
  t_swap	*x = (t_swap *)(w[1]);
  t_sample *in = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  int n = (int)(w[4]);


  if (x->swapper) 
    while (n--) {
      short dummy = FLOAT2SHORT * *in++;
      *out++ = SHORT2FLOAT * (short)( ((dummy & 0xFF) << 8) | ((dummy & 0xFF00) >> 8) );
    }
  else while (n--) *out++ = *in++;
  
  return (w+5);
}

static void swap_dsp(t_swap *x, t_signal **sp)
{
  dsp_add(swap_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void swap_helper(void)
{
  post("\n%c swap~-object for byteswapping a signal", HEARTSYMBOL);
  post("<1/0>  : turn the swapper on/off\n"
       "'bang' : toggle the swapper on/off\n"
       "'help' : view this\n"
       "signal~");
  post("outlet : signal~");
}

static void *swap_new(void)
{
  t_swap *x = (t_swap *)pd_new(swap_class);
  outlet_new(&x->x_obj, gensym("signal"));
  x->swapper = 1;
  return (x);
}

void swap_tilde_setup(void)
{
  swap_class = class_new(gensym("swap~"), (t_newmethod)swap_new, 0,
			 sizeof(t_swap), 0, A_NULL);
  class_addmethod(swap_class, nullfn, gensym("signal"), 0);
  class_addmethod(swap_class, (t_method)swap_dsp, gensym("dsp"), 0);
  
  class_addfloat(swap_class, swap_float);
  class_addbang(swap_class, swap_bang);
  
  class_addmethod(swap_class, (t_method)swap_helper, gensym("help"), 0);
  zexy_register("swap~");
}
