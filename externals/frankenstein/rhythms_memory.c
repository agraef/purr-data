/* 
rhythms_memory

Authors:
Davide Morelli http://www.davidemorelli.it
David Plans Casal http://www.studios.uea.ac.uk/people/staff/casal

uses graphs to store rhythms

TODO:
  * memory save/load to file (?)
  * output rhythms in the form of list of floats (easy)
  * add velo (?)
  * input rhythms from a list (second inlet) (easy)
  * let it create variations on known rhythms (?)
  * let it merge rhythms (?)

*/

#include "m_pd.h"

#include "common.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>

static t_class *rhythms_memory_class;

typedef struct event event;
typedef struct event
{
	unsigned short int voice;
	double when;
	event *next;
};

typedef struct _rhythms_memory
{
    t_object x_obj; // myself
	// 3 outlets:
	// bangs_out plays the wanted rhythmsin realtime
	// list_out outputs the wanted rhythm as a list of floats
	// info_out outputs info on the last recognized rhythm
	t_outlet *bangs_out, *list_out, *info_out;
	t_rhythm_event *curr_seq;
	int seq_initialized;
	// the memory
	t_rhythm_memory_representation *rhythms_memory;
	// measure length
	double measure_start_time;
	double measure_length;
	// input rhythm's events
	event *events;
	// output rhythm's events
	unsigned short int next_main_rhythm_out;
	unsigned short int next_sub_rhythm_out;
	event *events_out;
	t_clock *x_clock;
    double x_deltime;
	double last_event_out_time;
	
} t_rhythms_memory;

void rhythms_memory_free(t_rhythms_memory *x)
{
	if (x->curr_seq)
		freeBeats(x->curr_seq);	
	if (x->rhythms_memory)
		rhythm_memory_free(x->rhythms_memory);

	clock_free(x->x_clock);
}

// called when a new measure starts
void start_measure(t_rhythms_memory *x)
{
	// I call the pd functions to get a representation
	// of this very moment
	x->measure_start_time = clock_getlogicaltime();
	x->last_event_out_time = 0;
}

// called when a new event occours
void add_event(t_rhythms_memory *x, unsigned short int voice)
{
	event *newEvent, *currEvent, *lastEvent;
	double when;
	when = clock_gettimesince(x->measure_start_time);
	newEvent = (event *) malloc(sizeof(event));
	newEvent->when = when;
	newEvent->voice = voice;
	newEvent->next = 0;
	currEvent = x->events;
	if (currEvent)
	{
		// this is not the first event
		while(currEvent)
		{
			lastEvent = currEvent;
			currEvent = currEvent->next;
		}
		lastEvent->next = newEvent;
	} else
	{
		// this is the first event
		x->events = newEvent;
	}
	post("event added");
}

