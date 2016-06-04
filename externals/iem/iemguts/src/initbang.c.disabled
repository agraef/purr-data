/******************************************************
 *
 * initbang - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   1901:forum::für::umläute:2016
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2 (or later)
 *
 ******************************************************/


/*
 * this object send out a bang when an abstraction was loaded
 * (but before the parent abstraction continues to load)
 * usage:
 *   + it can be used to create abstractions with dynamic numbers of iolets
 * nice, eh?
 */


#include "iemguts.h"

/* need g_canvas.h for loadbang-actions */
#include "g_canvas.h"

#ifndef LB_INIT
# warning compiling against a version of Pd without initbang support
# define LB_INIT 1
# define LB_CLOSE 2
#endif


/* ------------------------- initbang ---------------------------- */

static t_class *initbang_class;

typedef struct _initbang
{
  t_object  x_obj;
} t_initbang;


static void initbang_loadbang(t_initbang *x, t_float type) {
  if(LB_INIT == (int)type)
    outlet_bang(x->x_obj.ob_outlet);
}

static void *initbang_new(void)
{
  t_initbang *x = (t_initbang *)pd_new(initbang_class);
  outlet_new(&x->x_obj, &s_bang);
  return (x);
}

void initbang_setup(void)
{
  iemguts_boilerplate("[initbang]", 0);
#if (PD_MINOR_VERSION < 47)
  verbose(0, "[initbang] has been compiled against an incompatible version of Pd, proceeding anyway...");
#endif
  initbang_class = class_new(gensym("initbang"), (t_newmethod)initbang_new, 0,
			     sizeof(t_initbang), CLASS_NOINLET, 0);

  if(iemguts_check_atleast_pdversion(0,47,0))
    class_addmethod(initbang_class, (t_method)initbang_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
  else
    error("[initbang] requires Pd>=0.47");
}
