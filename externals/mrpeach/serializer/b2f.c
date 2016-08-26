/* b2f.c MP 20130313 */
/* Convert a list of 4 bytes to a Pd float */
#include "m_pd.h"
#include <string.h>

typedef struct _b2f
{
    t_object        x_obj;
    t_outlet        *x_out;
} t_b2f;

static t_class *b2f_class;

void b2f_setup(void);
static void *b2f_new(t_symbol *s, int argc, t_atom *argv);
static void b2f_free(t_b2f *x);
static void b2f_list(t_b2f *x, t_symbol *s, int argc, t_atom *argv);

union fbuf
{
    t_float f;
    unsigned char b[4];
};

static void b2f_list(t_b2f *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, d;
    union fbuf  buf;

    //post("b2f_list: s is %s, argc is %d", s->s_name, argc);
    if (0 != strncmp("list", s->s_name, 4))
    {
        post("b2f_list: not a list of floats");
        return;
    }
    if (argc != 4)
    {
        post("b2f_list: need 4 floats");
        return;
    }
    for (i = 0; i < 4; ++i)
    {
        if (argv[i].a_type != A_FLOAT)
        {
            post("b2f_list: list element %d is not a float", i);
            return;
        }
        d = argv[i].a_w.w_float;
        if (d != argv[i].a_w.w_float)
        {
            post("b2f_list: list element %d is not an integer", i);
            return;
        }
        if (d < 0 || d > 255)
        {
            post("b2f_list: list element %d is not an integer on [0..255]", i);
            return;
        }
        buf.b[i] = d;
    }
    outlet_float(x->x_out, buf.f);
}

static void b2f_free(t_b2f *x)
{
    return;
}

static void *b2f_new(t_symbol *s, int argc, t_atom *argv)
{
    t_b2f           *x;

    x = (t_b2f *)pd_new(b2f_class);
    if (x == NULL) return (x);
    x->x_out = outlet_new((t_object *)x, &s_float);
    return (x);
}

void b2f_setup(void)
{
    b2f_class = class_new(gensym("b2f"),
                    (t_newmethod)b2f_new,
                    (t_method)b2f_free,
                    sizeof(t_b2f), 0, 0); /* no arguments */
    class_addlist(b2f_class, b2f_list);
}
/* end b2f.c */

