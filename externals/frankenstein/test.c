/* 
just a dummy test patch

*/

#include "m_pd.h"

#include "common.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>

static t_class *test_class;



typedef struct _test
{
    t_object x_obj; // myself
	t_outlet *l_out;
	t_rhythm_event *curr_seq;
	int seq_initialized;
	t_rhythm_memory_representation *rhythms_memory;
	
} t_test;

void test_free(t_test *x)
{
	freeBeats(x->curr_seq);
}

static void test_bang(t_test *x) {

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

void *test_new(t_symbol *s, int argc, t_atom *argv)
{
	int i;
	time_t a;
    t_test *x = (t_test *)pd_new(test_class);
	x->l_out = outlet_new(&x->x_obj, &s_list);
	
	x->seq_initialized = 0;

	rhythm_memory_create(&(x->rhythms_memory));

    return (x);
}

void test_setup(void)
{
    test_class = class_new(gensym("test"), (t_newmethod)test_new,
        (t_method)test_free, sizeof(t_test), CLASS_DEFAULT, A_GIMME, 0);
    class_addbang(test_class, (t_method)test_bang);
	

}
