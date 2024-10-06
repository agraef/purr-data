/*
Copyright (C) 2002 Antoine Rousseau

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/* this file is made from parts of m_object.c and g_io.c
* 	it defines a signal inlet named dinlet~ which is the same as inlet~
*  exepts you can give a default float value for the case none signal
*  is connected to this inlet~. */

/***********************************************************************/
/* CAUTION :
		You MUST fix a bug pd sources and recompile them in order to have
	dinlet~ working !!

	this function must be fixed in pd/m_obj.c:								  */
#if 0
t_sample *obj_findsignalscalar(t_object *x, int m)
{
    int n = 0,mbak=m;
    t_inlet *i;
    post("my obj_findsignalscalar");
    if (x->ob_pd->c_firstin && x->ob_pd->c_floatsignalin)
    {
        if (!m--)
            return (x->ob_pd->c_floatsignalin > 0 ?
                    (t_sample *)(((char *)x) + x->ob_pd->c_floatsignalin) : 0);
        n++;
    }
    for (i = x->ob_inlet; i; i = i->i_next, m--)
        if (i->i_symfrom == &s_signal)
        {
            /*if (m == 0)*/
            if(n==mbak)
                return (&i->i_un.iu_floatsignalvalue);
            n++;
        }
    return (0);
}
#endif
/***********************************************************************/

#include "m_pd.h"
#include "m_imp.h"
#include "g_canvas.h"
#include <string.h>

/******************** from m_obj.c: **************************/
/* only because inlet_float() is not public... */
union inletunion
{
    t_symbol *iu_symto;
    t_gpointer *iu_pointerslot;
    t_float *iu_floatslot;
    t_symbol **iu_symslot;
    t_blob **iu_blobslot; /* MP 20061226 blob type */
    t_sample iu_floatsignalvalue;
};

struct _inlet
{
    t_pd i_pd;
    struct _inlet *i_next;
    t_object *i_owner;
    t_pd *i_dest;
    t_symbol *i_symfrom;
    union inletunion i_un;
};

#define i_symto i_un.iu_symto

static void dinlet_float(t_inlet *x, t_float f)
{
    if (x->i_symfrom == &s_float)
        pd_vmess(x->i_dest, x->i_symto, "f", (t_floatarg)f);
    else if (x->i_symfrom == &s_signal)
        x->i_un.iu_floatsignalvalue = f;
    else if (!x->i_symfrom)
        pd_float(x->i_dest, f);
    /*else inlet_wrong(x, &s_float);*/
}

/* ------------------------- vinlet -------------------------- */
extern t_class *vinlet_class;

typedef struct _reblocker
{
    t_sample *r_buf;         /* signal buffer; zero if not a signal */
    t_resample r_updown;
} t_reblocker;

static void reblocker_init(t_reblocker *rb, int buflength)
{
    rb->r_buf = (t_sample *)getbytes(buflength * sizeof(t_sample));
    resample_init(&rb->r_updown);
}

typedef struct _vinlet
{
    t_object x_obj;
    t_canvas *x_canvas;
    t_inlet *x_inlet;
    int x_buflength;        /* number of samples per channel in buffer */
    int x_write;            /* write position in reblocker */
    int x_read;             /* read position in reblocker */
    int x_hop;
    int x_updownmethod;
            /* if not reblocking, the next slot communicates the parent's
                inlet signal from the prolog to the DSP routine: */
    t_signal *x_directsignal;
    int x_nchans;       /* this is also set in prolog & used in dsp */
    t_outlet *x_fwdout; /* optional outlet for forwarding messages to inlet~ */
    t_reblocker *x_rb;  /* reblocking and resampling, one per channel */
} t_vinlet;

// ag: I fixed the class data structure and initialization once more for
// multi-channel compatibility. However, I'm not sure whether it makes much
// sense to keep maintaining this object any longer, because it requires
// pulling so much source from g_io.c, and isn't really compatible with the
// new inlet~ anymore because it lacks the message forwarding.

static void *dinlet_newsig(t_floatarg f)
{
    t_vinlet *x = (t_vinlet *)pd_new(vinlet_class);
    x->x_canvas = canvas_getcurrent();
    x->x_inlet = canvas_addinlet(x->x_canvas, &x->x_obj.ob_pd, &s_signal);
    x->x_nchans = 1;
    x->x_buflength = 0;
    x->x_rb = (t_reblocker *)getbytes(sizeof(*x->x_rb));
    reblocker_init(x->x_rb, x->x_buflength);
    x->x_directsignal = 0;
    x->x_fwdout = 0;
    x->x_directsignal = 0;
    x->x_inlet->i_un.iu_floatsignalvalue=f;
    outlet_new(&x->x_obj, &s_signal);
    x->x_updownmethod = -1;
    return (x);
}

void dinlet_tilde_setup(void)
{
    class_addcreator((t_newmethod)dinlet_newsig, gensym("dinlet~"), A_DEFFLOAT,0);
}