// called when a measure ends
void end_measure(t_rhythms_memory *x)
{
	float fduration;
	event *currEvent, *lastEvent;
	unsigned short int is_it_a_new_rhythm;
	unsigned short int id, subid;
	float root_closeness, sub_closeness;
	int counter;
	t_atom *lista;
	// these 2 are for output rhythm
	int rhythm_found;
	t_rhythm_event *wanted_rhythm;
	t_rhythm_event *curr_rhythm;
	event *lastOutEvent;

	// I call the pd functions to get a representation
	// of this very moment
	x->measure_length = clock_gettimesince(x->measure_start_time);
	// now that i know the exact length of the current measure
	// i can process the rhythm in the current measure
	// and evaluate it
	currEvent = x->events;
	// this is not the first event
	// now I translate events in rhythm
	counter = 0;
	while(currEvent)
	{
		fduration = (float) (((float) currEvent->when) / ((float) x->measure_length));
		if (x->seq_initialized)
		{
			concatenateBeat(x->curr_seq, currEvent->voice, fduration, 1);
		} else
		{
			setFirstBeat(&(x->curr_seq), currEvent->voice, fduration, 1);
			x->seq_initialized = 1;
		}
		currEvent = currEvent->next;
		counter++;
	}
	
	// delete events after having evaluated them
	currEvent = x->events;
	// this is not the first event
	while(currEvent)
	{
		lastEvent = currEvent;
		currEvent = currEvent->next;
		free(lastEvent);
	}
	x->events = 0;

	if (x->curr_seq)
	{
		// now I evaluate this rhythm with the memory
		rhythm_memory_evaluate(x->rhythms_memory, x->curr_seq, &is_it_a_new_rhythm,
								&id, &subid, &root_closeness, &sub_closeness);
		// tell out the answer
		// allocate space for the list
		lista = (t_atom *) malloc(sizeof(t_atom) * 5);
		SETFLOAT(lista, (float) is_it_a_new_rhythm);
		SETFLOAT(lista+1, (float) id);
		SETFLOAT(lista+2, (float) subid);
		SETFLOAT(lista+3, (float) root_closeness);
		SETFLOAT(lista+4, (float) sub_closeness);
		outlet_anything(x->info_out,
						gensym("list") ,
						5, 
						lista);
		free(lista);
		// rhythm_memory_evaluate freed the memory for the rhythm if needed
		x->seq_initialized = 0;
		x->curr_seq = 0;
	}

	// I free the list of events_out (if present)
	currEvent = x->events_out;
	// this is not the first event
	while(currEvent)
	{
		lastEvent = currEvent;
		currEvent = currEvent->next;
		free(lastEvent);
	}
	x->events_out = 0;
	// i set up the list of events_out
	// for the wanted rhythm
	if (x->next_main_rhythm_out)
	{
		wanted_rhythm = 0;
		// ask the memory for the wanted rhythm
		rhythm_found = rhythm_memory_get_rhythm(x->rhythms_memory, // the memory
								&wanted_rhythm, // a pointer to the returned rhythm
								// the id of the main rhythm wanted
								x->next_main_rhythm_out, 
								// the sub-id of the sub-rhythm wanted
								x->next_sub_rhythm_out);
		if (rhythm_found==0)
		{
			post("rhythms_memory: rhythm %i %i was not found ", x->next_main_rhythm_out, x->next_sub_rhythm_out);
			return;
		}

		if (wanted_rhythm==0)
		{
			error("rhythms_memory: wanted_rhythm should not be null! ");
			return;
		}

		// now I setup the events_out list
		// for each event in wanted_rhythm
		// allocate and link an element of elements_out
		curr_rhythm = wanted_rhythm;
		lastOutEvent = 0;
		while (curr_rhythm)
		{
			event *newEvent;
			newEvent = malloc(sizeof(event));
			newEvent->next = 0;
			newEvent->voice = curr_rhythm->voice;
			newEvent->when = (double) (duration2float(curr_rhythm->start) * x->measure_length);
			post("DEBUG: add event in moment: %f", newEvent->when);
			if (x->events_out)
			{
				// this is not the first event
				// assign the next
				lastOutEvent->next = newEvent;
			} else
			{
				// this is the first event
				x->events_out = newEvent;
			}
			// change the last pointer
			lastOutEvent = newEvent;
			curr_rhythm = curr_rhythm->next;
		}

		// also setup the timer for the first event
		if (x->events_out)
		{
			// setup the clock
			clock_delay(x->x_clock, x->events_out->when);
			// remember when next event will occour
			x->last_event_out_time = x->events_out->when;
			// remember the curr event
			lastOutEvent = x->events_out;
			//reassign next event
			x->events_out = x->events_out->next;
			// free old event
			free(lastOutEvent);
		}
		x->next_main_rhythm_out = 0;

	}

	// also start the new measure!
	start_measure(x);

	
}

// this function is called  by pd
// when the timer bangs
static void rhythms_tick(t_rhythms_memory *x)
{
	event *lastOutEvent;
    // here i must:
	// take the next element in x->events_out
	// and compute when I'll need to schedule next event
	// (next_event - curr_event)
	// set the next element as the current one
	// and free the memory allocated for the old curr event
	// set up the timer	
	post("DEBUG: eveng bang");
	// first of all trigger the bang!
	outlet_bang(x->bangs_out);
	//then do the stuff
	if (x->events_out)
	{
		// setup the clock
		clock_delay(x->x_clock, x->events_out->when - x->last_event_out_time);
		// remember when next event will occour
		x->last_event_out_time = x->events_out->when ;
		// remember the curr event
		lastOutEvent = x->events_out;
		//reassign next event
		x->events_out = x->events_out->next;
		// free old event
		free(lastOutEvent);
	}
}

