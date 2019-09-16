#include "MSPd.h"

#define OBJECT_NAME "click2bang~"

static t_class *click2bang_class;

typedef struct _click2bang
{
	t_object x_obj;
	float x_f;
	void *bang;
	void *clock;
} t_click2bang;

void *click2bang_new(void);

t_int *click2bang_perform(t_int *w);
void click2bang_dsp(t_click2bang *x, t_signal **sp);
void click2bang_tick(t_click2bang *x) ;

void click2bang_tilde_setup(void)
{
	click2bang_class = class_new(gensym("click2bang~"), (t_newmethod)click2bang_new,
								 NO_FREE_FUNCTION,sizeof(t_click2bang), 0,0);
	CLASS_MAINSIGNALIN(click2bang_class, t_click2bang, x_f);
	class_addmethod(click2bang_class, (t_method)click2bang_dsp, gensym("dsp"), 0);
	potpourri_announce(OBJECT_NAME);
}


void click2bang_tick(t_click2bang *x)
{
    outlet_bang(x->bang);
}



void *click2bang_new(void)
{
    
	t_click2bang *x = (t_click2bang *)pd_new(click2bang_class);
	x->bang = outlet_new(&x->x_obj, gensym("bang"));
	x->clock = clock_new(x,(void *)click2bang_tick);
	return x;
}

t_int *click2bang_perform(t_int *w)
{
	t_click2bang *x = (t_click2bang *) (w[1]);
	t_float *in_vec = (t_float *)(w[2]);
	int n = (int) w[3];
    
	while( n-- ) {
		if(*in_vec++)
			clock_delay(x->clock, 0);
	}
	return (w+4);
}

void click2bang_dsp(t_click2bang *x, t_signal **sp)
{
    dsp_add(click2bang_perform, 3, x, sp[0]->s_vec,(t_int)sp[0]->s_n);
}

