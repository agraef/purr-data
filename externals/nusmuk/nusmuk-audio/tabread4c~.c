// tabread4c~
// can replace tabread4c~
// most of this code comes from pd. just the interpolation shematic is diferent.


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

// Cyrille Henry 06 2008

#include "m_pd.h"

/******************** tabread4c~ ***********************/

static t_class *tabread4c_tilde_class;

typedef struct _tabread4c_tilde
{
    t_object x_obj;
    int x_npoints;
    t_word *x_vec;
    t_symbol *x_arrayname;
    t_float x_f;
} t_tabread4c_tilde;

static void *tabread4c_tilde_new(t_symbol *s)
{
    t_tabread4c_tilde *x = (t_tabread4c_tilde *)pd_new(tabread4c_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *tabread4c_tilde_perform(t_int *w)
{
    t_tabread4c_tilde *x = (t_tabread4c_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);    
    int maxindex;
    t_word *buf = x->x_vec, *wp;
    int i;
    double a3,a1,a2; // CH
    
    maxindex = x->x_npoints - 3;

    if (!buf) goto zero;

#if 0       /* test for spam -- I'm not ready to deal with this */
    for (i = 0,  xmax = 0, xmin = maxindex,  fp = in1; i < n; i++,  fp++)
    {
        t_sample f = *in1;
        if (f < xmin) xmin = f;
        else if (f > xmax) xmax = f;
    }
    if (xmax < xmin + x->c_maxextent) xmax = xmin + x->c_maxextent;
    for (i = 0, splitlo = xmin+ x->c_maxextent, splithi = xmax - x->c_maxextent,
        fp = in1; i < n; i++,  fp++)
    {
        t_sample f = *in1;
        if (f > splitlo && f < splithi) goto zero;
    }
#endif

    for (i = 0; i < n; i++)
    {
        t_sample findex = *in++;
        int index = findex;
        t_sample frac,  a,  b,  c,  d, cminusb;
        static int count;
        if (index < 1)
            index = 1, frac = 0;
        else if (index > maxindex)
            index = maxindex, frac = 1;
        else frac = findex - index;
        wp = buf + index;
        a = wp[-1].w_float;
        b = wp[0].w_float;
        c = wp[1].w_float;
        d = wp[2].w_float;
        /* if (!i && !(count++ & 1023))
            post("fp = %lx,  shit = %lx,  b = %f",  fp, buf->b_shit,  b); */
//        cminusb = c-b;
//        *out++ = b + frac * (
//            cminusb - 0.1666667f * (1.-frac) * (
//                (d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
// CH
	// 4-point, 3rd-order Hermite (x-form)
	a1 = 0.5f * (c - a);
	a2 = a - 2.5 * b + 2.f * c - 0.5f * d;
	a3 = 0.5f * (d - a) + 1.5f * (b - c);

	*out++ =  ((a3 * frac + a2) * frac + a1) * frac + b;     
    }
    return (w+5);
 zero:
    while (n--) *out++ = 0;

    return (w+5);
}

void tabread4c_tilde_set(t_tabread4c_tilde *x, t_symbol *s)
{
    t_garray *a;
    
    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
        if (*s->s_name)
            pd_error(x, "tabread4c~: %s: no such array", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else if (!garray_getfloatwords(a, &x->x_npoints, &x->x_vec))
    {
        pd_error(x, "%s: bad template for tabread4c~", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

static void tabread4c_tilde_dsp(t_tabread4c_tilde *x, t_signal **sp)
{
    tabread4c_tilde_set(x, x->x_arrayname);

    dsp_add(tabread4c_tilde_perform, 4, x,
        sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);

}

static void tabread4c_tilde_free(t_tabread4c_tilde *x)
{
}

void tabread4c_tilde_setup(void)
{
    tabread4c_tilde_class = class_new(gensym("tabread4c~"),
        (t_newmethod)tabread4c_tilde_new, (t_method)tabread4c_tilde_free,
        sizeof(t_tabread4c_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabread4c_tilde_class, t_tabread4c_tilde, x_f);
    class_addmethod(tabread4c_tilde_class, (t_method)tabread4c_tilde_dsp,
        gensym("dsp"), 0);
    class_addmethod(tabread4c_tilde_class, (t_method)tabread4c_tilde_set,
        gensym("set"), A_SYMBOL, 0);
}
