#include "defines.h"

/*--------------- bp ---------------*/

typedef struct bpctl
{
    float c_x1;
    float c_x2;
    float c_coef1;
    float c_coef2;
    float c_gain;
} t_bpctl;

typedef struct bp
{
    t_object x_obj;
    float x_freq;
    float x_q;
    t_bpctl x_cspace;
    t_bpctl *x_ctl;
    float x_f;
} t_bp;

t_class *bp_class;

static void bp_docoef(t_bp *x, t_floatarg f, t_floatarg q);

static void *bp_new(t_floatarg f, t_floatarg q)
{
    t_bp *x = (t_bp *)pd_new(bp_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft2"));
    outlet_new(&x->x_obj, gensym("float"));
    x->x_ctl = &x->x_cspace;
    x->x_cspace.c_x1 = 0;
    x->x_cspace.c_x2 = 0;
    bp_docoef(x, f, q);
    x->x_f = 0;
    return (x);
}

static float bp_qcos(float f)
{
    if (f >= -(0.5f*3.14159f) && f <= 0.5f*3.14159f)
    {
    	float g = f*f;
    	return (((g*g*g * (-1.0f/720.0f) + g*g*(1.0f/24.0f)) - g*0.5f) + 1);
    }
    else return (0);
}

static void bp_docoef(t_bp *x, t_floatarg f, t_floatarg q)
{
    float r, oneminusr, omega;
    if (f < 0.0001f) f = 0.0001f;
    if (q < 0) q = 0;
    x->x_freq = f;
    x->x_q = q;
    omega = f * (2.0f * 3.14159f);
    if (q < 0.001) oneminusr = 1.0f;
    else oneminusr = omega/q;
    if (oneminusr > 1.0f) oneminusr = 1.0f;
    r = 1.0f - oneminusr;
    x->x_ctl->c_coef1 = 2.0f * bp_qcos(omega) * r;
    x->x_ctl->c_coef2 = - r * r;
    x->x_ctl->c_gain = 2 * oneminusr * (oneminusr + r * omega);
    /* post("r %f, omega %f, coef1 %f, coef2 %f",
    	r, omega, x->x_ctl->c_coef1, x->x_ctl->c_coef2); */
}

static void bp_ft1(t_bp *x, t_floatarg f)
{
    bp_docoef(x, f, x->x_q);
}

static void bp_ft2(t_bp *x, t_floatarg q)
{
    bp_docoef(x, x->x_freq, q);
}

static void bp_clear(t_bp *x, t_floatarg q)
{
    x->x_ctl->c_x1 = x->x_ctl->c_x2 = 0;
}

static void bp_perform(t_bp *x, t_float in)
{
	float out;
    t_bpctl *c = x->x_ctl;
    float last = c->c_x1;
    float prev = c->c_x2;
    float coef1 = c->c_coef1;
    float coef2 = c->c_coef2;
    float gain = c->c_gain;

	float output =  in + coef1 * last + coef2 * prev;
   	out = gain * output;

	prev = last;
	last = output;

	/* NAN protect */
    if (!((last <= 0) || (last >= 0)))
    	last = 0;
    if (!((prev <= 0) || (prev >= 0)))
    	prev = 0;
    c->c_x1 = last;
    c->c_x2 = prev;

    outlet_float(x->x_obj.ob_outlet, out);
}

void bp_setup(void)
{
    bp_class = class_new(gensym("bp"), (t_newmethod)bp_new, 0,
	sizeof(t_bp), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(bp_class, (t_method)bp_perform);
    class_addmethod(bp_class, (t_method)bp_ft1,
    	gensym("ft1"), A_FLOAT, 0);
    class_addmethod(bp_class, (t_method)bp_ft2,
    	gensym("ft2"), A_FLOAT, 0);
    class_addmethod(bp_class, (t_method)bp_clear, gensym("clear"), 0);
}
