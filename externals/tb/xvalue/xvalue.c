/* extended (settable) value object
   Copyright (c) 2004 Tim Blechmann
   a huge part of the code is broken out of the code of the value object.

 * Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"


/* -------------------- xvalue ----------------------------- */

static t_class *xvalue_class;

static t_class *vcommon_class; 

typedef struct vcommon
{
    t_pd c_pd;
    int c_refcount;
    t_float c_f;
} t_vcommon;


typedef struct _xvalue
{
    t_object x_obj;
    t_symbol *x_sym;
    t_float *x_floatstar;
} t_xvalue;

static void *xvalue_new(t_symbol *s)
{
    t_xvalue *x = (t_xvalue *)pd_new(xvalue_class);
    x->x_sym = s;
    x->x_floatstar = value_get(s);
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

static void xvalue_bang(t_xvalue *x)
{
    outlet_float(x->x_obj.ob_outlet, *x->x_floatstar);
}

static void xvalue_float(t_xvalue *x, t_float f)
{
    *x->x_floatstar = f;
}

static void xvalue_ff(t_xvalue *x)
{
    value_release(x->x_sym);
}

static void xvalue_set(t_xvalue *x, t_symbol *s)
{
    x->x_sym = s;
    x->x_floatstar = value_get(s);
}

void xvalue_setup(void)
{
    xvalue_class = class_new(gensym("xvalue"), (t_newmethod)xvalue_new,
    	(t_method)xvalue_ff,
    	sizeof(t_xvalue), 0, A_DEFSYM, 0);
    class_addcreator((t_newmethod)xvalue_new, gensym("xv"), A_DEFSYM, 0);
    class_addbang(xvalue_class, xvalue_bang);
    class_addfloat(xvalue_class, xvalue_float);
    class_addmethod(xvalue_class, (t_method) xvalue_set, gensym("set"), 
		    A_SYMBOL, 0);
    vcommon_class = class_new(gensym("xvalue"), 0, 0,
			      sizeof(t_vcommon), CLASS_PD, 0);
}


