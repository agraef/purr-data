/* Copyright (c) 2002 Damien HENRY.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* ------------------------  based on remote  --------------------------------- */
/*                                                                              */
/* Send data to receive obejct <name>.                                          */
/* Written by Olaf Matthes (olaf.matthes@gmx.de)                                */

#include "m_pd.h"

/*dh : 
#include <string.h>
#include <stdio.h> */

#define MAX_REC 64		/* maximum number of receive objects */
#define MAX_ARG 32		/* maximum number of arguments to pass on */

static t_class *send2_class;

/*dh: I've removed the static *char */

typedef struct _send2
{
    t_object x_obj;
} t_send2;

	/* send 'anything' to receiver */
static void send2_anything(t_send2 *x, t_symbol *s, int argc, t_atom *argv)
{
	int i;
	t_atom av[MAX_ARG];		/* the 'new' t_atom without first element */
	t_int ac = argc - 1;		/* the 'new' number of arguments */

	if(argc < 1)			/* need <name> <data> */
	{
		post("send2: too few arguments!");
		return;
	}
	if(ac > MAX_ARG)
	{
		post("send2: too many arguments!");
		return;
	}

	for(i = 1; i < argc; i++)
	{
		av[i - 1] = argv[i];	/* just copy, don't care about types */
	}
		/* send only argument-part to receivers */
	if (s->s_thing) pd_forwardmess(s->s_thing, argc, argv);
}

static void *send2_new(void)
{
    t_send2 *x = (t_send2 *)pd_new(send2_class);
    /*dh: I've removed the post here */
    return (x);
}

void send2_setup(void)
{
    send2_class = class_new(gensym("send2"), (t_newmethod)send2_new, 0,
    	sizeof(t_send2), 0, 0);
    class_addanything(send2_class, send2_anything);
	class_sethelpsymbol(send2_class, gensym("xgui/help_send2.pd"));
}
