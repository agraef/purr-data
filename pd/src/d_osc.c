/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* sinusoidal oscillator and table lookup; see also tabosc4~ in d_array.c.
*/

#include "m_pd.h"
#include "math.h"
#include <float.h>  /* for definition of machine epsilon */

#define TWOPI 6.283185307179586
#define COSTABMASK (COSTABSIZE-1)
#define GOODINT(i) (!(i & 0xC0000000))    /* used for integer overflow protection */
#define BIGFLOAT 1.0e+19

/* find machine epsilon (largest relative rounding error) for t_float */
#if PD_FLOATSIZE == 32
#define EPSILON FLT_EPSILON
#elif PD_FLOATSIZE == 64
#define EPSILON DBL_EPSILON
#endif

/*------------------------ global cosine table ---------------------------------*/

t_float *cos_table;    /* global pointer to cosine table */

static void cos_maketable(void)
{
    cos_table = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
    t_float *costab = cos_table;
    t_float angle =  TWOPI / COSTABSIZE;
    int n;

    for(n=0; n<(COSTABSIZE+1); n++) *costab++ = cos(angle * n);
}

/* -------------------------- phasor~ ------------------------------ */
static t_class *phasor_class;

typedef struct
{
    t_object x_obj;
    double x_phase;
    t_float x_oneoversamplerate;
    t_float x_f;                       /* scalar frequency */
} t_phasor;

