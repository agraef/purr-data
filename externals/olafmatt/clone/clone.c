/* -------------------------  clone  ------------------------------------------ */
/*                                                                              */
/* clone :: abstraction cloner object                                           */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>                                */
/* Based on rabin~ by Krzysztof Czaja.                                          */
/* Get source at http://www.akustische-kunst.org/puredata/maxlib/               */
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

#include "m_imp.h"
#include "g_canvas.h"
#include "clone.h"

#ifdef NT
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>

#define MAXINOUT    256 /* maximum number of control inlets / outlets */
#define MAXSIGINOUT 8   /* maximum number of signal inlets / outlets */

static char *version = "clone v0.0.6, written by Olaf Matthes <olaf.matthes@gmx.de>\n"
                       "              based on rabin~ by Krzysztof Czaja";

static t_class *clone_class;
static t_class *clone_rcv_class;
static t_class *clone_snd_class;
extern t_class *clone_in_class;
extern t_class *clone_sigin_class;
extern t_class *clone_out_class;
extern t_class *clone_sigout_class;

int ncloneinstance = 0;			/* number of instances of 'clone' */

typedef struct _cloneelement
{
    struct _clone        *e_owner;
    t_canvas             *e_ab;
    t_pd                 *e_target[MAXINOUT];
} t_cloneelement;

typedef struct _clone
{
    t_object       x_obj;
	t_outlet       *x_outlet[MAXINOUT];
    t_outlet       *x_rejectout;

    t_glist        *x_glist;     /* parent glist */
    int            x_vectorsize; /* signal vector size */
    t_float        *x_catchvec[MAXSIGINOUT]; /* signal vector */
    t_float        *x_throwvec[MAXSIGINOUT]; /* signal vector */
    t_symbol       *x_name;      /* .pd file name */
    t_symbol       *x_dir;       /* file directory */
	t_symbol       *x_rcvname[MAXINOUT];     /* our internal receive names */
    int            x_argc;       /* number of creation arguments (incl. inst. no.) */
    t_atom         *x_argv;      /* the creation arguments + prepended instance number */
    t_binbuf       *x_binbuf;    /* file contents */
    int            x_nelems;     /* number of instances we've created */
	t_int          x_nin;        /* number of control inlets */
	t_int          x_nsigin;     /* number of signal inlets */
	t_int          x_nout;       /* number of control outlets */
	t_int          x_nsigout;    /* number of signal outlets */
    int            x_tablesize;  /* as allocated */
    t_cloneelement *x_table;
	t_float        x_float;
} t_clone;

typedef struct _clone_rcv
{
    t_object r_obj;
	t_clone *r_owner;
	t_int    r_index;            /* number of outlet */
} t_clone_rcv;

typedef struct _clone_snd
{
    t_object s_obj;
	t_clone *s_owner;
	t_int    s_index;            /* number of inlet */
} t_clone_snd;


/* ------------------- stuff to handle the abstractions ---------------------- */

static t_symbol *clone_s_inlet, *clone_s_siginlet,
    *clone_s_outlet, *clone_s_sigoutlet,
    *clone_s_in, *clone_s_sigin,
    *clone_s_out, *clone_s_sigout;

static void clone_setup_symbols(void)
{
    clone_s_inlet = gensym("inlet");
    clone_s_siginlet = gensym("inlet~");
    clone_s_outlet = gensym("outlet");
    clone_s_sigoutlet = gensym("outlet~");
    clone_s_in = gensym("clonein");
    clone_s_sigin = gensym("clonein~");
    clone_s_out = gensym("cloneout");
    clone_s_sigout = gensym("cloneout~");
}

static void clone_inout_premapping(t_atom *ap)
{
    if (ap->a_type == A_SYMBOL)
    {
		if (ap->a_w.w_symbol == clone_s_inlet)
			SETSYMBOL(ap, clone_s_in);
		else if (ap->a_w.w_symbol == clone_s_siginlet)
			SETSYMBOL(ap, clone_s_sigin);
		else if (ap->a_w.w_symbol == clone_s_outlet)
			SETSYMBOL(ap, clone_s_out);
		else if (ap->a_w.w_symbol == clone_s_sigoutlet)
			SETSYMBOL(ap, clone_s_sigout);
    }
}

