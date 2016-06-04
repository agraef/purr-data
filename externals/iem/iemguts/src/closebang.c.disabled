/******************************************************
 *
 * closebang - implementation file
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

#ifndef LB_CLOSE
# warning compiling against a version of Pd without closebang support
# define LB_CLOSE 2
#endif


/* ------------------------- closebang ---------------------------- */

static t_class *closebang_class;

typedef struct _closebang
{
  t_object  x_obj;
} t_closebang;


static void closebang_loadbang(t_closebang *x, t_float type) {
  if(LB_CLOSE == (int)type)
    outlet_bang(x->x_obj.ob_outlet);
}

static void *closebang_new(void)
{
  t_closebang *x = (t_closebang *)pd_new(closebang_class);
  outlet_new(&x->x_obj, &s_bang);
  return (x);
}

void closebang_setup(void)
{
  iemguts_boilerplate("[closebang]", 0);
#if (PD_MINOR_VERSION < 47)
  verbose(0, "[closebang] has been compiled against an incompatible version of Pd, proceeding anyway...");
#endif
  closebang_class = class_new(gensym("closebang"), (t_newmethod)closebang_new, 0,
			     sizeof(t_closebang), CLASS_NOINLET, 0);

  if(iemguts_check_atleast_pdversion(0,47,0))
    class_addmethod(closebang_class, (t_method)closebang_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
  else
    error("[closebang] requires Pd>=0.47");
}
