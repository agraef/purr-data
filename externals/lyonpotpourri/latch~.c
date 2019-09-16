#include "MSPd.h"

static t_class *latch_class;

#define OBJECT_NAME "latch~"
#define LATCH_WAITING 0
#define LATCH_RUNNING 1

typedef struct _latch
{
	t_object x_obj;
    t_float x_f;
	t_float current;
	long latch;
	long dsamps;
	long sr;
	t_float duration;
	long status;
	long connected;
} t_latch;

static void *latch_new(t_float duration);
static t_int *latch_perform(t_int *w);
static void latch_dsp(t_latch *x, t_signal **sp);

void latch_tilde_setup(void)
{
	latch_class = class_new(gensym("latch~"), (t_newmethod)latch_new, 0, sizeof(t_latch), 0,A_FLOAT, 0);
	CLASS_MAINSIGNALIN(latch_class, t_latch, x_f);
	class_addmethod(latch_class, (t_method)latch_dsp, gensym("dsp"), A_CANT, 0);
	potpourri_announce(OBJECT_NAME);
}

static void *latch_new(t_float duration)
{

	t_latch *x = (t_latch *)pd_new(latch_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));

	x->status = LATCH_WAITING;    
	x->duration = duration;
	x->sr = sys_getsr();
	if(x->duration <= 1.0)
		x->duration = 1.0;
	x->duration *= 0.001;
	x->dsamps = x->sr ? x->sr * x->duration : 44100;

	return x;
}

static t_int *latch_perform(t_int *w)
{
	t_latch *x = (t_latch *) (w[1]);
	t_float *trigger = (t_float *)(w[2]);
	t_float *vec_duration = (t_float *)(w[3]);
	t_float *output = (t_float *)(w[4]);
	int n = (int) w[5];
	int next_pointer = 6;
	long latch = x->latch;
	long dsamps = x->dsamps;
	long status = x->status;
	t_float current = x->current;
	t_float duration = x->duration;
	long sr = x->sr;
	long connected = x->connected;
	int i = 0;

	

	for(i = 0; i < n; i++){
        
		if(connected){
			duration = vec_duration[i];
			if( duration > 0.0 ){ // dummy proof
				dsamps = sr * duration * 0.001;
			}
		}
        
		if(trigger[i]){
			latch = 0;
			status = LATCH_RUNNING;
			current = trigger[i];
		} else {
			latch++;
			if(latch >= dsamps){
				status = LATCH_WAITING;
				current = 0.0;
			}
		}
		output[i] = current;
     
	}
    
	
	x->current = current;
	x->status = status;
	x->latch = latch;
	x->dsamps = dsamps;
	return w + next_pointer;
}		

static void latch_dsp(t_latch *x, t_signal **sp)
{
	x->connected = 1;
	if(x->sr != sp[0]->s_sr){
		x->sr = sp[0]->s_sr;
		x->dsamps = x->duration * x->sr;
	}
    dsp_add(latch_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, (t_int)sp[0]->s_n);
}