static void clone_inout_postmapping(t_atom *ap)
{
    if (ap->a_type == A_SYMBOL)
    {
		if (ap->a_w.w_symbol == clone_s_in)
			SETSYMBOL(ap, clone_s_inlet);
		else if (ap->a_w.w_symbol == clone_s_sigin)
			SETSYMBOL(ap, clone_s_siginlet);
		else if (ap->a_w.w_symbol == clone_s_out)
			SETSYMBOL(ap, clone_s_outlet);
		else if (ap->a_w.w_symbol == clone_s_sigout)
			SETSYMBOL(ap, clone_s_sigoutlet);
    }
}

	/* canvas_popabstraction() */
static t_canvas *clone_popabstraction(t_pd *x)
{
    t_canvas *cv = (t_canvas *)x;
    pd_popsym(x);  /* same as canvas_unsetcurrent() */
    cv->gl_loading = 0;
    canvas_resortinlets(cv);
    canvas_resortoutlets(cv);
    return (cv);  /* or 0, when we add a check */
}

static void clone_instance_init(t_clone *x, t_cloneelement *ep, t_pd *y, int instance)
{
    if (ep->e_ab = clone_popabstraction(y))
    {
		t_gobj *g;
		ep->e_owner = x;
		x->x_nin = x->x_nsigin = x->x_nout = x->x_nsigout = 0;
		for (g = ep->e_ab->gl_list; g; g = g->g_next)
		{
			if (g->g_pd == clone_in_class)
				ep->e_target[x->x_nin++] = (t_pd *)g;
			else if (g->g_pd == clone_sigin_class)
				clone_sigin_set((t_clone_sigin *)g, x->x_vectorsize, x->x_throwvec[x->x_nsigin++]);
			else if (g->g_pd == clone_out_class)
				clone_out_set((t_clone_out *)g, instance, x->x_rcvname[x->x_nout++]);
			else if (g->g_pd == clone_sigout_class)
				clone_sigout_set((t_clone_sigout *)g, x->x_vectorsize, x->x_catchvec[x->x_nsigout++]);
		}
    }
}

	/* send loadbang to all instances */
static void clone_loadbang(t_clone *x)
{
	t_canvas *cv;
	int i;

	for (i = 0; i < x->x_nelems; i++)
	{
		if (cv = x->x_table[i].e_ab)
			pd_vmess(&cv->gl_pd, gensym("loadbang"), "");
	}
}

	/* the two below borrow from binbuf_evalfile() -- which may change... */
static void clone_instantiate_all(t_clone *x)
{
    if (x->x_name && x->x_dir && x->x_binbuf)
    {
		int dspstate = canvas_suspend_dsp();
		t_pd *current = s__X.s_thing;
		t_cloneelement *ep;
		int i;

		for (i = 0, ep = x->x_table; i < x->x_nelems; i++, ep++)
		{
			glob_setfilename(0, x->x_name, x->x_dir);
			SETFLOAT(x->x_argv, i + 1);	/* first arg is number of instance */
			canvas_setargs(x->x_argc, x->x_argv);	/* pass args to canvas */
			binbuf_eval(x->x_binbuf, 0, 0, 0);
			if (s__X.s_thing != current)
				clone_instance_init(x, ep, current = s__X.s_thing, i);
			glob_setfilename(0, &s_, &s_);
			canvas_setargs(0, 0);
		}
		canvas_resume_dsp(dspstate);
		clone_loadbang(x);
    }
}

static void clone_loadfile(t_clone *x)
{
    if (x->x_name && x->x_dir)
    {
		glob_setfilename(0, x->x_name, x->x_dir);
		if (!x->x_binbuf) x->x_binbuf = binbuf_new();
		if (binbuf_read(x->x_binbuf, x->x_name->s_name, x->x_dir->s_name, 0))
		{
			binbuf_free(x->x_binbuf);
			x->x_binbuf = 0;
			perror(x->x_name->s_name);
		}
		else
		{
				/* rename inlets and outlets */
			t_atom *ap = binbuf_getvec(x->x_binbuf);
			int natoms = binbuf_getnatom(x->x_binbuf);
			int i;
			for (i = 0; i < natoms; i++, ap++) clone_inout_premapping(ap);
		}
    }
}

