/* Copyright (c) 2002 Damien HENRY.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* recv2 */

#include "m_pd.h"

static t_class *recv2_class;

typedef struct _recv2
{
    t_object x_obj;
    t_symbol *x_sym;
} t_recv2;

static void recv2_bang(t_recv2 *x)
{
    outlet_bang(x->x_obj.ob_outlet);
}

static void recv2_float(t_recv2 *x, t_float f)
{
    outlet_float(x->x_obj.ob_outlet, f);
}

static void recv2_symbol(t_recv2 *x, t_symbol *s)
{
    outlet_symbol(x->x_obj.ob_outlet, s);
}

static void recv2_pointer(t_recv2 *x, t_gpointer *gp)
{
    outlet_pointer(x->x_obj.ob_outlet, gp);
}

static void recv2_list(t_recv2 *x, t_symbol *s, int argc, t_atom *argv)
{
    outlet_list(x->x_obj.ob_outlet, s, argc, argv);
}

static void recv2_anything(t_recv2 *x, t_symbol *s, int argc, t_atom *argv)
{
    outlet_anything(x->x_obj.ob_outlet, s, argc, argv);
}

static void recv2_set(t_recv2 *x, t_symbol *s)
{
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
    x->x_sym = s;
    pd_bind(&x->x_obj.ob_pd, s);  
}

static void *recv2_new(t_symbol *s)
{
    t_recv2 *x = (t_recv2 *)pd_new(recv2_class);
    x->x_sym = s;
    pd_bind(&x->x_obj.ob_pd, s);  
    outlet_new(&x->x_obj, 0);
    return (x);
}

static void recv2_free(t_recv2 *x)
{
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
}

void recv2_setup(void)
{
    recv2_class = class_new(gensym("recv2"), (t_newmethod)recv2_new, 
    	(t_method)recv2_free, sizeof(t_recv2), 0, A_DEFSYM, 0);
    class_addbang(recv2_class, recv2_bang);
    class_addfloat(recv2_class, (t_method)recv2_float);
    class_addsymbol(recv2_class, recv2_symbol);
    class_addpointer(recv2_class, recv2_pointer);
    class_addlist(recv2_class, recv2_list);
    class_addanything(recv2_class, recv2_anything);
    class_addmethod(recv2_class, (t_method)recv2_set,  gensym("@"),A_DEFSYMBOL, 0);
    class_sethelpsymbol(recv2_class, gensym("xgui/help_recv2"));
}
