#include "MSPd.h"


static t_class *distortion_class;


#define OBJECT_NAME "distortion~"

typedef struct _distortion
{
    
    t_object x_obj;
    float x_f;
	float knee;
	float cut;
	float rescale ;
	short mute ;
	short case1;
} t_distortion;

void *distortion_new(t_floatarg knee, t_floatarg cut);
t_int *distortion1_perform(t_int *w);
t_int *distortion2_perform(t_int *w);
t_int *distortion3_perform(t_int *w);
void distortion_dsp(t_distortion *x, t_signal **sp);
void distortion_float(t_distortion *x, double f);
void distortion_mute(t_distortion *x, t_floatarg f);


// no freeing function needed

void distortion_tilde_setup(void){
    distortion_class = class_new(gensym("distortion~"), (t_newmethod)distortion_new,
                                 0,sizeof(t_distortion), 0,A_DEFFLOAT,A_DEFFLOAT,0);
    CLASS_MAINSIGNALIN(distortion_class, t_distortion, x_f);
    class_addmethod(distortion_class,(t_method)distortion_dsp,gensym("dsp"),0);
    class_addmethod(distortion_class,(t_method)distortion_mute,gensym("mute"),A_FLOAT,0);
    potpourri_announce(OBJECT_NAME);
}

void *distortion_new(t_floatarg knee, t_floatarg cut)
{

    t_distortion *x = (t_distortion *)pd_new(distortion_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
	if( knee >= cut || knee <= 0 || cut <= 0 ) {
		// post("setting defaults");
		x->knee = .1;
		x->cut = .3 ;
	} else {
		x->knee = knee;
		x->cut = cut;
		// post("User defined values: knee %f cut %f", knee, cut);
        
	}
	x->rescale = 1.0 / x->cut ;
	x->mute = 0;
    return x;
}


// use when neither signal is connected

t_int *distortion1_perform(t_int *w)
{
    
	float rectified_sample, in_sample;
    
	t_distortion *x = (t_distortion *) (w[1]);
	float *in = (t_float *)(w[2]);
	float *out = (t_float *)(w[5]);
	int n = (int)(w[6]);
	
	float knee = x->knee;
	float cut = x->cut;
	float rescale = x->rescale;
    
	
	if( x->mute ){
		while( n-- ){
			*out++ = 0;
		}
		return (w+7);
	}
	
	while (n--) {
		in_sample = *in++;
		rectified_sample = fabs( in_sample );
		if( rectified_sample < knee ){
			*out++ = in_sample;
		} else {
			if( in_sample > 0.0 ){
				*out++ = rescale * (knee + (rectified_sample - knee) * (cut - knee));
			} else {
				*out++ = rescale * (-(knee + (rectified_sample - knee) * (cut - knee)));
			}
		}
        
	}
	return (w+7);
}

// use when both signals are connected

t_int *distortion2_perform(t_int *w)
{
    
	float rectified_sample, in_sample;
    
	t_distortion *x = (t_distortion *) (w[1]);
	float *in = (t_float *)(w[2]);
	float *data1 = (t_float *)(w[3]);
	float *data2 = (t_float *)(w[4]);
	float *out = (t_float *)(w[5]);
	int n = (int)(w[6]);
    //	double fabs();
    
	float knee = x->knee;
	float cut = x->cut;
	float rescale = x->rescale;
    
	
	if( x->mute ){
		while( n-- ){
			*out++ = 0;
		}
		return (w+7);
	}
	
	while (n--) {
		in_sample = *in++;
		knee = *data1++;
		cut = *data2++;
		if( cut > 0.000001 )
			rescale = 1.0 / cut;
		else
			rescale = 1.0;
		
		rectified_sample = fabs( in_sample );
		if( rectified_sample < knee ){
			*out++ = in_sample;
		} else {
			if( in_sample > 0.0 ){
				*out++ = rescale * (knee + (rectified_sample - knee) * (cut - knee));
			} else {
				*out++ = rescale * (-(knee + (rectified_sample - knee) * (cut - knee)));
			}
		}
        
	}
	x->knee = knee;
	x->cut = cut;
	x->rescale = rescale;
	return (w+7);
}

t_int *distortion3_perform(t_int *w)
{
    
	float rectified_sample, in_sample;
    
	t_distortion *x = (t_distortion *) (w[1]);
	float *in = (t_float *)(w[2]);
	float *data1 = (t_float *)(w[3]);
	float *data2 = (t_float *)(w[4]);
	float *out = (t_float *)(w[5]);
	int n = (int)(w[6]);
    //	double fabs();
    
	float knee = x->knee;
	float cut = x->cut;
	float rescale = x->rescale;
	short case1 = x->case1;
	
	if( x->mute ){
		while( n-- ){
			*out++ = 0;
		}
		return (w+7);
	}
	
	while (n--) {
        // first case, knee is connected, otherwise cut is connected
		in_sample = *in++;
		if( case1 ){
			knee = *data1++;
		}
		else {
			cut = *data2++;
		}
		if( cut > 0.000001 )
			rescale = 1.0 / cut;
		else
			rescale = 1.0;
		
		rectified_sample = fabs( in_sample );
		if( rectified_sample < knee ){
			*out++ = in_sample;
		} else {
			if( in_sample > 0.0 ){
				*out++ = rescale * (knee + (rectified_sample - knee) * (cut - knee));
			} else {
				*out++ = rescale * (-(knee + (rectified_sample - knee) * (cut - knee)));
			}
		}
        
	}
	x->knee = knee;
	x->cut = cut;
	x->rescale = rescale;
	return (w+7);
}
void distortion_mute(t_distortion *x, t_floatarg f) {
	x->mute = f;
}


void distortion_dsp(t_distortion *x, t_signal **sp)
{

	dsp_add(distortion2_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,(t_int)sp[0]->s_n);

}