static int clone_new_table(t_clone *x, int nelems)
{
    int i;
    t_cloneelement *ep, **epp;
    if (!x->x_table || nelems > x->x_tablesize)
    {
		/* LATER check if vc supports null pointer passing */
		if (!(x->x_table = resizebytes(x->x_table,
						   x->x_tablesize * sizeof(t_cloneelement),
						   nelems * sizeof(t_cloneelement))))
		{
			x->x_tablesize = x->x_nelems = 0;
			return (0);
		}
		x->x_tablesize = nelems;
    }
    x->x_nelems = nelems;
    return (1);
}

static void clone_free_elems(t_clone *x)
{
    if (x->x_table && x->x_tablesize > 0)
    {
		int i;
		for (i = 0; i < x->x_nelems; i++)
			if (x->x_table[i].e_ab) canvas_free(x->x_table[i].e_ab);
    }
    x->x_nelems = 0;
}

static void clone_free_table(t_clone *x)
{
    if (x->x_table && x->x_tablesize > 0)
		freebytes(x->x_table, x->x_tablesize * sizeof(t_cloneelement));
    x->x_tablesize = 0;
    x->x_table = 0;
}

	/* returns: number of elements, or 0 in case of error */
static int clone_checkargs(int ac, t_atom *av)
{
    int nelems = 0;
    if (ac < 2 || av->a_type != A_SYMBOL
	|| av->a_w.w_symbol == &s_
	|| av[1].a_type != A_FLOAT
	|| (nelems = (int)av[1].a_w.w_float) < 1)
	error("clone: bad arguments");
    return (nelems > 0 ? nelems : 0);
}

/* ---------------------- methods --------------------------------- */
/*
  (This whole issue below is probably obsolete (we do not use ordinary
  inlets/outlets), but lets keep it that way -- just in case.)

  We must make sure there is always at most one instance visible.

  If user creates or deletes an inlet or outlet, Pd wants to kill
  clone box in order to recreate it with this change reflected.  This
  poses a problem, because clone box interface should not change until
  all instances are updated.  Unfortunately, we have to allow for the
  change, because it is hard-coded into canvas_class's widgetbehaviour.
  We cannot use our replacement class derived from the canvas_class,
  because there are various ``== canvas_class'' tests all over around
  Pd core, and failing those tests means cheating, and eventually
  crashing Pd.
*/

	/* show instance 'f' of our abstraction */
static void clone_show(t_clone *x, float f)
{
    if (x->x_table)
    {
		int i = (int)f;
		if (i > 0) i--;
		if (i >= 0 && i < x->x_nelems && x->x_table[i].e_ab)
		{
			int j;
			for (j = 0; j < x->x_nelems; j++)
			{
				if (j == i)
				{
					canvas_vis(x->x_table[i].e_ab, 1);
				}
				else if (x->x_table[j].e_ab)
					canvas_vis(x->x_table[j].e_ab, 0);
			}
		}
    }
}

	/* show abstraction in case we click on object */
static void clone_click(t_clone *x, t_floatarg xpos, t_floatarg ypos,
			t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    clone_show(x, 0);
}

	/* we don't accept anything else than lists ! */
static void clone_anything(t_clone *x, t_symbol *sel, int argc, t_atom *argv)
{
    outlet_anything(x->x_rejectout, sel, argc, argv);
}

	/* route input to the specified instance of the abstraction */
