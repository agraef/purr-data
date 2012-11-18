/* Original phasor~ code Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* disis_phasor~ by Copyright (c) 2009  Ivica Ico Bukvic <ico@vt.edu>
* DISIS http://disis.music.vt.edu
* released under the same BSD license as the above */

/* v.1.0.1 fixes destructor which was not properly called before */
/* v.1.0.0 first release

/* sinusoidal oscillator and table lookup; see also tabosc4~ in d_array.c.
*/

#include "m_pd.h"
#include "math.h"

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

    /* machine-dependent definitions.  These ifdefs really
    should have been by CPU type and not by operating system! */
#ifdef IRIX
    /* big-endian.  Most significant byte is at low address in memory */
#define HIOFFSET 0    /* word offset to find MSB */
#define LOWOFFSET 1    /* word offset to find LSB */
#define int32 long  /* a data type that has 32 bits */
#endif /* IRIX */

#ifdef MSW
    /* little-endian; most significant byte is at highest address */
#define HIOFFSET 1
#define LOWOFFSET 0
#define int32 long
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <machine/endian.h>
#endif

#ifdef __linux__
#include <endian.h>
#endif

#if defined(__unix__) || defined(__APPLE__)
#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)                         
#error No byte order defined                                                    
#endif                                                                          

#if BYTE_ORDER == LITTLE_ENDIAN                                             
#define HIOFFSET 1                                                              
#define LOWOFFSET 0                                                             
#else                                                                           
#define HIOFFSET 0    /* word offset to find MSB */                             
#define LOWOFFSET 1    /* word offset to find LSB */                            
#endif /* __BYTE_ORDER */                                                       
#include <sys/types.h>
#define int32 int32_t
#endif /* __unix__ or __APPLE__*/

union disis_phasor_tabfudge
{
    double tf_d;
    int32 tf_i[2];
};

/* -------------------------- disis_phasor~ ------------------------------ */
static t_class *disis_phasor_class; //, *scalardisis_phasor_class;

typedef struct _disis_phasor
{
    t_object x_obj;
    double x_phase;
	double x_phase_delta;
    float x_conv;
    float x_f;      /* scalar frequency */
	t_clock *x_clock;
	t_outlet *b_out;
} t_disis_phasor;

void phasor_tick(t_disis_phasor *x)
{
    outlet_bang(x->b_out);
}

t_int *disis_phasor_perform(t_int *w)
{
    t_disis_phasor *x = (t_disis_phasor *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    int n = (int)(w[4]);
    double dphase = x->x_phase + UNITBIT32;
    union disis_phasor_tabfudge tf;
    int normhipart;
    float conv = x->x_conv;
    tf.tf_d = UNITBIT32;
    normhipart = tf.tf_i[HIOFFSET];
    tf.tf_d = dphase;

    while (n--)
    {
        tf.tf_i[HIOFFSET] = normhipart;
        dphase += *in++ * conv;
        *out++ = tf.tf_d - UNITBIT32;
        tf.tf_d = dphase;
    }
    tf.tf_i[HIOFFSET] = normhipart;
    x->x_phase = tf.tf_d - UNITBIT32;
	if (x->x_phase_delta > 0.5 && x->x_phase < 0.5) {
		//this is unsafe to call from a signal thread
		//so we use clock_delay instead
		//outlet_bang(x->b_out);
		clock_delay(x->x_clock, 0);
	}
	x->x_phase_delta = x->x_phase;
    return (w+5);
}

static void disis_phasor_dsp(t_disis_phasor *x, t_signal **sp)
{
    x->x_conv = 1./sp[0]->s_sr;
    dsp_add(disis_phasor_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void disis_phasor_free(t_disis_phasor *x)
{
	clock_free(x->x_clock);
}

void *disis_phasor_new(t_floatarg f)
{
	post("DISIS implementation of PD's vanilla phasor~ v.1.0.1");
    t_disis_phasor *x = (t_disis_phasor *)pd_new(disis_phasor_class);
    x->x_f = f;
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_phase = 0;
	x->x_phase_delta = 1;
    x->x_conv = 0;
    outlet_new(&x->x_obj, gensym("signal"));
	x->x_clock = clock_new(x, (t_method)phasor_tick);
	x->b_out = outlet_new(&x->x_obj, &s_bang);
    return (x);
}

void disis_phasor_ft1(t_disis_phasor *x, t_float f)
{
    x->x_phase = f;
	x->x_phase_delta = x->x_phase;
	outlet_bang(x->b_out);
}

void disis_phasor_tilde_setup(void)
{
    disis_phasor_class = class_new(gensym("disis_phasor~"), (t_newmethod)disis_phasor_new, (t_method) disis_phasor_free,
        sizeof(t_disis_phasor), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(disis_phasor_class, t_disis_phasor, x_f);
    class_addmethod(disis_phasor_class, (t_method)disis_phasor_dsp, gensym("dsp"), 0);
    class_addmethod(disis_phasor_class, (t_method)disis_phasor_ft1,
        gensym("ft1"), A_FLOAT, 0);
}