// the user wants me to play a rhythm in the next measure
// the user MUST pass 2 args: main_rhythm and sub_rhythm wanted
static void ask_rhythm(t_rhythms_memory *x, t_symbol *s, int argc, t_atom *argv)
{
	if (argc<2)
	{
		error("this method needs at least 2 floats: main_rhythm and sub_rhythm wanted");
		return;
	}
	//argv++;
	x->next_main_rhythm_out = atom_getfloat(argv++);
	x->next_sub_rhythm_out = atom_getfloat(argv);
	post("DEBUG: asked rhythm %i %i", x->next_main_rhythm_out, x->next_sub_rhythm_out); 
	// i have nothing left to do:
	// when this measure will end a list of events_out will be set
	// from the current values of x->next_main_rhythm_out and x->next_sub_rhythm_out
}

// add this rhythm to the memory
static void add_rhythm(t_rhythms_memory *x, t_symbol *s, int argc, t_atom *argv)
{
 // TODO
	post("TODO");
}

// creates a variation of a given rhythm (in memory)
// with a given degree of closeness
static void variation(t_rhythms_memory *x, t_symbol *s, int argc, t_atom *argv)
{
 // TODO
	post("TODO");

	// get the rhythm

	// using the transitions table create a new one

	// add it to the memory?

	// output to the list outlet?

	// set it as the next played rhythm
}

static void rhythms_memory_bang(t_rhythms_memory *x) {

	// generate a random value
	float rnd;
	t_rhythm_event *events;
	t_duration dur;

	rnd = rand()/((double)RAND_MAX + 1);
	dur = float2duration(rnd);

	post("random value=%f duration.numerator=%i duration.denominator=%i", rnd, dur.numerator, dur.denominator);

	if (x->seq_initialized)
	{
		concatenateBeat(x->curr_seq, 0, rnd, 1);
	} else
	{
		setFirstBeat(&(x->curr_seq), 0, rnd, 1);
		x->seq_initialized = 1;
	}

	// print the sequence
	events = x->curr_seq;
	while(events)
	{
		post("event: numerator=%i, denominator=%i", events->duration.numerator, events->duration.denominator);
		events=events->next;
	}
	
}

void *rhythms_memory_new(t_symbol *s, int argc, t_atom *argv)
{
	int i;
	time_t a;
    t_rhythms_memory *x = (t_rhythms_memory *)pd_new(rhythms_memory_class);
	// first is for bangs (to let this external play in realtime
	//x->l_out = outlet_new(&x->x_obj, &s_list);
	x->bangs_out = outlet_new(&x->x_obj, gensym("bang"));
	// this outputs lists of events
	x->list_out = outlet_new(&x->x_obj, gensym("symbol"));
	// this outputs info on the last detected rhythm
	x->info_out = outlet_new(&x->x_obj, gensym("symbol"));

	// inlet for rhythms in the form of lists
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("list"), gensym("rhythm_in"));

	x->x_clock = clock_new(x, (t_method)rhythms_tick);
	x->seq_initialized = 0;

	rhythm_memory_create(&(x->rhythms_memory));
	start_measure(x);

    return (x);
}

// debugging function
void init(t_rhythms_memory *x)
{
	if (x->curr_seq)
		freeBeats(x->curr_seq);	
	if (x->rhythms_memory)
		rhythm_memory_free(x->rhythms_memory);
	x->seq_initialized = 0;
	rhythm_memory_create(&(x->rhythms_memory));
}

// debugging function
void crash(t_rhythms_memory *x)
{
	int *a;
	a = malloc(sizeof(int));
	a[99999999999] = 1;
}


void rhythms_memory_setup(void)
{
    rhythms_memory_class = class_new(gensym("rhythms_memory"), (t_newmethod)rhythms_memory_new,
        (t_method)rhythms_memory_free, sizeof(t_rhythms_memory), CLASS_DEFAULT, A_GIMME, 0);
    class_addbang(rhythms_memory_class, (t_method)rhythms_memory_bang);
	class_addmethod(rhythms_memory_class, (t_method)end_measure, gensym("measure"), 0);
	class_doaddfloat(rhythms_memory_class, (t_method)add_event);
	class_addmethod(rhythms_memory_class, (t_method)crash, gensym("crash"), 0);
	// the user asks for a rhythm
	class_addmethod(rhythms_memory_class, (t_method)ask_rhythm, gensym("rhythm_out"),
        A_GIMME, 0);
	// adds a rhythm passing it as a list of floats
	class_addmethod(rhythms_memory_class, (t_method)add_rhythm, gensym("rhythm_in"),
        A_GIMME, 0);
	// builds a variation of a given rhythm
	class_addmethod(rhythms_memory_class, (t_method)variation, gensym("variation"),
        A_GIMME, 0);
	class_addmethod(rhythms_memory_class, (t_method)init, gensym("init"), 0);
}