static void *phasor_new(t_floatarg f)
{
    t_phasor *x = (t_phasor *)pd_new(phasor_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_f = f;
    x->x_phase = 0.;
    x->x_oneoversamplerate = 0.;
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static t_int *phasor_perform(t_int *w)
{
    t_phasor *x = (t_phasor *)(w[1]);
    t_sample *freq = (t_sample *)(w[2]);
    t_sample *phaseout = (t_sample *)(w[3]);
    int vecsize = (int)(w[4]);

    t_float oneoversamplerate = x->x_oneoversamplerate;
    double phase = x->x_phase;
    t_float temp;
    int intphase;
    if(PD_BIGORSMALL(phase)) phase = 0.;
    while (vecsize--)
    {
        /* wrap phase within interval 0. - 1. */
        if(phase>=1.)
        {
            intphase = (int)phase;
            phase = (GOODINT(intphase)? phase - intphase : 0.);
        }
        else if(phase<0.)
        {
            intphase = (int)phase;
            phase = (GOODINT(intphase)? phase - intphase + 1. : 0.);
        }

        temp = *freq++;
        *phaseout++ = phase;
        phase += temp * oneoversamplerate;
    }

    x->x_phase = phase;
    return (w+5);
}

static void phasor_dsp(t_phasor *x, t_signal **sp)
{
    x->x_oneoversamplerate= 1. / sp[0]->s_sr;
    dsp_add(phasor_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void phasor_ft1(t_phasor *x, t_float f)
{
    x->x_phase = (double)f;
}

void phasor_tilde_setup(void)
{
    phasor_class = class_new(gensym("phasor~"),
        (t_newmethod)phasor_new, 0,
        sizeof(t_phasor), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(phasor_class, t_phasor, x_f);
    class_addmethod(phasor_class, (t_method)phasor_dsp, gensym("dsp"),
        A_CANT, 0);
    class_addmethod(phasor_class, (t_method)phasor_ft1,
        gensym("ft1"), A_FLOAT, 0);
}

/* ------------------------ cos~ ----------------------------- */

static t_class *cos_class;

typedef struct
{
    t_object x_obj;
    t_float x_f;            /* scalar phase value */
} t_cos;

static void *cos_new(t_floatarg f)
{
    t_cos *x = (t_cos *)pd_new(cos_class);
    x->x_f = f;
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static t_int *cos_perform(t_int *w)
{
    t_sample *phase = (t_sample *)(w[2]);
    t_sample *cosine = (t_sample *)(w[3]);
    int vecsize = (int)(w[4]);

    t_float tabphase, fraction;
    t_float tabfactor = (t_float)COSTABSIZE;
    t_float *costab = cos_table;
    int index;
    
    while(vecsize--)
    {
        tabphase = *phase++ * tabfactor;
        index = (tabphase >= 0.? (int)tabphase : (int)tabphase - 1);                                                                                                         // round towards -inf
        fraction = (GOODINT(index)? tabphase - index : 0.);
        index &= COSTABMASK;
        *cosine++ = costab[index] + fraction * (costab[index+1] - costab[index]);
    }
    return(w+5);
}

static void cos_dsp(t_cos *x, t_signal **sp)
{
    dsp_add(cos_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void cos_tilde_setup(void)
{
    cos_class = class_new(gensym("cos~"), (t_newmethod)cos_new, 0,
        sizeof(t_cos), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(cos_class, t_cos, x_f);
    class_addmethod(cos_class, (t_method)cos_dsp, gensym("dsp"), A_CANT, 0);
}

/* ------------------------ osc~ ----------------------------- */

static t_class *osc_class;

typedef struct
{
    t_object x_obj;
    double x_tabphase;
    t_float x_oneoversamplerate;
    t_float x_f;                 /* scalar frequency */
} t_osc;

static void *osc_new(t_floatarg f)
{
    t_osc *x = (t_osc *)pd_new(osc_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_f = f;
    x->x_tabphase = 0.;
    x->x_oneoversamplerate = 0.;
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static t_int *osc_perform(t_int *w)
{
    t_osc *x = (t_osc *)(w[1]);
    t_sample *freq = (t_sample *)(w[2]);
    t_sample *cosine = (t_sample *)(w[3]);
    int vecsize = (int)(w[4]);

    double tabphase = x->x_tabphase;
    t_float baseincrement = x->x_oneoversamplerate * (t_float)COSTABSIZE;
    t_float fraction = 0.;
    t_float *costab = cos_table;
    t_float endfreq = freq[vecsize-1];
    int index = 0;
    
    while (vecsize--)
    {
        index = (tabphase >= 0.? (int)tabphase : (int)tabphase - 1);
        fraction = (GOODINT(index)? tabphase - index : 0.);
        tabphase += *freq++ * baseincrement;
        index &= COSTABMASK;
        *cosine++ = costab[index] + fraction * (costab[index+1] - costab[index]);
    }

    x->x_tabphase = fraction + index + (endfreq * baseincrement);     /* wrap phase state */
    return (w+5);
}

static void osc_dsp(t_osc *x, t_signal **sp)
{
    x->x_oneoversamplerate = 1. / sp[0]->s_sr;
    dsp_add(osc_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void osc_ft1(t_osc *x, t_float phase)
{
    x->x_tabphase = phase * COSTABSIZE;
}

void osc_tilde_setup(void)
{
    osc_class = class_new(gensym("osc~"), (t_newmethod)osc_new, 0,
        sizeof(t_osc), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(osc_class, t_osc, x_f);
    class_addmethod(osc_class, (t_method)osc_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(osc_class, (t_method)osc_ft1, gensym("ft1"), A_FLOAT, 0);
}

/* ---------------- vcf~ - 2-pole bandpass filter. ----------------- */

typedef struct
{
    t_float c_real;
    t_float c_im;
    t_float c_q;
    t_float c_oneoversamplerate;
} t_vcfctl;

typedef struct
{
    t_object x_obj;
    t_vcfctl x_cspace;
    t_vcfctl *x_ctl;
    t_float x_f;
} t_vcf;

t_class *vcf_class;

static void *vcf_new(t_floatarg q)
{
    t_vcf *x = (t_vcf *)pd_new(vcf_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_ctl = &x->x_cspace;
    x->x_cspace.c_real = 0;
    x->x_cspace.c_im = 0;
    x->x_cspace.c_q = q;
    x->x_cspace.c_oneoversamplerate = 0;
    x->x_f = 0;
    return (x);
}

static void vcf_ft1(t_vcf *x, t_floatarg q)
{
    if(q < 0.) q = 0.;
    if(q > BIGFLOAT) q = BIGFLOAT;
    x->x_ctl->c_q = q;
}

static t_int *vcf_perform(t_int *w)
{
    t_sample *inputsample = (t_sample *)(w[1]);
    t_sample *freqin = (t_sample *)(w[2]);
    t_sample *out1 = (t_sample *)(w[3]);
    t_sample *out2 = (t_sample *)(w[4]);
    t_vcfctl *c = (t_vcfctl *)(w[5]);
    int vectorsize = (t_int)(w[6]);

    t_float real = c->c_real, real2;
    t_float im = c->c_im;
    t_float q = c->c_q;
    t_float *costab = cos_table;

    t_float ampcorrect = 2. - 2. / (q + 2.);
    t_float q2radius = (q ? c->c_oneoversamplerate * TWOPI / q : 0.);
    t_float tabnorm =  c->c_oneoversamplerate * COSTABSIZE;
    t_float realcoef, imcoef, fraction, insamp;
    t_float tabphase, centerfreq, radius, oneminusr;
    int index;

    while(vectorsize--)
    {
        centerfreq = (*freqin > 0.? *freqin : 0.);
        freqin++;

        /* radius */
        radius = 1. - centerfreq * q2radius - EPSILON;
        if ((radius < 0.) || (q2radius == 0.)) radius = 0.;
        oneminusr = 1.0 - radius;

        /* phase */
        tabphase = centerfreq * tabnorm;
        index = (int)tabphase;
        fraction = (GOODINT(index)? tabphase - index : 0.);
        index &= COSTABMASK;

        /* coefficients */
        realcoef = radius * (costab[index] + fraction * (costab[index+1] - costab[index]));
        index -= COSTABSIZE>>2;
        index &= COSTABSIZE-1;
        imcoef = radius * (costab[index] + fraction * (costab[index+1] - costab[index]));

        insamp = *inputsample++;
        real2 = real;
        *out1++ = real = ampcorrect * oneminusr * insamp + realcoef * real2 - imcoef * im;
        *out2++ = im = imcoef * real2 + realcoef * im;
    }

    if (PD_BIGORSMALL(real)) real = 0.;
    if (PD_BIGORSMALL(im)) im = 0.;
    c->c_real = real;
    c->c_im = im;
    return (w+7);
}

static void vcf_dsp(t_vcf *x, t_signal **sp)
{
    x->x_ctl->c_oneoversamplerate = 1. / sp[0]->s_sr;
    dsp_add(vcf_perform, 6, sp[0]->s_vec, sp[1]->s_vec,
        sp[2]->s_vec, sp[3]->s_vec, x->x_ctl, sp[0]->s_n);
}

void vcf_tilde_setup(void)
{
    vcf_class = class_new(gensym("vcf~"), (t_newmethod)vcf_new, 0,
        sizeof(t_vcf), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(vcf_class, t_vcf, x_f);
    class_addmethod(vcf_class, (t_method)vcf_dsp, gensym("dsp"),
        A_CANT, 0);
    class_addmethod(vcf_class, (t_method)vcf_ft1,
        gensym("ft1"), A_FLOAT, 0);
}

/* -------------------------- noise~ ------------------------------ */
static t_class *noise_class;

typedef struct _noise
{
    t_object x_obj;
    int x_val;
} t_noise;

static void *noise_new(void)
{
    t_noise *x = (t_noise *)pd_new(noise_class);
    static int init = 307;
    x->x_val = (init *= 1319); 
    outlet_new(&x->x_obj, gensym("signal"));
    return (x);
}

static t_int *noise_perform(t_int *w)
{
    t_sample *out = (t_sample *)(w[1]);
    int *vp = (int *)(w[2]);
    int n = (int)(w[3]);
    int val = *vp;
    while (n--)
    {
        *out++ = ((t_float)((val & 0x7fffffff) - 0x40000000)) *
            (t_float)(1.0 / 0x40000000);
        val = val * 435898247 + 382842987;
    }
    *vp = val;
    return (w+4);
}

static void noise_dsp(t_noise *x, t_signal **sp)
{
    dsp_add(noise_perform, 3, sp[0]->s_vec, &x->x_val, sp[0]->s_n);
}

static void noise_tilde_setup(void)
{
    noise_class = class_new(gensym("noise~"), (t_newmethod)noise_new, 0,
        sizeof(t_noise), 0, 0);
    class_addmethod(noise_class, (t_method)noise_dsp, gensym("dsp"), A_CANT, 0);
}

/* ----------------------- global setup routine ---------------- */
void d_osc_setup(void)
{
    cos_maketable();
    phasor_tilde_setup();
    cos_tilde_setup();
    osc_tilde_setup();
    vcf_tilde_setup();
    noise_tilde_setup();
}