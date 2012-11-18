/* Copyright (c) 2005 Federico Ferri.
 * Release under the terms of GPL license.
 * Based on PureData by Miller Puckette and others. */

#include <stdio.h>
#include <stdlib.h>
#include "m_pd.h"

#define buf_sz MAXPDSTRING

typedef struct
{
    t_object       x_obj;
    t_outlet      *x_out2;
} t_ATOX;

static t_class *ATOX_class;

static void ATOX_out(t_ATOX *x, t_float f)
{
    outlet_float(x->x_obj.ob_outlet, f);
}

static void ATOX_float(t_ATOX *x, t_float f)
{
    ATOX_out(x, f);
}

static void ATOX_symbol(t_ATOX *x, t_symbol *s)
{
    char buf[buf_sz];
    t_atom a;
    SETSYMBOL(&a, s);
    atom_string(&a, buf, buf_sz);
    ATOX_out(x, ATOX(buf));
}

static void *ATOX_new(t_floatarg f)
{
    t_ATOX *x = (t_ATOX *)pd_new(ATOX_class);
    outlet_new((t_object *)x, &s_float);
    return (x);
}

void ATOX_setup(void)
{
    ATOX_class = class_new(gensym("ATOX"),
			      (t_newmethod)ATOX_new, 0,
			      sizeof(t_ATOX), CLASS_DEFAULT,
			      A_DEFSYMBOL, 0);
    class_addfloat(ATOX_class, ATOX_float);
    class_addsymbol(ATOX_class, ATOX_symbol);
}
