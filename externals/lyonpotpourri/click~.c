#include "MSPd.h"

static t_class *click_class;

typedef struct _click
{
	t_object x_obj;
	float x_f;
	void *float_outlet;
	float float_value;	
	long status;
} t_click;

#define OBJECT_NAME "click~"

void *click_new(t_symbol *s, int argc, t_atom *argv);
void click_bang(t_click *x);
t_int *click_perform(t_int *w);
void click_dsp(t_click *x, t_signal **sp);
void click_set(t_click *x, t_floatarg f);

#define NO_FREE_FUNCTION 0
void click_tilde_setup(void)
{
	click_class = class_new(gensym("click~"), (t_newmethod)click_new, 
								 NO_FREE_FUNCTION,sizeof(t_click), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(click_class, t_click, x_f);
	class_addmethod(click_class, (t_method)click_dsp, gensym("dsp"), 0);
	class_addmethod(click_class, (t_method)click_bang, gensym("bang"), 0);
	class_addmethod(click_class, (t_method)click_set, gensym("set"), A_FLOAT, 0);
	potpourri_announce(OBJECT_NAME);
}

void click_bang(t_click *x) 
{
  x->status = 1;
}

void click_set(t_click *x, t_floatarg f)
{
	x->float_value = f;
}

void *click_new(t_symbol *s, int argc, t_atom *argv)
{
	t_click *x = (t_click *)pd_new(click_class);
	x->float_outlet = outlet_new(&x->x_obj, gensym("signal"));
	x->float_value = 1.0;
	x->status = 0;
	return x;
}

t_int *click_perform(t_int *w)
{
	t_click *x = (t_click *) (w[1]);
	t_float *output = (t_float *)(w[2]);
	int n = (int) w[3];
	int i;
	if(x->status){
		x->status = 0;
		output[0] = x->float_value;
		for(i = 1; i < n; i++){
			output[i] = 0.0;
		}
	} 
	else {
		for(i = 0; i < n; i++){
			output[i] = 0.0;
		}
	}
	return w+4;
}		

void click_dsp(t_click *x, t_signal **sp)
{
    dsp_add(click_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

