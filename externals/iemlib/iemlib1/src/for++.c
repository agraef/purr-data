/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2011 */


#include "m_pd.h"
#include "iemlib.h"

/* ------------------------------------ for++ -------------------------------------- */
/* -- an optional timed counter (begin number, end number, metro time, increment) -- */

typedef struct _forpp
{
  t_object  x_obj;
  double    x_beg;
  double    x_end;
  double    x_delay;
  double    x_cur;
  double    x_incr;
  t_outlet  *x_out_counter;
  t_outlet  *x_out_end;
  t_clock   *x_clock_incr;
  t_clock   *x_clock_end;
} t_forpp;

static t_class *forpp_class;

static void forpp_tick_end(t_forpp *x)
{
  outlet_bang(x->x_out_end);
  clock_unset(x->x_clock_end);
}

static void forpp_tick_incr(t_forpp *x)
{
  int stop_it=1;
  
  outlet_float(x->x_out_counter, (t_float)x->x_cur);
  x->x_cur += x->x_incr;
  if(x->x_incr > 0)
  {
    if(x->x_cur <= x->x_end)
      stop_it = 0;
  }
  else
  {
    if(x->x_cur >= x->x_end)
      stop_it = 0;
  }
  
  if(stop_it)
  {
    clock_unset(x->x_clock_incr);
    clock_delay(x->x_clock_end, x->x_delay);
  }
  else
    clock_delay(x->x_clock_incr, x->x_delay);
}

static void forpp_bang(t_forpp *x)
{
  if(x->x_delay > 0.0)
  {
    x->x_cur = x->x_beg;
    forpp_tick_incr(x);
  }
  else
  {
    double cur=x->x_beg, end=x->x_end, incr=x->x_incr;
    
    if(x->x_end < x->x_beg)
    {
      for(; cur >= end; cur += incr)
        outlet_float(x->x_out_counter, (t_float)cur);
    }
    else
    {
      for(; cur <= end; cur += incr)
        outlet_float(x->x_out_counter, (t_float)cur);
    }
    outlet_bang(x->x_out_end);
  }
}

static void forpp_start(t_forpp *x)
{
  forpp_bang(x);
}

static void forpp_stop(t_forpp *x)
{
  x->x_cur = x->x_end + x->x_incr;
  clock_unset(x->x_clock_incr);
  clock_unset(x->x_clock_end);
}

static void forpp_ft3(t_forpp *x, t_floatarg incr)
{
  if(x->x_end < x->x_beg)
  {
    if(incr > 0.0)
      incr = -incr;
  }
  else
  {
    if(incr < 0.0)
      incr = -incr;
  }
  x->x_incr = (double)incr;
}

static void forpp_ft2(t_forpp *x, t_floatarg delay)
{
  if(delay < 0.0)
    delay = 0.0;
  x->x_delay = (double)delay;
}

static void forpp_ft1(t_forpp *x, t_floatarg end)
{
  x->x_end = (double)end;
  forpp_ft3(x, (t_floatarg)x->x_incr);
}

static void forpp_float(t_forpp *x, t_floatarg beg)
{
  x->x_beg = (double)beg;
  forpp_ft3(x, (t_floatarg)x->x_incr);
}

static void forpp_list(t_forpp *x, t_symbol *s, int argc, t_atom *argv)
{
  if((argc >= 4) && IS_A_FLOAT(argv, 3))
    forpp_ft3(x, atom_getfloatarg(3, argc, argv));
  if((argc >= 3) && IS_A_FLOAT(argv, 2))
    forpp_ft2(x, atom_getfloatarg(2, argc, argv));
  if((argc >= 2) && IS_A_FLOAT(argv, 1))
    forpp_ft1(x, atom_getfloatarg(1, argc, argv));
  if((argc >= 1) && IS_A_FLOAT(argv, 0))
    forpp_float(x, atom_getfloatarg(0, argc, argv));
}

static void *forpp_new(t_symbol *s, int argc, t_atom *argv)
{
  t_forpp *x = (t_forpp *)pd_new(forpp_class);
  t_float fbeg=0.0, fend=0.0, fdelay=0.0, fincr=1.0; // default
  
  if((argc >= 1) && IS_A_FLOAT(argv, 0))
    fbeg = (t_float)atom_getfloatarg(0, argc, argv);
  if((argc >= 2) && IS_A_FLOAT(argv, 1))
    fend = (t_float)atom_getfloatarg(1, argc, argv);
  if((argc >= 3) && IS_A_FLOAT(argv, 2))
    fdelay = (t_float)atom_getfloatarg(2, argc, argv);
  if((argc >= 4) && IS_A_FLOAT(argv, 3))
    fincr = (t_float)atom_getfloatarg(3, argc, argv);
  
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft2"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft3"));
  x->x_out_counter = outlet_new(&x->x_obj, &s_float);
  x->x_out_end = outlet_new(&x->x_obj, &s_bang);
  x->x_clock_incr = clock_new(x, (t_method)forpp_tick_incr);
  x->x_clock_end = clock_new(x, (t_method)forpp_tick_end);
  
  x->x_beg = (double)fbeg;
  x->x_end = (double)fend;
  forpp_ft3(x, (t_floatarg)fincr);
  forpp_ft2(x, (t_floatarg)fdelay);
  x->x_cur = x->x_beg;
  return(x);
}

static void forpp_ff(t_forpp *x)
{
  clock_free(x->x_clock_incr);
  clock_free(x->x_clock_end);
}

void forpp_setup(void)
{
  forpp_class = class_new(gensym("for++"), (t_newmethod)forpp_new,
    (t_method)forpp_ff, sizeof(t_forpp),
    0, A_GIMME, 0);
  class_addcreator((t_newmethod)forpp_new, gensym("for_pp"), A_GIMME, 0);
  class_addbang(forpp_class, forpp_bang);
  class_addfloat(forpp_class, forpp_float);
  class_addlist(forpp_class, forpp_list);
  class_addmethod(forpp_class, (t_method)forpp_start, gensym("start"), 0);
  class_addmethod(forpp_class, (t_method)forpp_stop, gensym("stop"), 0);
  class_addmethod(forpp_class, (t_method)forpp_ft1, gensym("ft1"), A_FLOAT, 0);
  class_addmethod(forpp_class, (t_method)forpp_ft2, gensym("ft2"), A_FLOAT, 0);
  class_addmethod(forpp_class, (t_method)forpp_ft3, gensym("ft3"), A_FLOAT, 0);
}

void setup_for0x2b0x2b(void)
{
    forpp_setup();
}