static void clone_list(t_clone *x, t_symbol *sel, int argc, t_atom *argv)
{
    if (x->x_table)
	{
		if (x->x_nin && !x->x_nsigin)
		{
			t_cloneelement *ep;
			int i, j;
			if (argc > 1)	    /* 2 or more args: treat as "list" */
			{
    			i = (int)atom_getfloat(argv);
				if(i <= 0)
					j = x->x_nelems, i = 0;
				else
					j = i--;
				if(j > x->x_nelems) goto reject;	/* out of range */

				for (ep = &x->x_table[i]; i < j; i++, ep++)
				{
					if (ep->e_target[0])
					{
						if (argv[1].a_type == A_SYMBOL)
							typedmess(ep->e_target[0], argv[1].a_w.w_symbol, argc-2, argv+2);
						else
							pd_list(ep->e_target[0], 0, argc-1, argv+1);
					}
				}
    			return;
			}
		}
		else post("clone: no method for list...");
	}
reject:
    outlet_list(x->x_rejectout, 0, argc, argv);
}

	/* send symbol to all instances */
static void clone_symbol(t_clone *x, t_symbol *s)
{
	if(x->x_nin && !x->x_nsigin)
	{
		t_atom list[2];
		SETFLOAT(list, 0);
		SETSYMBOL(list+1, s);
		clone_list(x, NULL, 2, list);
	}
	else post("clone: no method for symbol...");
}

	/* send float to all instances */
static void clone_float(t_clone *x, t_floatarg f)
{
	if(x->x_nin && !x->x_nsigin)
	{
		t_atom list[2];
		SETFLOAT(list, 0);
		SETFLOAT(list+1, f);
		clone_list(x, NULL, 2, list);
	}
	else post("clone: no method for float...");
}

	/* we don't accept anything else than lists ! */
static void clone_snd_anything(t_clone_snd *snd, t_symbol *sel, int argc, t_atom *argv)
{
	t_clone *x = (t_clone *)(snd->s_owner);
    outlet_anything(x->x_rejectout, sel, argc, argv);
}

	/* route input to the specified instance of the abstraction */
static void clone_snd_list(t_clone_snd *snd, t_symbol *sel, int argc, t_atom *argv)
{
	t_clone *x = (t_clone *)(snd->s_owner);
    if (x->x_table)
    {
		t_cloneelement *ep;
		int i, j;
		if (argc > 1)	    /* 2 or more args: treat as "list" */
		{
    		i = (int)atom_getfloat(argv);
			if(i <= 0)
				j = x->x_nelems, i = 0;
			else
				j = i--;
			if(j > x->x_nelems) goto reject;	/* out of range */

			for (ep = &x->x_table[i]; i < j; i++, ep++)
			{
				if (ep->e_target[0])
				{
					if (argv[1].a_type == A_SYMBOL)
						typedmess(ep->e_target[snd->s_index], argv[1].a_w.w_symbol, argc-2, argv+2);
					else
						pd_list(ep->e_target[0], 0, argc-1, argv+1);
				}
			}
    		return;
		}
	}
reject:
    outlet_list(x->x_rejectout, 0, argc, argv);
}

	/* send symbol to all instances */
static void clone_snd_symbol(t_clone_snd *snd, t_symbol *s)
{
	t_clone *x = (t_clone *)(snd->s_owner);
	t_atom list[2];
    SETFLOAT(list, 0);
    SETSYMBOL(list+1, s);
	clone_snd_list(snd, NULL, 2, list);
}

	/* send float to all instances */
static void clone_snd_float(t_clone_snd *snd, t_floatarg f)
{
	t_clone *x = (t_clone *)(snd->s_owner);
	t_atom list[2];
    SETFLOAT(list, 0);
    SETFLOAT(list+1, f);
	clone_snd_list(snd, NULL, 2, list);
}

	/* send received anything to outlet */
static void clone_rcv_anything(t_clone_rcv *r, t_symbol *s, int argc, t_atom *argv)
{
	t_clone *x = (t_clone *)(r->r_owner);
	outlet_anything(x->x_outlet[r->r_index], s, argc, argv);
}

	/* send received list to outlet */
static void clone_rcv_list(t_clone_rcv *r, t_symbol *s, int argc, t_atom *argv)
{
	t_clone *x = (t_clone *)(r->r_owner);
	outlet_list(x->x_outlet[r->r_index], s, argc, argv);
}

	/* send received symbol to outlet */
