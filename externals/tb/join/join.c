/* 
* 
* detatch
* Copyright (C) 2005  Tim Blechmann
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*/

#include "m_pd.h"

static t_class *join_class;

typedef struct _join
{
    t_object x_obj;
	t_outlet * x_outlet;
} join_t;


static join_t * join_new(void)
{
	join_t *x = (join_t*) pd_new(join_class);

	x->x_outlet = outlet_new(&x->x_obj, NULL);
	return x;
}


static t_int join_bang_callback(t_int * argv)
{
	outlet_bang((t_outlet*)argv[0]);
	return 0;
}

static void join_bang(join_t * x)
{
	t_int* argv = getbytes(sizeof(t_int*));
	argv[0] = (t_int)x->x_outlet;
	set_callback(join_bang_callback, argv, 1);
}


static t_int join_pointer_callback(t_int * argv)
{
	outlet_pointer((t_outlet*)argv[0], (t_gpointer*)argv[1]);
	return 0;
}

static void join_pointer(join_t * x, t_gpointer * gp)
{
	t_int* argv = getbytes(2*sizeof(t_int*));
	argv[0] = (t_int)x->x_outlet;
	argv[1] = (t_int)gp;

	set_callback(join_pointer_callback, argv, 2);
}


static t_int join_float_callback(t_int * argv)
{
	outlet_float((t_outlet*)argv[0], (t_float)argv[1]);
	return 0;
}

static void join_float(join_t * x, t_float f)
{
	t_int* argv = getbytes(2*sizeof(t_int*));
	argv[0] = (t_int)x->x_outlet;
	argv[1] = (t_int)f;

	set_callback(join_float_callback, argv, 2);
}


static t_int join_symbol_callback(t_int * argv)
{
	outlet_symbol((t_outlet*)argv[0], (t_symbol*)argv[1]);
	return 0;
}

static void join_symbol(join_t * x, t_symbol * s)
{
	t_int* argv = getbytes(2*sizeof(t_int*));
	argv[0] = (t_int)x->x_outlet;
	argv[1] = (t_int)s;

	set_callback(join_symbol_callback, argv, 2);
}

static t_int join_list_callback(t_int * argv)
{
	outlet_list((t_outlet*)argv[0], 0, (int)argv[1], (t_atom*)argv[2]);
	freebytes ((t_atom*)argv[2], (int)argv[1] * sizeof(t_atom));
	return 0;
}

static void join_list(join_t * x, t_symbol * s, int argc, t_atom* largv)
{
	t_int* argv = getbytes(3*sizeof(t_int*));
	t_atom* copied_argv = copybytes(largv, argc * sizeof(t_atom));

	argv[0] = (t_int)x->x_outlet;
	argv[1] = (t_int)argc;
	argv[2] = (t_int)copied_argv;

	set_callback(join_list_callback, argv, 3);
}


static t_int join_anything_callback(t_int * argv)
{
	outlet_anything((t_outlet*)argv[0], &s_list, 
					(int)argv[1], (t_atom*)argv[2]);
	freebytes ((t_atom*)argv[2], (int)argv[1] * sizeof(t_atom));
	return 0;
}

static void join_anything(join_t * x, t_symbol * s, int argc, t_atom* largv)
{
	t_int* argv = getbytes(3*sizeof(t_int*));
	t_atom* copied_argv = copybytes(largv, argc * sizeof(t_atom));

	argv[0] = (t_int)x->x_outlet;
	argv[1] = (t_int)argc;
	argv[2] = (t_int)copied_argv;

	set_callback(join_anything_callback, argv, 3);
}


void join_setup(void)
{
	join_class = class_new(gensym("join"), (t_newmethod)join_new,
						   NULL, sizeof(join_t),
						   CLASS_DEFAULT, 0);
	class_addbang(join_class, join_bang);
    class_addfloat(join_class, join_float);
    class_addpointer(join_class, join_pointer);
    class_addsymbol(join_class, join_symbol);
    class_addlist(join_class, join_list);
    class_addanything(join_class, join_anything);
}
