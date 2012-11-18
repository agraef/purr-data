
/*****************************************************
 *
 * receivecanvas - implementation file
 *
 * copyleft (c) 2009, IOhannes m zmölnig
 *
 *   forum::für::umläute
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/


/* 
 * this object provides a way to receive messages to upstream canvases
 * by default it receives messages to the containing canvas, but you can give the "depth" as argument;
 * e.g. [receivecanvas 1] will receive messages to the parent of the containing canvas
 */

/* NOTES:
 *  it would be _really_ nice to get all the messages that are somehow "sent" to a (parent) object,
 *  no matter whether using typedmess() or using sendcanvas()
 *  this would require (however) to overwrite and proxy the classmethods for canvas which is a chore
 *
 *  currently this objects only gets the messages from typedmess()...
 */

#include "m_pd.h"
#include "g_canvas.h"

#include <stdio.h>

/* ------------------------- receivecanvas ---------------------------- */

static t_class *receivecanvas_class;

typedef struct _receivecanvas
{
  t_object  x_obj;
  t_symbol  *x_sym;
} t_receivecanvas;

static void receivecanvas_anything(t_receivecanvas *x, t_symbol*s, int argc, t_atom*argv)
{
  outlet_anything(x->x_obj.ob_outlet, s, argc, argv);
}

static void receivecanvas_free(t_receivecanvas *x)
{
  if(x->x_sym)
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
}

static void *receivecanvas_new(t_floatarg f)
{
  t_receivecanvas *x = (t_receivecanvas *)pd_new(receivecanvas_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
  int depth=(int)f;
  if(depth<0)depth=0;

  while(depth && canvas) {
    canvas=canvas->gl_owner;
    depth--;
  }

  x->x_sym=NULL;

  if(canvas) {
    char buf[40];
    snprintf(buf, 40, ".x%lx", (t_int)canvas);
    x->x_sym=gensym(buf);

    pd_bind(&x->x_obj.ob_pd, x->x_sym);

  }

  outlet_new(&x->x_obj, 0);

  return (x);
}

void receivecanvas_setup(void)
{
  receivecanvas_class = class_new(gensym("receivecanvas"), (t_newmethod)receivecanvas_new,
                               (t_method)receivecanvas_free, sizeof(t_receivecanvas), CLASS_NOINLET, A_DEFFLOAT, 0);
  class_addanything(receivecanvas_class, (t_method)receivecanvas_anything);
}
