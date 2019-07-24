#include "MSPd.h"

static t_class *clickhold_class;

#define OBJECT_NAME "clickhold~"

typedef struct _clickhold
{
	t_object x_obj;
	float x_f;
	float hold_value;
} t_clickhold;

void *clickhold_new(void);
t_int *clickhold_perform(t_int *w);
void clickhold_dsp(t_clickhold *x, t_signal **sp);


void clickhold_tilde_setup(void)
{
	clickhold_class = class_new(gensym("clickhold~"), (t_newmethod)clickhold_new,
                                NO_FREE_FUNCTION,sizeof(t_clickhold), 0,0);
	CLASS_MAINSIGNALIN(clickhold_class, t_clickhold, x_f);
	class_addmethod(clickhold_class, (t_method)clickhold_dsp, gensym("dsp"), 0);
	potpourri_announce(OBJECT_NAME);
}

void *clickhold_new(void)
{
	t_clickhold *x = (t_clickhold *)pd_new(clickhold_class);
	outlet_new(&x->x_obj, gensym("signal"));
	x->hold_value = 0;
	return x;
}

t_int *clickhold_perform(t_int *w)
{
	t_clickhold *x = (t_clickhold *) (w[1]);
	t_float *in_vec = (t_float *)(w[2]);
	t_float *out_vec = (t_float *)(w[3]);
	int n = (int) w[4];
    
	float hold_value = x->hold_value;
	
	while( n-- ) {
		if(*in_vec){
			hold_value = *in_vec;
		}
		*in_vec++;
		*out_vec++ = hold_value;
        
	}
	x->hold_value = hold_value;
	return (w+5);
}

void clickhold_dsp(t_clickhold *x, t_signal **sp)
{
    dsp_add(clickhold_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

