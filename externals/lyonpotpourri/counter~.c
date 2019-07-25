#include "MSPd.h"

static t_class *counter_class;

#define OBJECT_NAME "counter~"

#define COUNTER_UP (1)
#define COUNTER_DOWN (-1)

typedef struct _counter
{
	t_object x_obj;
	float x_f;
	long current;
	long min;
	long max;
	short direction;	
} t_counter;

static void *counter_new(t_symbol *s, int argc, t_atom *argv);
static t_int *counter_perform(t_int *w);
static void counter_dsp(t_counter *x, t_signal **sp);
static void counter_setnext(t_counter *x, t_floatarg val);
static void counter_direction(t_counter *x, t_floatarg d);
static void counter_minmax(t_counter *x, t_floatarg min, t_floatarg max);
static void counter_version(t_counter *x);

void counter_tilde_setup(void)
{
    t_class *c;
	c = class_new(gensym("counter~"), (t_newmethod)counter_new,0,sizeof(t_counter), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_counter, x_f);
    class_addmethod(c, (t_method)counter_minmax,gensym("minmax"),A_FLOAT,A_FLOAT,0);
    class_addmethod(c, (t_method)counter_direction,gensym("direction"),A_FLOAT,0);
    class_addmethod(c, (t_method)counter_setnext,gensym("setnext"),A_FLOAT,0);
	class_addmethod(c, (t_method)counter_dsp, gensym("dsp"), 0);
    counter_class = c;
	potpourri_announce(OBJECT_NAME);
}

static void counter_setnext(t_counter *x, t_floatarg val)
{
	if( val < x->min || val > x->max)
		return;
	x->current = (long) val;
}

static void counter_direction(t_counter *x, t_floatarg d)
{
	if( (d != COUNTER_UP) && (d != COUNTER_DOWN) )
		return;
	x->direction = (short) d;
}

static void counter_minmax(t_counter *x, t_floatarg min, t_floatarg max)
{
	if(min < 1){
		return;
	}
	if(min >= max){
		return;
	}
	x->min = min;
	x->max = max;
}

static void *counter_new(t_symbol *s, int argc, t_atom *argv)
{
    float farg;
    t_symbol *fraud;
	t_counter *x = (t_counter *)pd_new(counter_class);
    fraud = s;
	outlet_new(&x->x_obj, gensym("signal"));
	x->direction = COUNTER_UP;
    farg = 1.0;
	atom_arg_getfloat(&farg,0,argc,argv);
    x->min = farg;
    farg = 10.0;
	atom_arg_getfloat(&farg,1,argc,argv);
    x->max = farg;
	if(x->min <= 1)
		x->min = 1;
	if(x->max <= x->min)
		x->max = 10;

	return x;
}

static t_int *counter_perform(t_int *w)
{
	t_counter *x = (t_counter *) (w[1]);
	t_float *in_vec = (t_float *)(w[2]);
	t_float *out_vec = (t_float *)(w[3]);
	int n = (int) w[4];
	
	int i;
	long min = x->min;
	long max = x->max;
	long current = x->current;
	short direction = x->direction;
	
	for(i = 0; i < n; i++){
		if(in_vec[i]){
			out_vec[i] = current;
			current = current + direction;
			if( direction == COUNTER_UP ){
				if( current > max ){
					current = min; 
				}
			} else if( direction == COUNTER_DOWN ){
				if( current < min ){
					current = max;
				}
			}
		} else {
			out_vec[i] = 0.0;
		}
	}
	
	x->current = current;
	return w + 5;
}		

static void counter_dsp(t_counter *x, t_signal **sp)
{
    dsp_add(counter_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}
