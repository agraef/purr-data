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

/* ------------------------ blockswap~ ----------------------------- */

/* swaps a signalblock around it's center:
   {x[0], x[1], ... x[n-1]} --> {x[n-1], x[n-2], ... x[0]}
*/

static t_class *blockswap_class;

typedef struct _blockswap
{
  t_object x_obj;
  int doit;
  int blocksize;
  t_sample *blockbuffer;
} t_blockswap;

static void blockswap_float(t_blockswap *x, t_floatarg f)
{
  x->doit = (f != 0);
}

static t_int *blockswap_perform(t_int *w)
{
  t_blockswap	*x = (t_blockswap *)(w[1]);
  t_sample *in = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  int N = (int)(w[4]);
  int N2=N/2;
  if (x->doit) {
    int n=N2;
    t_sample *dummy=x->blockbuffer;
    while(n--)*dummy++=*in++;
    n=N-N2;
    while(n--)*out++=*in++;
    dummy=x->blockbuffer;
    n=N2;    
    while(n--)*out++=*dummy++;
  } else while (N--) *out++=*in++;
  return (w+5);
}

static void blockswap_dsp(t_blockswap *x, t_signal **sp)
{
  if (x->blocksize*2<sp[0]->s_n){
    if(x->blockbuffer)freebytes(x->blockbuffer, sizeof(*x->blockbuffer)*x->blocksize);
    x->blocksize = sp[0]->s_n/2;
    x->blockbuffer = getbytes(sizeof(*x->blockbuffer)*x->blocksize);
  }
  dsp_add(blockswap_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void blockswap_helper(t_blockswap *x)
{
  post("\n%c blockswap~-object for blockwise-swapping of a signal ", HEARTSYMBOL);
  post("'help' : view this\n"
       "signal~");
  post("outlet : signal~");
}

static void blockswap_free(t_blockswap *x)
{
  if(x->blockbuffer){
    freebytes(x->blockbuffer, sizeof(*x->blockbuffer)*x->blocksize);
  }
  x->blockbuffer=0;
}

static void *blockswap_new(void)
{
  t_blockswap *x = (t_blockswap *)pd_new(blockswap_class);
  outlet_new(&x->x_obj, gensym("signal"));
  x->doit = 1;
  x->blocksize=0;
  return (x);
}

void blockswap_tilde_setup(void)
{
  blockswap_class = class_new(gensym("blockswap~"), (t_newmethod)blockswap_new, (t_method)blockswap_free,
                              sizeof(t_blockswap), 0, A_NULL);
  class_addmethod(blockswap_class, nullfn, gensym("signal"), 0);
  class_addmethod(blockswap_class, (t_method)blockswap_dsp, gensym("dsp"), 0);
  
  class_addfloat(blockswap_class, blockswap_float);
  
  class_addmethod(blockswap_class, (t_method)blockswap_helper, gensym("help"), 0);
  zexy_register("blockswap~");
}
