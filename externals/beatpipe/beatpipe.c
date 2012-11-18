/* beatpipe externals for pd
 * copyright 2003 Gerard van Dongen gml@xs4all.nl

* This program is free software; you can redistribute it and/or               
* modify it under the terms of the GNU General Public License                 
* as published by the Free Software Foundation; either version 2              
* of the License, or (at your option) any later version.                      
* This program is distributed in the hope that it will be useful,             
* but WITHOUT ANY WARRANTY; without even the implied warranty of              
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               
* GNU General Public License for more details.                                
* You should have received a copy of the GNU General Public License           
* along with this program; if not, write to the Free Software                 
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 



* objects:  
* beatpipe
* schedules and quantizes lists. A realtime qlist, sort of.

 */



#include "m_pd.h"
#include <stdio.h>
#include <math.h>
static t_class *beatpipe_class;


typedef struct queue_node{
	float time;
	int argc;
	t_atom *argv;
	struct queue_node * next;
	struct queue_node * previous;
}t_queue;

typedef struct {
	t_object x_obj;
	t_queue *queue;
	t_clock *x_clock;
	double current_beat;
	t_float tempo;
	t_float tpq;
	double delay;
	int frozen;
	t_outlet *queue_out,*done_out;
}t_beatpipe;




void beatpipe_tick(t_beatpipe *x)
{


	x->current_beat = x->current_beat + 1/x->tpq;
 	x->delay = 60000/(x->tempo*x->tpq);//to _milisecs_!!
	if (x->frozen) return;

	while (x->queue) {
		//pop of events of the top of the list

		if (x->current_beat >= x->queue->time) {
		
			t_queue *temp;
			temp = x->queue;
			outlet_anything(x->queue_out,&s_list,x->queue->argc,x->queue->argv);
			x->queue = x->queue->next;
			freebytes(temp->argv,temp->argc*sizeof(t_atom));
			freebytes(temp,sizeof(t_queue));

			//if empty put out a bang on the right outlet:
			if (!x->queue) outlet_bang(x->done_out);


		} else {
			break;
		}
	}
	clock_delay(x->x_clock,x->delay);


}

void beatpipe_bang(t_beatpipe *x)
{
	x->frozen = 0;
	beatpipe_tick(x);
}

void beatpipe_freeze(t_beatpipe *x)
{
	x->frozen =1;
}
void beatpipe_sync(t_beatpipe *x)
{
	x->frozen = 0;
	clock_unset(x->x_clock);
	beatpipe_tick(x);
}
void beatpipe_syncbeat(t_beatpipe *x)
{
	x->frozen = 0;
	clock_unset(x->x_clock);
	x->current_beat =floor(x->current_beat+1.0);
	beatpipe_tick(x);

}

void beatpipe_set_tpq(t_beatpipe *x,t_floatarg tpq)
{
	x->tpq = tpq;
	if (x->tpq == 0) x->tpq = 96;
}


void beatpipe_clear(t_beatpipe *x)
{
	while (x->queue) {
		t_queue *temp;
		temp = x->queue;
		freebytes(x->queue,sizeof(t_queue));
		x->queue = temp->next;
	}
}




void beatpipe_list(t_beatpipe *x,t_symbol *s,int argc, t_atom* argv)
{
	t_float beat;
	t_queue *new;
	t_queue * temp;
	t_queue * previous;

	beat = atom_getfloatarg(0,argc,argv);
	// if time<=0 (or first element of list is not a number)output immediatly
	if (beat<=0.0)	{ 
		outlet_anything(x->queue_out,s,argc-1,argv+1);
		return;
	}

	beat=beat+x->current_beat;

	//now insert in the queue

	new = getbytes(sizeof(t_queue));
	new->time = beat;
	new->argc = argc-1;
	new->argv = copybytes(argv+1,new->argc * sizeof(t_atom));


	//is queue empty?
	if (!x->queue) {

	
		x->queue = new;
		new->next = NULL;
		new->previous = NULL;
		return;
	}

	//now walk the queue

	temp = x->queue;
	
	while (temp) {

		if (!temp->next) {
			temp->next =   new;
			new->previous =  temp;
			return;
		}
			
		if (beat > temp->time) {
			temp=  temp->next;
		
		} else {
			if(!temp->previous) {
				x->queue = new;
				new->next = temp;
				new->previous = NULL;
				return;
			}
			new->previous = temp->previous;
			temp->previous =  new;
			new->next = temp;
			temp= new->previous;
			temp->next= new;
			return;
		}
	}





}

void *beatpipe_new(t_floatarg tempo,t_floatarg tpq)
{


	t_beatpipe *x =(t_beatpipe *)pd_new(beatpipe_class);

	x->x_clock = clock_new(x, (t_method)beatpipe_tick);


	x->queue = NULL;

	if (tempo != 0.0)
		x->tempo = tempo;
	else
		x->tempo = 60.0;

	if (tpq !=0.0)
		x->tpq = tpq;
	else
		x->tpq = 96.0;


	x->current_beat = 0;
	x->delay = (double) 60000/(x->tempo*x->tpq);//to _milisecs_!!
	floatinlet_new(&x->x_obj,&x->tempo);

	clock_delay(x->x_clock,x->delay);

	x->queue_out=outlet_new(&x->x_obj,0);

	x->done_out=outlet_new(&x->x_obj, &s_bang);
	x->frozen = 0;
	return (x);

}

void beatpipe_free(t_beatpipe *x)
{
	clock_free(x->x_clock);

	while (x->queue) {
		t_queue *temp;
		temp = x->queue;
		freebytes(x->queue,sizeof(t_queue));
		x->queue = temp->next;
	}

}


void beatpipe_setup(void)
{


	beatpipe_class = class_new(gensym("beatpipe"), 
				 (t_newmethod)beatpipe_new,
				 (t_method)beatpipe_free,
				 sizeof(t_beatpipe),
				 CLASS_DEFAULT,
				 A_DEFFLOAT,
				 A_DEFFLOAT,0);

	class_addbang(beatpipe_class,(t_method) beatpipe_bang);

	class_addmethod(beatpipe_class, (t_method)beatpipe_clear,gensym("clear"),0);
	class_addmethod(beatpipe_class, (t_method)beatpipe_sync,gensym("sync"),0);
	class_addmethod(beatpipe_class, (t_method)beatpipe_syncbeat,gensym("sync-beat"),0);
	class_addmethod(beatpipe_class, (t_method)beatpipe_set_tpq,gensym("set-tpq"),A_DEFFLOAT,0);
	class_addmethod(beatpipe_class, (t_method)beatpipe_freeze,gensym("freeze"),0);
	class_addmethod(beatpipe_class, (t_method)beatpipe_bang,gensym("continue"),0);
	class_addlist(beatpipe_class,(t_method) beatpipe_list);
}