static void clone_rcv_symbol(t_clone_rcv *r, t_symbol *s)
{
	t_clone *x = (t_clone *)(r->r_owner);
	outlet_symbol(x->x_outlet[r->r_index], s);
}

	/* send received float to outlet */
static void clone_rcv_float(t_clone_rcv *r, t_floatarg f)
{
	t_clone *x = (t_clone *)(r->r_owner);
	outlet_float(x->x_outlet[r->r_index], f);
}

static t_int *clone_perform(t_int *w)
{
    t_clone *x = (t_clone *)(w[1]);
    t_float* throwout[MAXSIGINOUT];
    t_float* in[MAXSIGINOUT];
    t_float* catchin[MAXSIGINOUT];
    t_float* out[MAXSIGINOUT];
    int n = (int)(w[2 + x->x_nsigin + x->x_nsigout]);
	int i;

	for(i = 0; i < x->x_nsigin; i++)
	{
		in[i] = (t_float *)(w[2+i]);
		throwout[i] = (t_float *)(x->x_throwvec[i]);
	}

	for(i = 0; i < x->x_nsigout; i++)
	{
		catchin[i] = (t_float *)(x->x_catchvec[i]);
		out[i] = (t_float *)(w[2 + x->x_nsigin + i]);
	}

    while (n--)
	{
			/* throw audio to abstractions */
		for(i = 0; i < x->x_nsigin; i++)
			*throwout[i]++ = *in[i]++;
			/* catch audio from abstractions */
		for(i = 0; i < x->x_nsigout; i++)
			*out[i]++ = *catchin[i], *catchin[i]++ = 0;
	}
    return (w + 3 + x->x_nsigin + x->x_nsigout);
}

