/* rcosc~.c by Martin Peach 20100331 */
/* Pd external emulating a resistor-capacitor-controlled oscillator */
/* The first control parameter is a time constant in seconds (or resistance X capacitance) */
/* The second control parameter is a threshold above which the cap is set to discharge */
/* The third control parameter is a threshold below which the cap is set to charge */

#include "m_pd.h"

static t_class *rcosc_tilde_class;

typedef struct _rcosc_tilde
{
    t_object    rc_obj;
    t_float     rc_f;
    t_float     rc_upper_threshold; /* if rc_dir is 1, charge to here */
    t_float     rc_lower_threshold; /* if rc_dir is 0, discharge to here */
    t_int       rc_dir;
    double      rc_node;
    double      rc_one;
    double      rc_zero;
    double      rc_sp;
    double      rc_slewmax;
} t_rcosc_tilde;

static t_int *rcosc_tilde_perform(t_int *w);
static void rcosc_tilde_dsp(t_rcosc_tilde *x, t_signal **sp);
static void *rcosc_tilde_new(t_floatarg f);

static t_int *rcosc_tilde_perform(t_int *w)
{
    t_rcosc_tilde   *x = (t_rcosc_tilde *)(w[1]);
    t_sample        *in = (t_sample *)(w[2]);
    t_sample        *out = (t_sample *)(w[3]);
    int             n = (int)(w[4]);
    double          slewrate, delta, node, overshoot;
    int             i, n_oversamples = 64;

//    if (x->rc_rc < x->rc_sp) slewrate = 1.0;    
//    else slewrate = x->rc_sp/x->rc_rc;
        
    while (n--)
    {
        slewrate =  x->rc_one/(n_oversamples*(x->rc_sp*(*in++)));
        if (slewrate < x->rc_zero) slewrate = x->rc_zero;
        else if (slewrate > x->rc_one) slewrate = x->rc_one;
        node = x->rc_node;
        for (i = 0; i < n_oversamples; ++i)
        {
            if (x->rc_dir)
            {
                delta = slewrate*(x->rc_one - x->rc_node);
                if (delta > x->rc_slewmax) delta = x->rc_slewmax;
                x->rc_node += delta;
                overshoot = x->rc_node - x->rc_upper_threshold;
                if (overshoot > 0)
                {
                    x->rc_dir = 0;
                    x->rc_node = x->rc_upper_threshold - overshoot; /* bounce */
                }
            }
            else
            {
                delta = slewrate*(x->rc_zero - x->rc_node);
                if (delta < -x->rc_slewmax) delta = -x->rc_slewmax;
                x->rc_node += delta;
                overshoot = x->rc_lower_threshold - x->rc_node;
                if (overshoot > 0)
                {
                    x->rc_dir = 1;
                    x->rc_node = x->rc_lower_threshold + overshoot; /* bounce */
                }
            }
        }
        *out++ = ((node + x->rc_node)*3)-3;
    }
    return (w+5);
}

static void rcosc_tilde_dsp(t_rcosc_tilde *x, t_signal **sp)
{
    x->rc_sp = sys_getsr();
    dsp_add(rcosc_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void *rcosc_tilde_new(t_floatarg f)
{
    t_rcosc_tilde *x = (t_rcosc_tilde *)pd_new(rcosc_tilde_class);

    x->rc_f = f;
    x->rc_node = 0.0;
    x->rc_upper_threshold = 0.666; /* if rc_dir is 1, charge to here */
    x->rc_lower_threshold = 0.333; /* if rc_dir is 0, discharge to here */
    x->rc_slewmax = 0.003;/* arbitrary slew rate limit */
    x->rc_dir = 1; /* start charging */
    x->rc_one = 1;
    x->rc_zero = 0;
  
    outlet_new(&x->rc_obj, &s_signal);

    post("rcosc~ 20100331 Martin Peach");
    return (void *)x;
}

void rcosc_tilde_setup(void)
{
    rcosc_tilde_class = class_new(gensym("rcosc~"),
        (t_newmethod)rcosc_tilde_new,
        0, sizeof(t_rcosc_tilde),
        CLASS_DEFAULT, 
        A_DEFFLOAT, 0);

    class_addmethod(rcosc_tilde_class, (t_method)rcosc_tilde_dsp, gensym("dsp"), 0);
    CLASS_MAINSIGNALIN(rcosc_tilde_class, t_rcosc_tilde, rc_f);
}

/* fin rcosc~.c */

