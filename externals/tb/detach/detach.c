/* 
* 
* detach
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
#include "m_fifo.h"

#include "pthread.h"


static t_class *detach_class;

typedef t_fifo fifo_t; /* for emacs syntax highlighting */

typedef struct _detach
{
    t_object x_obj;

	t_outlet * x_outlet;

	pthread_t x_thread;
	pthread_mutex_t x_mutex;
    pthread_cond_t x_cond;

	fifo_t * x_fifo;

} detach_t;

typedef struct _detach_content
{
	struct _detach_content_t * next;
	enum { BANG, 
		   POINTER,
		   FLOAT,
		   SYMBOL,
		   LIST,
		   ANYTHING,
		   CANCEL} type;
	int argc;
	t_atom * argv;
	t_symbol * symbol;
} detach_content_t;


static void detach_thread(detach_t* x)
{
	detach_content_t * me;
	while(1)
	{
		pthread_cond_wait(&x->x_cond, &x->x_mutex);
		
		me = (detach_content_t*) fifo_get(x->x_fifo);
		
		while (me != NULL)
		{
			/* run function */
			switch (me->type)
			{
			case BANG:
				outlet_bang(x->x_outlet);
				break;
			case FLOAT:
				outlet_float(x->x_outlet, atom_getfloat(me->argv));
				break;
			case SYMBOL:
				outlet_symbol(x->x_outlet, atom_getsymbol(me->argv));
				break;
			case LIST:
				outlet_list(x->x_outlet, 0, me->argc, me->argv);
				freebytes(me->argv, me->argc * sizeof(t_atom));
				break;
			case ANYTHING:
				outlet_anything(x->x_outlet, me->symbol, me->argc, me->argv);
				freebytes(me->argv, me->argc * sizeof(t_atom));
				break;
			case CANCEL:
				goto done;
				
			}
			
			/* free */
			if (me->argc)
				freebytes(me->argv, me->argc * sizeof (t_atom));
			freebytes (me, sizeof(detach_content_t));
			
			me = (detach_content_t*) fifo_get(x->x_fifo);
		}
	}
	
 done: /* free the fifo and exit */

	do 
	{
		if (me->argc)
			freebytes(me->argv, me->argc * sizeof (t_atom));
		freebytes (me, sizeof(detach_content_t));
	}
	while (me = (detach_content_t*) fifo_get(x->x_fifo));

	fifo_destroy(x->x_fifo);
	return;
}

/* todo: take argument for thread priority */
static detach_t * detach_new(void)
{
	detach_t *x = (detach_t*) pd_new(detach_class);
	pthread_attr_t thread_attr;
	struct sched_param thread_sched;
	int status;

	x->x_outlet = outlet_new(&x->x_obj, NULL);
	x->x_fifo = fifo_init();

	/* thread initialisation */
	pthread_mutex_init (&x->x_mutex,NULL);
	pthread_mutex_unlock (&x->x_mutex);
	pthread_cond_init (&x->x_cond,NULL);

	pthread_attr_init(&thread_attr);
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
    thread_sched.sched_priority=sched_get_priority_min(SCHED_OTHER);
    pthread_attr_setschedparam(&thread_attr,&thread_sched);
#endif
	status = pthread_create(&x->x_thread, &thread_attr,
							(void*)detach_thread, x);

#if 1
	if (status == 0)
		post("detaching thread");
#endif
	return x;
}


static void detach_free(detach_t * x)
{
	detach_content_t * me = getbytes(sizeof(detach_content_t));
	
	me->type = CANCEL;
	me->argc = 0;
	fifo_put(x->x_fifo, me);
	pthread_cond_broadcast(&x->x_cond);
}


static void detach_bang(detach_t * x)
{
	detach_content_t * me = getbytes(sizeof(detach_content_t));
	
	me->type = BANG;
	me->argc = 0;
	fifo_put(x->x_fifo, me);
	
	pthread_cond_broadcast(&x->x_cond);
}


static void detach_float(detach_t * x, t_float f)
{
	detach_content_t * me = getbytes(sizeof(detach_content_t));
	
	me->type = FLOAT;
	me->argc = 1;
	me->argv = getbytes(sizeof(t_atom));
	SETFLOAT(me->argv, f);
	fifo_put(x->x_fifo, me);
	
	pthread_cond_broadcast(&x->x_cond);
}

static void detach_pointer(detach_t * x, t_gpointer* gp)
{
	detach_content_t * me = getbytes(sizeof(detach_content_t));
	
	me->type = POINTER;
	me->argc = 1;
	me->argv = getbytes(sizeof(t_atom));
	SETPOINTER(me->argv, gp);
	fifo_put(x->x_fifo, me);
	
	pthread_cond_broadcast(&x->x_cond);
}

static void detach_symbol(detach_t * x, t_symbol * s)
{
	detach_content_t * me = getbytes(sizeof(detach_content_t));
	
	me->type = SYMBOL;
	me->argc = 1;
	me->argv = getbytes(sizeof(t_atom));
	SETSYMBOL(me->argv, s);
	fifo_put(x->x_fifo, me);
	
	pthread_cond_broadcast(&x->x_cond);
}


static void detach_list(detach_t * x, t_symbol * s, 
						 int argc, t_atom* argv)
{
	detach_content_t * me = getbytes(sizeof(detach_content_t));
	
	me->type = LIST;
	me->argc = argc;
	me->argv = copybytes(argv, argc * sizeof(t_atom));
	fifo_put(x->x_fifo, me);
	
	pthread_cond_broadcast(&x->x_cond);
}

static void detach_anything(detach_t * x, t_symbol * s, 
							 int argc, t_atom* argv)
{
	detach_content_t * me = getbytes(sizeof(detach_content_t));
	
	me->type = ANYTHING;
	me->argc = argc;
	me->argv = copybytes(argv, argc * sizeof(t_atom));
	me->symbol = s;
	fifo_put(x->x_fifo, me);
	
	pthread_cond_broadcast(&x->x_cond);
}


void detach_setup(void)
{
	detach_class = class_new(gensym("detach"), (t_newmethod)detach_new,
							  (t_method)detach_free, sizeof(detach_t),
							  CLASS_DEFAULT, 0);
	class_addbang(detach_class, detach_bang);
    class_addfloat(detach_class, detach_float);
    class_addpointer(detach_class, detach_pointer);
    class_addsymbol(detach_class, detach_symbol);
    class_addlist(detach_class, detach_list);
    class_addanything(detach_class, detach_anything);
}
