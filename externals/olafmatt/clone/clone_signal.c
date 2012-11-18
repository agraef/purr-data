/* ------------------------- clone_signal ------------------------------------- */
/*                                                                              */
/* clone :: abstraction cloner object                                           */
/*   here:: signal inlets and outlets                                           */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>                                */
/* Based on rabin_sigout.c by Krzysztof Czaya.                                  */
/* Get source at http://www.akustische-kunst.org/puredata/clone/                */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* Based on PureData by Miller Puckette and others.                             */
/*                                                                              */
/* ---------------------------------------------------------------------------- */

#include <stdio.h>

#include "m_pd.h"
#include "clone.h"

#define undenormalise(sample) if(((*(unsigned int*)&sample)&0x7f800000)==0) sample=0.0

t_class *clone_sigout_class;

static void *clone_sigout_new(void)
{
    t_clone_sigout *x = (t_clone_sigout *)pd_new(clone_sigout_class);
    x->x_whereto  = 0;
    x->x_vs = sys_getblksize();
    x->x_f = 0;
    return (x);
}

static t_int *clone_sigout_perform(t_int *w)
{
    t_clone_sigout *x = (t_clone_sigout *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    int n = (int)(w[3]);
    t_float *out = x->x_whereto;
    if (out)
    {
    	while (n--)
	{
	    float f = *in++;
		undenormalise(f);
	    *out++ += f;
	}
    }
    return (w+4);
}

	/* called by clone (this is not a method!) */
void clone_sigout_set(t_clone_sigout *x, int vs, t_float *vec)
{
    if (vs == x->x_vs)
		x->x_whereto = vec;
    else
    {
		pd_error(x, "out~: vector size mismatch (set)");
		x->x_whereto = 0;
    }
}

static void clone_sigout_dsp(t_clone_sigout *x, t_signal **sp)
{
    if (sp[0]->s_n == x->x_vs)
		dsp_add(clone_sigout_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
    else
    	pd_error(x, "out~: vector size mismatch (dsp)");
}

void clone_sigout_setup(void)
{
    clone_sigout_class = class_new(gensym("cloneout~"), (t_newmethod)clone_sigout_new, 0,
				   sizeof(t_clone_sigout), 0, 0);
    CLASS_MAINSIGNALIN(clone_sigout_class, t_clone_sigout, x_f);
    class_addmethod(clone_sigout_class, (t_method)clone_sigout_dsp, gensym("dsp"), 0);
}

/* signal inlets */

t_class *clone_sigin_class;

static void *clone_sigin_new(void)
{
    t_clone_sigin *x = (t_clone_sigin *)pd_new(clone_sigin_class);
    x->x_wherefrom  = 0;
    x->x_vs = sys_getblksize();
    x->x_f = 0;
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static t_int *clone_sigin_perform(t_int *w)
{
    t_clone_sigin *x = (t_clone_sigin *)(w[1]);
    t_float *in = x->x_wherefrom;
    t_float *out = (t_float *)(w[2]);
    int n = (int)(w[3]);
    if (in)
    {
    	while (n--)
		{
			float f = *in++;
			undenormalise(f);
			*out++ = f;
		}
    }
    else
    {
    	while (n--)
		{
			*out++ = 0;
		}
    }
    return (w+4);
}

	/* called by clone (this is not a method!) */
void clone_sigin_set(t_clone_sigin *x, int vs, t_float *vec)
{
    if (vs == x->x_vs)
		x->x_wherefrom = vec;
    else
    {
		pd_error(x, "in~: vector size mismatch (set)");
		x->x_wherefrom = 0;
    }
}

static void clone_sigin_dsp(t_clone_sigin *x, t_signal **sp)
{
    if (sp[0]->s_n == x->x_vs)
		dsp_add(clone_sigin_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
    else
    	pd_error(x, "in~: vector size mismatch (dsp)");
}

void clone_sigin_setup(void)
{
    clone_sigin_class = class_new(gensym("clonein~"), (t_newmethod)clone_sigin_new, 0,
				   sizeof(t_clone_sigin), 0, 0);
    class_addmethod(clone_sigin_class, (t_method)clone_sigin_dsp, gensym("dsp"), 0);
}
