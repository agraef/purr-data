#include "MSPd.h"


static t_class *click2float_class;


typedef struct _click2float
{
    
	t_object x_obj;
	float x_f;
	void *float_outlet;
	void *clock;
	double float_value;
} t_click2float;

#define OBJECT_NAME "click2float~"

void *click2float_new(void);
t_int *click2float_perform(t_int *w);
void click2float_dsp(t_click2float *x, t_signal **sp);
void click2float_tick(t_click2float *x) ;


void click2float_tilde_setup(void)
{
	click2float_class = class_new(gensym("click2float~"), (t_newmethod)click2float_new, NO_FREE_FUNCTION,sizeof(t_click2float), 0,0);
	CLASS_MAINSIGNALIN(click2float_class, t_click2float, x_f);
	class_addmethod(click2float_class, (t_method)click2float_dsp, gensym("dsp"), 0);
	potpourri_announce(OBJECT_NAME);
}

void click2float_tick(t_click2float *x)
{
    outlet_float(x->float_outlet,x->float_value);
}


void *click2float_new(void)
{

	t_click2float *x = (t_click2float *)pd_new(click2float_class);
	x->float_outlet = outlet_new(&x->x_obj, gensym("float"));
	x->clock = clock_new(x,(void *)click2float_tick);
	return x;
}

t_int *click2float_perform(t_int *w)
{
	t_click2float *x = (t_click2float *) (w[1]);
	t_float *in_vec = (t_float *)(w[2]);
	int n = (int) w[3];
    
	while( n-- ) {
		if(*in_vec){
			x->float_value = *in_vec;
			clock_delay(x->clock, 0);
		}
		*in_vec++;
	}
	return (w+4);
}

void click2float_dsp(t_click2float *x, t_signal **sp)
{
    dsp_add(click2float_perform, 3, x, sp[0]->s_vec,sp[0]->s_n);
}

