/* f2b.c MP 20130313 */
/* Convert a Pd float to a list of 4 bytes */
#include "m_pd.h"
#include <string.h>

typedef struct _f2b
{
    t_object        x_obj;
    t_outlet        *x_out;
} t_f2b;

static t_class *f2b_class;

void f2b_setup(void);
static void *f2b_new(t_symbol *s, int argc, t_atom *argv);
static void f2b_free(t_f2b *x);
static void f2b_float(t_f2b *x, t_float f);

union fbuf
{
    t_float f;
    unsigned char b[4];
};

static void f2b_float(t_f2b *x, t_float f)
{
    int i;
    union fbuf  buf;
    t_atom outs[4];


    //post("f2b_float: f is %f", f);
    buf.f = f;
    for (i = 0; i < 4; ++i) SETFLOAT(&outs[i], buf.b[i]);

    outlet_list(x->x_out, gensym("list"), 4, outs);
}

static void f2b_free(t_f2b *x)
{
    return;
}

static void *f2b_new(t_symbol *s, int argc, t_atom *argv)
{
    t_f2b           *x;

    x = (t_f2b *)pd_new(f2b_class);
    if (x == NULL) return (x);
    x->x_out = outlet_new((t_object *)x, &s_float);
    return (x);
}

void f2b_setup(void)
{
    f2b_class = class_new(gensym("f2b"),
                    (t_newmethod)f2b_new,
                    (t_method)f2b_free,
                    sizeof(t_f2b), 0, 0); /* no arguments */
    class_addfloat(f2b_class, f2b_float);
}
/* end f2b.c */

