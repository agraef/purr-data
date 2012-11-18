
/* 1903:forum::für::umläute:2005 */

/*
 *  mulitplex   :  zpack a specified input to the output
 *
 * THINK: should the selector-inlet be the first or the last ???
 * pros/cons:
 *  the 1st inlet being the selector is not consistant with pd (hot/cold)
 *   but as it since the hot inlet is selectable, the whole object is not really consitant
 *  numbering would have to start with 1 (for the 1st not-leftmost inlet)
 * if the selector is rightmost this would mean: cold is right(most), hot is (somewhere) left
 * numbering would be ok
 *
 * conclusio: make the selector rightmost
 *
 */

#include "zexy.h"
#include <stdio.h>


/* ------------------------- zpack ------------------------------- */

/*
  a zpacker
*/

static t_class *zpack_class;
static t_class *zpackproxy_class;

typedef struct _zpack
{
  t_object x_obj;
  struct _zpackproxy  **x_proxy;

  t_inlet **in;

  t_atom*x_argv;
  int    x_argc;
} t_zpack;


typedef struct _zpackproxy
{
  t_pd  p_pd;
  t_zpack    *p_master;
  int id;
} t_zpackproxy;


static void setatom(t_zpack *x, t_atom*from, int to) {
  x->x_argv[to].a_type=from->a_type;
  x->x_argv[to].a_w   =from->a_w;
}

static void zpack_bang(t_zpack*x) {
  outlet_list(x->x_obj.ob_outlet, gensym("list"), x->x_argc, x->x_argv);
}

static void zpack_list0(t_zpack*x, t_symbol *s, int argc, t_atom *argv) {
  int i=0;
  for(i=0; i<argc && i<x->x_argc; i++)
    setatom(x, argv+i, i);
  zpack_bang(x);
}

static void zpack_list(t_zpackproxy *y, t_symbol *s, int argc, t_atom *argv)
{
  if(argc>0)
    setatom(y->p_master, argv, y->id);
}

static void *zpack_new(t_symbol *s, int argc, t_atom *argv)
{
  t_zpack *x = (t_zpack *)pd_new(zpack_class);
  int n =0;

  x->x_argc = (argc < 1)?2:argc;


  if(argc<1) {
    x->x_argv=(t_atom*)getbytes(2*sizeof(t_atom));
    SETFLOAT(x->x_argv+0, 0.f);
    SETFLOAT(x->x_argv+1, 0.f);
  } else {
    int i=0;
    x->x_argv=(t_atom*)getbytes(x->x_argc*sizeof(t_atom));
    for(i=0; i<x->x_argc; i++)
      setatom(x, argv+i, i);
  }

  x->in = (t_inlet **)getbytes(x->x_argc * sizeof(t_inlet *));
  x->x_proxy = (t_zpackproxy**)getbytes(x->x_argc * sizeof(t_zpackproxy*));

  x->in[0]     =0;
  x->x_proxy[0]=0;

  for (n = 1; n<x->x_argc; n++) {
    x->x_proxy[n]=(t_zpackproxy*)pd_new(zpackproxy_class);
    x->x_proxy[n]->p_master = x;
    x->x_proxy[n]->id=n;
    x->in[n] = inlet_new ((t_object*)x, (t_pd*)x->x_proxy[n], 0,0);
  }

  outlet_new(&x->x_obj, 0);
  return (x);
}

static void zpack_free(t_zpack*x){
  const int count = x->x_argc;

  if(x->in && x->x_proxy){
    int n=0;
    for(n=0; n<count; n++){
      if(x->in[n]){
        inlet_free(x->in[n]);
      }
      x->in[n]=0;
      if(x->x_proxy[n]){
        t_zpackproxy *y=x->x_proxy[n];
        y->p_master=0;
        y->id=0;
        pd_free(&y->p_pd);        
      }
      x->x_proxy[n]=0;      
    }
    freebytes(x->in, x->x_argc * sizeof(t_inlet *));
    freebytes(x->x_proxy, x->x_argc * sizeof(t_zpackproxy*));
  }
}

void zpack_setup(void)
{
  zpack_class = class_new(gensym("zexy/pack"), (t_newmethod)zpack_new,
			(t_method)zpack_free, sizeof(t_zpack), 0, A_GIMME,  0);
#if 0
  /* oops Pd-0.42 allows us to override built-ins
   * this is bad as long as the 2 objects are not compatible */
  class_addcreator((t_newmethod)zpack_new, gensym("pack"), A_GIMME, 0);
#endif
  class_addbang(zpack_class, zpack_bang);
  class_addlist(zpack_class, zpack_list0);

  zpackproxy_class = class_new(gensym("zpack proxy"), 0, 0,
			    sizeof(t_zpackproxy),
			    CLASS_PD | CLASS_NOINLET, 0);
  class_addlist(zpackproxy_class, zpack_list);


  zexy_register("pack");
}

void pack_setup(void)
{
  zpack_setup();
}

