// tabosci~
// tabosc interpolation
// can replace tabosc4~
// most of this code comes from pd. but with somes modifications

/* 
This software is copyrighted by Miller Puckette and others.  The following
terms (the "Standard Improved BSD License") apply to all files associated with
the software unless explicitly disclaimed in individual files:

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above  
   copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided
   with the distribution.
3. The name of the author may not be used to endorse or promote
   products derived from this software without specific prior 
   written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

// Cyrille Henry 02 2009


#include "m_pd.h"
#include <math.h>

#define max(a,b) ( ((a) > (b)) ? (a) : (b) ) 
#define min(a,b) ( ((a) < (b)) ? (a) : (b) ) 

/******************** tabosci~ ***********************/

/* this is all copied from d_osc.c... what include file could this go in? */
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

union tabfudge
{
    double tf_d;
    int32 tf_i[2];
};

static t_class *tabosci_tilde_class;

typedef struct _tabosci_tilde
{
    t_object x_obj;
    t_float x_fnpoints;
    t_float x_finvnpoints;
    t_word *x_vec;
    t_symbol *x_arrayname;
    t_float x_f;
    double x_phase;
    t_float x_conv;
    t_sample x_prev_in, x_last_in, x_prev_out, x_last_out;
    t_float x_fa1, x_fa2, x_fb1, x_fb2, x_fb3; 
    t_float cutoff;
    t_int upsample;
    t_float x_sr;
} t_tabosci_tilde;

void tabosci_tilde_cutoff(t_tabosci_tilde *x, t_float cut)
{
    x->cutoff = cut;

    if (x->cutoff == 0)
    {
        x->x_fb1 = 1;
        x->x_fb2 = 0;
        x->x_fb3 = 0;
        x->x_fa1 = 0;
        x->x_fa2 = 0;

        x->x_prev_in = 0;
        x->x_last_in = 0;
        x->x_prev_out = 0; // reset filter memory
    }
    else
    {
        // filter coef to cut all high freq.
        t_float tmp1, tmp2;

        tmp1 = sqrt(2)/2;
        tmp1 = sinh(tmp1);

        tmp2 = x->cutoff * 2 * 3.1415926 / (x->upsample * x->x_sr);
	tmp2 = min(6.28,tmp2);

        tmp1 *= sin(tmp2);
        tmp2 = cos(tmp2);

        x->x_fb1 = (1-tmp2 ) /2;
        x->x_fb2 = (1-tmp2 );
        x->x_fb3 = (1-tmp2 ) /2;
        x->x_fa1 = -2 * tmp2;
        x->x_fa2 = 1 - tmp1;
	
        tmp1 +=1;

        x->x_fb1 /= tmp1;
        x->x_fb2 /= tmp1;
        x->x_fb3 /= tmp1;
        x->x_fa1 /= tmp1;
        x->x_fa2 /= tmp1;
    }
}

