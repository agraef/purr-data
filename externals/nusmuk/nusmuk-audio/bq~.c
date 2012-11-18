/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// modification of this code by cyrille henry in order to change the biquad topology and add audio inlet for filter coefs

#include "m_pd.h"
#include <math.h>

/* ---------------- bq~ - raw bq filter ----------------- */

typedef struct bqctl
{
    t_sample c_x1;
    t_sample c_x2;
    t_sample c_y1;
    t_sample c_y2;
} t_bqctl;

typedef struct bq_tilde
{
    t_object x_obj;
    t_float x_f;
    t_bqctl x_cspace;
    t_bqctl *x_ctl;
} t_bq_tilde;

t_class *bq_tilde_class;

static void *bq_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
    t_bq_tilde *x = (t_bq_tilde *)pd_new(bq_tilde_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    x->x_ctl = &x->x_cspace;
    x->x_cspace.c_x1 = x->x_cspace.c_x2 = 0;
    x->x_cspace.c_y1 = x->x_cspace.c_y2 = 0;
    x->x_f = 0;
    return (x);
}

static t_int *bq_tilde_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *ina1 = (t_sample *)(w[2]);
    t_sample *ina2 = (t_sample *)(w[3]);
    t_sample *inb1 = (t_sample *)(w[4]);
    t_sample *inb2 = (t_sample *)(w[5]);
    t_sample *inb3 = (t_sample *)(w[6]);
    t_sample *out = (t_sample *)(w[7]);
    t_bqctl *c = (t_bqctl *)(w[8]);
    int n = (t_int)(w[9]);
    int i;
    t_sample last_in = c->c_x1;
    t_sample prev_in = c->c_x2;
    t_sample last_out = c->c_y1;
    t_sample prev_out = c->c_y2;

    for (i = 0; i < n; i++)
    {
        t_sample output =  *inb1++ * *in + *inb2++ * last_in + *inb3++ * prev_in - *ina1++ * last_out - *ina2++ * prev_out;
//        if (PD_BIGORSMALL(output))
//            output = 0; i don't understnd why it did not compile with this 2 lines. 
// should be fixed latter if denormal is a problem
        *out++ = output; 
        prev_in = last_in;
        prev_out = last_out;
        last_out = output;
        last_in = *in++;
    }
    c->c_x1 = last_in;
    c->c_x2 = prev_in;
    c->c_y1 = last_out;
    c->c_y2 = prev_out;

    return (w+10);
}

static void bq_tilde_set(t_bq_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    t_bqctl *c = x->x_ctl;
    c->c_x1 = atom_getfloatarg(0, argc, argv);
    c->c_x2 = atom_getfloatarg(1, argc, argv);
    c->c_y1 = atom_getfloatarg(2, argc, argv);
    c->c_y2 = atom_getfloatarg(3, argc, argv);
}

static void bq_tilde_dsp(t_bq_tilde *x, t_signal **sp)
{
    dsp_add(bq_tilde_perform, 9,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, 
			sp[5]->s_vec, sp[6]->s_vec, x->x_ctl, sp[0]->s_n);

}

void bq_tilde_setup(void)
{
    bq_tilde_class = class_new(gensym("bq~"), (t_newmethod)bq_tilde_new,
        0, sizeof(t_bq_tilde), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(bq_tilde_class, t_bq_tilde, x_f);
    class_addmethod(bq_tilde_class, (t_method)bq_tilde_dsp, gensym("dsp"), 0);
    class_addmethod(bq_tilde_class, (t_method)bq_tilde_set, gensym("set"),
        A_GIMME, 0);
    class_addmethod(bq_tilde_class, (t_method)bq_tilde_set, gensym("clear"),
        A_GIMME, 0);
}