static void clone_dsp(t_clone *x, t_signal **sp)
{
    if (x->x_table)
    {
		t_canvas *cv;
		int i, nsig = x->x_nsigout + x->x_nsigin;
		for (i = 0; i < x->x_nelems; i++)
			if (cv = x->x_table[i].e_ab)
				mess1(&cv->gl_pd, gensym("dsp"), sp);

		if (x->x_nsigout || x->x_nsigin)
		{
			if (x->x_vectorsize != sp[0]->s_n)
			{
				error("clone: unexpected vector size");
				return;
			}
			switch (nsig)
			{
				case 1:
					dsp_add(clone_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
					break;
				case 2:
					dsp_add(clone_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, 
						sp[0]->s_n);
					break;
				case 3:
					dsp_add(clone_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, 
						sp[2]->s_vec, sp[0]->s_n);
					break;
				case 4:
					dsp_add(clone_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, 
						sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
					break;
				case 5:
					dsp_add(clone_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, 
						sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
					break;
				case 6:
					dsp_add(clone_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, 
						sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec,
						sp[0]->s_n);
					break;
				case 7:
					dsp_add(clone_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, 
						sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec,
						sp[6]->s_vec, sp[0]->s_n);
					break;
				case 8:
					dsp_add(clone_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, 
						sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec,
						sp[6]->s_vec, sp[7]->s_vec, sp[0]->s_n);
					break;
				default:
					break;
			}
		}
    }
}

static void clone_reload(t_clone *x, t_symbol *s, int ac, t_atom *av)
{
    int nelems;

    if (nelems = clone_checkargs(ac, av))
    {
		t_symbol *dir;
		int fd;
		int i;
		char dirbuf[MAXPDSTRING], *nameptr;
		ac--; s = av++->a_w.w_symbol;	/* name of abstraction */
		av++; ac--;
		canvas_setcurrent(x->x_glist);
		dir = canvas_getcurrentdir();
		if ((fd = open_via_path(dir->s_name, s->s_name, ".pd",
					dirbuf, &nameptr, MAXPDSTRING, 0)) >= 0)
		{
			close(fd);
			clone_free_elems(x);
			if (!clone_new_table(x, nelems))
			{
				canvas_unsetcurrent(x->x_glist);
				return;
			}
			x->x_name = gensym(nameptr);
			x->x_dir = gensym(dirbuf);
			if (x->x_argv) freebytes(x->x_argv, x->x_argc * sizeof(t_atom));
			x->x_argc = ac + 1;	/* we'll add the instance's number later */
			x->x_argv = getbytes(x->x_argc * sizeof(t_atom));	/* allocate memory */
			for(i = 0; i < ac; i++)		/* copy args, leave space for instance number */
			{
				x->x_argv[i + 1] = av[i];
			}
				/* Here goes: if (!pd_setloadingabstraction(s))
				   needed for nested loads.  It is local to pd.c. */
			clone_loadfile(x);
			clone_instantiate_all(x);
		}
		else post("no such abstraction: %s", s->s_name);
		canvas_unsetcurrent(x->x_glist);
    }
}

static void clone_free(t_clone *x)
{
	int i;
    clone_free_elems(x);
    clone_free_table(x);
	if (x->x_argv) freebytes(x->x_argv, x->x_argc * sizeof(t_atom));
	for(i = 0; i < MAXSIGINOUT; i++)
	{
		freebytes(x->x_catchvec[i], x->x_vectorsize * sizeof(t_float));
		freebytes(x->x_throwvec[i], x->x_vectorsize * sizeof(t_float));
	}
}

static void *clone_new(t_symbol *s, int ac, t_atom *av)
{
    t_clone *x = 0;
	t_clone_rcv *rcv[MAXINOUT];
	t_clone_snd *snd[MAXINOUT];
	int i;
	char rcvname[20];

	post(version);

    if (clone_checkargs(ac, av) && (x = (t_clone *)pd_new(clone_class)))
    {
		x->x_glist = canvas_getcurrent();
			/* create unique receive names */
		for(i = 0; i < MAXINOUT; i++)
		{
			sprintf(rcvname, "clone_rcv_%03d_%03d", ncloneinstance, i);
			x->x_rcvname[i] = gensym(rcvname);
		}
		ncloneinstance++;	/* count this new instance of clone */

		x->x_name = 0;
		x->x_dir = 0;
		x->x_argc = 0;
		x->x_argv = 0;
		x->x_binbuf = 0;
		x->x_nelems = 0;
		x->x_tablesize = 0;
		x->x_table = 0;
			/* catch & throw vector must be ready for clone_reload() */
		x->x_vectorsize = sys_getblksize();
		for (i = 0; i < MAXSIGINOUT; i++)
		{
			x->x_catchvec[i] = (t_float *)getbytes(x->x_vectorsize * sizeof(float));
			memset((char *)x->x_catchvec[i], 0, x->x_vectorsize * sizeof(float));
			x->x_throwvec[i] = (t_float *)getbytes(x->x_vectorsize * sizeof(float));
		}

		clone_reload(x, s, ac, av);

		if (x->x_table)
		{
				/* create inlets as needed */
			for (i = 1; i < x->x_nsigin; i++)		/* signal inlets come first... */
			{
				inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
			}
			for (i = 0; i < x->x_nin; i++)			/* ..followed by control inlets */
			{
				if(i == 0 && x->x_nsigin == 0)
					continue;                /* no need to create extra proxy inlet */
				snd[i] = (t_clone_snd *)pd_new(clone_snd_class);
				snd[i]->s_owner = x;          /* make x visible to the proxy inlets */
				snd[i]->s_index = i;	                     /* remember our number */
					/* the inlet we're going to create belongs to the object 
					   't_clone' but the destination is the instance 'i'
					   of the proxy inlet class 't_clone_snd'                       */
				inlet_new(&x->x_obj, &snd[i]->s_obj.ob_pd, 0,0);
			}
				/* create outlets as needed */
			for (i = 0; i < x->x_nsigout; i++)		/* signal outlets come first... */
				outlet_new(&x->x_obj, &s_signal);
			for (i = 0; i < x->x_nout; i++)			/* ..followed by control outlets */
			{
					/* create a 'receive-like' class that feeds the outlet */
				rcv[i] = (t_clone_rcv *)pd_new(clone_rcv_class);
				pd_bind(&rcv[i]->r_obj.ob_pd, x->x_rcvname[i]);   /* bind to receive name */
				rcv[i]->r_owner = x;                   /* make x visible to the new class */
				rcv[i]->r_index = i;
				x->x_outlet[i] = outlet_new(&x->x_obj, &s_list);
			}
			x->x_rejectout = outlet_new(&x->x_obj, &s_list);
		}
		else
		{
			pd_free((t_pd *)x);
			x = 0;
		}
    }
    return (x);
}

static t_widgetbehavior *clone_text_widgetbehaviorp;
static t_widgetbehavior clone_widgetbehavior;

static void clone_inout_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_binbuf *bb = ((t_text *)z)->te_binbuf;
    if (bb && binbuf_getnatom(bb))
		clone_inout_postmapping(binbuf_getvec(bb));
    (*clone_text_widgetbehaviorp->w_visfn)(z, glist, vis);
}

static void clone_inout_save(t_gobj *z, t_binbuf *b)
{
    t_binbuf *bb = ((t_text *)z)->te_binbuf;
    if (bb && binbuf_getnatom(bb))
		clone_inout_postmapping(binbuf_getvec(bb));
    (*clone_text_widgetbehaviorp->w_savefn)(z, b);
}

void clone_setup(void)
{
    clone_class = class_new(gensym("clone"), (t_newmethod)clone_new,
			    (t_method)clone_free, sizeof(t_clone), 0, A_GIMME, 0);
	class_addcreator((t_newmethod)clone_new, gensym("clone~"), A_GIMME, 0);
	CLASS_MAINSIGNALIN(clone_class, t_clone, x_float);

		/* a class for the proxy inlet: */
	clone_snd_class = class_new(gensym("clone_snd"), NULL, NULL, sizeof(t_clone_snd),
		CLASS_PD|CLASS_NOINLET, A_NULL);
		/* a class for the control outlet */
	clone_rcv_class = class_new(gensym("clone_rcv"), NULL, NULL, sizeof(t_clone_rcv),
		CLASS_PD|CLASS_NOINLET, A_NULL);

    class_addmethod(clone_class, (t_method)clone_reload, gensym("reload"), A_GIMME, 0);
    class_addmethod(clone_class, (t_method)clone_show, gensym("show"), A_DEFFLOAT, 0);
    class_addmethod(clone_class, (t_method)clone_click, gensym("click"),
		    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);

	class_addmethod(clone_class, (t_method)clone_dsp, gensym("dsp"), 0);
    class_addfloat(clone_class, (t_method)clone_float);
    class_addsymbol(clone_class, (t_method)clone_symbol);
    class_addlist(clone_class, (t_method)clone_list);
    class_addanything(clone_class, (t_method)clone_anything);
		/* methods for outlets */
    class_addfloat(clone_rcv_class, (t_method)clone_rcv_float);
    class_addsymbol(clone_rcv_class, (t_method)clone_rcv_float);
    class_addlist(clone_rcv_class, (t_method)clone_rcv_list);
    class_addanything(clone_rcv_class, (t_method)clone_rcv_anything);
		/* methods for proxy inlets */
    class_addfloat(clone_snd_class, (t_method)clone_snd_float);
    class_addsymbol(clone_snd_class, (t_method)clone_snd_symbol);
    class_addlist(clone_snd_class, (t_method)clone_snd_list);
    class_addanything(clone_snd_class, (t_method)clone_snd_anything);

    clone_setup_symbols();
    clone_text_widgetbehaviorp = clone_class->c_wb;
    clone_widgetbehavior = *clone_text_widgetbehaviorp;
    clone_widgetbehavior.w_visfn = clone_inout_vis;
    clone_widgetbehavior.w_savefn = clone_inout_save;
    clone_in_setup();
    class_setwidget(clone_in_class, &clone_widgetbehavior);
    clone_sigin_setup();
    class_setwidget(clone_sigin_class, &clone_widgetbehavior);
    clone_out_setup();
    class_setwidget(clone_out_class, &clone_widgetbehavior);
    clone_sigout_setup();
    class_setwidget(clone_sigout_class, &clone_widgetbehavior);

	class_sethelpsymbol(clone_class, gensym("help-clone.pd"));
}