static void *tabosci_tilde_new(t_symbol *s)
{
    t_tabosci_tilde *x = (t_tabosci_tilde *)pd_new(tabosci_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    x->x_fnpoints = 512.;
    x->x_finvnpoints = (1./512.);
    outlet_new(&x->x_obj, gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_f = 0;
    x->cutoff = 0;
    x->upsample = 1;
    x->x_sr = 0;
    tabosci_tilde_cutoff(x,0); // comput filter coef
    return (x);
}

static t_int *tabosci_tilde_perform(t_int *w)
{
    t_tabosci_tilde *x = (t_tabosci_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    int normhipart;
    union tabfudge tf;
    double a1,a2,a3; // CH : for the interpolation
    t_float fnpoints = x->x_fnpoints;
    int mask = fnpoints - 1;
    t_float conv = fnpoints * x->x_conv;
    int maxindex;
    t_word *tab = x->x_vec, *addr;
    int i;
    double dphase = fnpoints * x->x_phase + UNITBIT32;

    if (!tab) goto zero;
    tf.tf_d = UNITBIT32;
    normhipart = tf.tf_i[HIOFFSET];

    while (n--)
    {
        t_sample frac,  a,  b,  c,  d, cminusb, temp, filter_out; 

    	    for (i=0;i<x->upsample;i++)
    	    {
    	    tf.tf_d = dphase;
    	    dphase += *in/x->upsample * conv;
    	    addr = tab + (tf.tf_i[HIOFFSET] & mask);
    	    a = addr[0].w_float;
    	    addr = tab + ((1+tf.tf_i[HIOFFSET]) & mask);
    	    b = addr[0].w_float;
    	    addr = tab + ((2+tf.tf_i[HIOFFSET]) & mask);
    	    c = addr[0].w_float;
    	    addr = tab + ((3+tf.tf_i[HIOFFSET]) & mask);
    	    d = addr[0].w_float;
    	    tf.tf_i[HIOFFSET] = normhipart;
    	    frac = tf.tf_d - UNITBIT32;

    	    // 4-point, 3rd-order Hermite (x-form)
    	    a1 = 0.5f * (c - a);
    	    a2 = a - 2.5 * b + 2.f * c - 0.5f * d;
    	    a3 = 0.5f * (d - a) + 1.5f * (b - c);

    	    temp =  ((a3 * frac + a2) * frac + a1) * frac + b;

    	    filter_out = x->x_fb1* temp + x->x_fb2 * x->x_last_in + x->x_fb3 * x->x_prev_in - x->x_fa1 * x->x_last_out - x->x_fa2 * x->x_prev_out; 
    	    // low pass 

    	    x->x_prev_in = x->x_last_in;
    	    x->x_last_in = temp;
    	    x->x_prev_out = x->x_last_out;
    	    x->x_last_out = filter_out;
    	    }
        *out++ = x->x_last_out;
        *in++;
    }

    tf.tf_d = UNITBIT32 * fnpoints;
    normhipart = tf.tf_i[HIOFFSET];
    tf.tf_d = dphase + (UNITBIT32 * fnpoints - UNITBIT32);
    tf.tf_i[HIOFFSET] = normhipart;
    x->x_phase = (tf.tf_d - UNITBIT32 * fnpoints)  * x->x_finvnpoints;
    return (w+5);
 zero:
    while (n--) *out++ = 0;

    return (w+5);
}

void tabosci_tilde_set(t_tabosci_tilde *x, t_symbol *s)
{
    t_garray *a;
    int npoints, pointsinarray;

    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
        if (*s->s_name)
            pd_error(x, "tabosci~: %s: no such array", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else if (!garray_getfloatwords(a, &pointsinarray, &x->x_vec))
    {
        pd_error(x, "%s: bad template for tabosci~", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else
    {
        npoints = 1 << ilog2(pointsinarray);
//        if ( npoints != pointsinarray) post("warning, tabosci~ is using only the first %d points of array %s",npoints, x->x_arrayname->s_name);
        x->x_fnpoints = npoints;
        x->x_finvnpoints = 1./npoints;
        garray_usedindsp(a);
    }
}


void tabosci_tilde_upsample(t_tabosci_tilde *x, t_float up)
{
    x->upsample = max(1,up);
    tabosci_tilde_cutoff(x,x->cutoff);
}

static void tabosci_tilde_ft1(t_tabosci_tilde *x, t_float f)
{
    x->x_phase = f;
}

static void tabosci_tilde_dsp(t_tabosci_tilde *x, t_signal **sp)
{
    if (x->x_sr != sp[0]->s_sr)
    {
        x->x_sr = sp[0]->s_sr;
        tabosci_tilde_cutoff(x,x->cutoff);
        x->x_conv = 1. / sp[0]->s_sr;
    }
    tabosci_tilde_set(x, x->x_arrayname);
    dsp_add(tabosci_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void tabosci_tilde_setup(void)
{
    tabosci_tilde_class = class_new(gensym("tabosci~"),
        (t_newmethod)tabosci_tilde_new, 0,
        sizeof(t_tabosci_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabosci_tilde_class, t_tabosci_tilde, x_f);
    class_addmethod(tabosci_tilde_class, (t_method)tabosci_tilde_dsp,
        gensym("dsp"), 0);
    class_addmethod(tabosci_tilde_class, (t_method)tabosci_tilde_set,
        gensym("set"), A_SYMBOL, 0);
    class_addmethod(tabosci_tilde_class, (t_method)tabosci_tilde_cutoff,
        gensym("cutoff"), A_FLOAT, 0);
    class_addmethod(tabosci_tilde_class, (t_method)tabosci_tilde_upsample,
        gensym("upsample"), A_FLOAT, 0);
    class_addmethod(tabosci_tilde_class, (t_method)tabosci_tilde_ft1,
        gensym("ft1"), A_FLOAT, 0);
}

