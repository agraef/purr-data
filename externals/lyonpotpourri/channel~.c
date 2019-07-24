#include "MSPd.h"

static t_class *channel_class;


typedef struct _channel
{
    
	t_object x_obj;
	float x_f;
	void *float_outlet;
	int channel;
    
	
} t_channel;

#define OBJECT_NAME "channel~"

void *channel_new(t_symbol *s, int argc, t_atom *argv);

t_int *channel_perform(t_int *w);
void channel_dsp(t_channel *x, t_signal **sp);
void channel_channel(t_channel *x, t_floatarg chan) ;
void channel_int(t_channel *x, long chan) ;

#define NO_FREE_FUNCTION 0
void channel_tilde_setup(void)
{
	channel_class = class_new(gensym("channel~"), (t_newmethod)channel_new,
                              NO_FREE_FUNCTION,sizeof(t_channel), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(channel_class, t_channel, x_f);
	class_addmethod(channel_class, (t_method)channel_dsp, gensym("dsp"), 0);
	class_addmethod(channel_class,(t_method)channel_channel,gensym("channel"),A_FLOAT,0);
	potpourri_announce(OBJECT_NAME);
}

void channel_channel(t_channel *x, t_floatarg chan)
{
	if(chan >= 0)
		x->channel = (int) chan;
}


void channel_int(t_channel *x, long chan)
{
	if(chan >= 0)
		x->channel = (int) chan;
}

void *channel_new(t_symbol *s, int argc, t_atom *argv)
{
    
	t_channel *x = (t_channel *)pd_new(channel_class);
	outlet_new(&x->x_obj, gensym("signal"));
    
	x->channel = (int)atom_getfloatarg(0,argc,argv);
	return (x);
}

t_int *channel_perform(t_int *w)
{
	t_channel *x = (t_channel *) (w[1]);
	t_float *in_vec = (t_float *)(w[2]);
	t_float *out_vec = (t_float *)(w[3]);
	int n = (int) w[4];
	int channel = x->channel;
	float value;
	
	if(channel < 0 || channel > n){
		return w + 5;
	}
	value = in_vec[channel];
	
	while( n-- ) {
		*out_vec++ = value;
	}
	return w + 5;
}

void channel_dsp(t_channel *x, t_signal **sp)
{
    dsp_add(channel_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

