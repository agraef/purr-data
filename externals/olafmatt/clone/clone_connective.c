/* ------------------------- clone_connective --------------------------------- */
/*                                                                              */
/* clone :: abstraction cloner object                                           */
/*   here:: control inlets and outlets                                          */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>                                */
/* Based on Pd's x_connective.c by Miller Puckette.                             */
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

#include "m_imp.h"
#include "g_canvas.h"
#include "clone.h"

#if 1
#define clone_VERBOSE
#if 0
#define clone_DEBUG
#endif
#endif


/* ---------------------- out class --------------------------- */
/*           (after send_class from x_connective.c)             */

t_class *clone_out_class;

static void clone_out_bang(t_clone_out *x)
{
    if (x->x_sym->s_thing)
		pd_float(x->x_sym->s_thing, x->x_ab + 1);
}

static void clone_out_float(t_clone_out *x, t_float f)
{
    if (x->x_sym->s_thing)
	{
		t_atom list[2];
		SETFLOAT(list, x->x_ab + 1);
		SETFLOAT(list+1, f);
		pd_list(x->x_sym->s_thing, NULL, 2, list);
	}
}

static void clone_out_symbol(t_clone_out *x, t_symbol *s)
{
    if (x->x_sym->s_thing)
	{
		t_atom list[2];
		SETFLOAT(list, x->x_ab + 1);
		SETSYMBOL(list+1, s);
		pd_list(x->x_sym->s_thing, NULL, 2, list);
	}
}

static void clone_out_pointer(t_clone_out *x, t_gpointer *gp)
{
    if (x->x_sym->s_thing)
	{
		t_atom list[2];
		SETFLOAT(list, x->x_ab + 1);
		SETPOINTER(list+1, gp);
		pd_list(x->x_sym->s_thing, NULL, 2, list);
	}
}

static void clone_out_list(t_clone_out *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_sym->s_thing)
	{
		t_atom *list;
		int i;
		list = getbytes((argc + 1) * sizeof(t_atom));	/* allocate memory */;
		for(i = 0; i < argc; i++)  /* copy args, leave space for instance number */
		{
			list[i + 1] = argv[i];
		}
		SETFLOAT(list, x->x_ab + 1);
		pd_list(x->x_sym->s_thing, NULL, argc+1, list);
		freebytes(list, (argc + 1) * sizeof(t_atom));	/* free memory */;
	}
}

static void clone_out_anything(t_clone_out *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_sym->s_thing)
	{
		t_atom *list;
		int i;
		list = getbytes((argc + 2) * sizeof(t_atom));	/* allocate memory */;
		for(i = 0; i < argc; i++)  /* copy args, leave space for instance number */
		{
			list[i + 2] = argv[i];
		}
		SETFLOAT(list, x->x_ab + 1);
		SETSYMBOL(list+1, s);
		pd_list(x->x_sym->s_thing, NULL, argc+2, list);
		freebytes(list, (argc + 2) * sizeof(t_atom));	/* free memory */;
	}
}

void clone_out_set(t_clone_out *x, int i, t_symbol *s)
{
	x->x_sym = s;	/* set the receive name */
	x->x_ab = i;    /* the instance that called us */
}

static void *clone_out_new(t_symbol *s)
{
    t_clone_out *x = (t_clone_out *)pd_new(clone_out_class);
    return (x);
}

void clone_out_setup(void)
{
    clone_out_class = class_new(gensym("cloneout"), (t_newmethod)clone_out_new, 0,
    	sizeof(t_clone_out), 0, A_DEFSYM, 0);
    class_addbang(clone_out_class, clone_out_bang);
    class_addfloat(clone_out_class, clone_out_float);
    class_addsymbol(clone_out_class, clone_out_symbol);
    class_addpointer(clone_out_class, clone_out_pointer);
    class_addlist(clone_out_class, clone_out_list);
    class_addanything(clone_out_class, clone_out_anything);
}

/* -------------------- in class ------------------------------ */
/*        (after receive_class from x_connective.c)             */

t_class *clone_in_class;

typedef struct _clone_in
{
    t_object  x_obj;
} t_clone_in;

static void clone_in_bang(t_clone_in *x)
{
    outlet_bang(x->x_obj.ob_outlet);
}

static void clone_in_float(t_clone_in *x, t_float f)
{
    outlet_float(x->x_obj.ob_outlet, f);
}

static void clone_in_symbol(t_clone_in *x, t_symbol *s)
{
    outlet_symbol(x->x_obj.ob_outlet, s);
}

static void clone_in_pointer(t_clone_in *x, t_gpointer *gp)
{
    outlet_pointer(x->x_obj.ob_outlet, gp);
}

static void clone_in_list(t_clone_in *x, t_symbol *s, int argc, t_atom *argv)
{
    outlet_list(x->x_obj.ob_outlet, s, argc, argv);
}

static void clone_in_anything(t_clone_in *x,
			      t_symbol *s, int argc, t_atom *argv)
{
    outlet_anything(x->x_obj.ob_outlet, s, argc, argv);
}

static void *clone_in_new(t_symbol *s)
{
    t_clone_in *x = (t_clone_in *)pd_new(clone_in_class);
    outlet_new(&x->x_obj, 0);
    return (x);
}

void clone_in_setup(void)
{
    clone_in_class = class_new(gensym("clonein"), (t_newmethod)clone_in_new, 0,
			       sizeof(t_clone_in), CLASS_NOINLET, 0);
    class_addbang(clone_in_class, clone_in_bang);
    class_addfloat(clone_in_class, (t_method)clone_in_float);
    class_addsymbol(clone_in_class, clone_in_symbol);
    class_addpointer(clone_in_class, clone_in_pointer);
    class_addlist(clone_in_class, clone_in_list);
    class_addanything(clone_in_class, clone_in_anything);
}
