/* rc~.c by Martin Peach 20100315 */
/* Pd external emulating a resistor-capacitor low-pass filter */
/* The control parameter is a time constant in seconds (or resistance X capacitance) */

#include "m_pd.h"

static t_class *rc_tilde_class;

typedef struct _rc_tilde
{
    t_object    rc_obj;
    t_float     rc_rc;
    t_float     rc_f;
    t_sample    rc_node;
} t_rc_tilde;

static t_int *rc_tilde_perform(t_int *w);
static void rc_tilde_dsp(t_rc_tilde *x, t_signal **sp);
static void *rc_tilde_new(t_floatarg f);

static t_int *rc_tilde_perform(t_int *w)
{
    t_rc_tilde      *x = (t_rc_tilde *)(w[1]);
    t_sample        *in = (t_sample *)(w[2]);
    t_sample        *out = (t_sample *)(w[3]);
    int             n = (int)(w[4]);
    float           slewrate;
    float           sp = 1.0/sys_getsr();

    if (x->rc_rc < sp) slewrate = 1.0;    
    else slewrate = sp/x->rc_rc;
        
    while (n--)
    {
        x->rc_node += (*in++ - x->rc_node)*slewrate;
        *out++ = x->rc_node;
    }

    return (w+5);
}

static void rc_tilde_dsp(t_rc_tilde *x, t_signal **sp)
{
    dsp_add(rc_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void *rc_tilde_new(t_floatarg f)
{
    t_rc_tilde *x = (t_rc_tilde *)pd_new(rc_tilde_class);

    x->rc_rc = f;
    x->rc_node = 0.0;
  
    floatinlet_new (&x->rc_obj, &x->rc_rc);
    outlet_new(&x->rc_obj, &s_signal);
    return (void *)x;
}

void rc_tilde_setup(void)
{
    rc_tilde_class = class_new(gensym("rc~"),
        (t_newmethod)rc_tilde_new,
        0, sizeof(t_rc_tilde),
        CLASS_DEFAULT, 
        A_DEFFLOAT, 0);

    class_addmethod(rc_tilde_class, (t_method)rc_tilde_dsp, gensym("dsp"), 0);
    CLASS_MAINSIGNALIN(rc_tilde_class, t_rc_tilde, rc_f);
}

/* fin rc~.c */

