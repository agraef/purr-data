/* Copyright (c) 1997-2001 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "e_sqrt.h"

static float rsqrt_exptab[DUMTAB1SIZE], rsqrt_mantissatab[DUMTAB2SIZE];

typedef struct sigsqrt
{
    t_object x_obj;
    t_float x_f;
} t_sigsqrt;

static t_class *sigsqrt_class;

static void *sigsqrt_new(void)
{
    t_sigsqrt *x = (t_sigsqrt *)pd_new(sigsqrt_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

t_int *sigsqrt_perform(t_int *w)    /* not static; also used in d_fft.c */
{
    t_sample *in = *(t_sample **)(w+1), *out = *(t_sample **)(w+2);
    t_int n = *(t_int *)(w+3);
    while (n--)
    {   
        t_sample f = *in;
        long l = *(long *)(in++);
        if (f < 0) *out++ = 0;
        else
        {
            t_sample g = rsqrt_exptab[(l >> 23) & 0xff] *
                rsqrt_mantissatab[(l >> 13) & 0x3ff];
            *out++ = f * (1.5 * g - 0.5 * g * g * g * f);
        }
    }
    return (w + 4);
}

static void sigsqrt_dsp(t_sigsqrt *x, t_signal **sp)
{
    dsp_add(sigsqrt_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void sqrt_tilde_setup(void)
{
    sigsqrt_class = class_new(gensym("sqrt~"), (t_newmethod)sigsqrt_new, 0,
        sizeof(t_sigsqrt), 0, 0);
    class_addcreator(sigsqrt_new, gensym("q8_sqrt~"), 0);   /* old name */
    CLASS_MAINSIGNALIN(sigsqrt_class, t_sigsqrt, x_f);
    class_addmethod(sigsqrt_class, (t_method)sigsqrt_dsp, gensym("dsp"), 0);
}
