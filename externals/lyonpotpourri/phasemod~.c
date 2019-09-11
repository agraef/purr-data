#include "MSPd.h"
#define FUNC_LEN (32768)
#define OBJECT_NAME "phasemod~"


static t_class *phasemod_class;


typedef struct _phasemod
{
    
    t_object x_obj;
    float x_f;
    t_float x_val;
    float mygain;
    float *wavetab;
	float phs;
	float bendphs;
	float frequency;
	float alpha;
	short mute;
	short connections[4];
	float si_fac;
	float sr;
} t_phasemod;

void *phasemod_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *phasemod_perform(t_int *w);
void phasemod_float(t_phasemod *x, double f);
void phasemod_int(t_phasemod *x, long n);
void phasemod_mute(t_phasemod *x, t_floatarg toggle);
void phasemod_dsp(t_phasemod *x, t_signal **sp);
void phasemod_dsp_free(t_phasemod *x);

void phasemod_tilde_setup(void){
    phasemod_class = class_new(gensym("phasemod~"), (t_newmethod)phasemod_new,
                               (t_method)phasemod_dsp_free,sizeof(t_phasemod), 0,A_GIMME,0);
    CLASS_MAINSIGNALIN(phasemod_class, t_phasemod, x_f);
    class_addmethod(phasemod_class,(t_method)phasemod_dsp,gensym("dsp"),0);
    class_addmethod(phasemod_class,(t_method)phasemod_mute,gensym("mute"),A_FLOAT,0);
    potpourri_announce(OBJECT_NAME);
}

void phasemod_dsp_free( t_phasemod *x )
{
    free(x->wavetab);
}

void phasemod_mute(t_phasemod *x, t_floatarg toggle)
{
	x->mute = toggle;
}
void phasemod_assist (t_phasemod *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal/float) Frequency ");
				break;
			case 1:
				sprintf(dst,"(signal/float) Slope Factor ");
				break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output ");
	}
}

void *phasemod_new(t_symbol *s, int argc, t_atom *argv)
{
  	int i;
    t_phasemod *x = (t_phasemod *)pd_new(phasemod_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->phs = 0;
  	x->mute = 0;
    x->frequency = 440.0;
	
    x->wavetab = (float *) calloc(FUNC_LEN, sizeof(float) );
    for( i = 0 ; i < FUNC_LEN; i++ ) {
    	x->wavetab[i] = sin( TWOPI * ((float)i/(float) FUNC_LEN)) ;
    }
    x->bendphs = 0;
    x->sr = sys_getsr();
    if(!x->sr)
        x->sr = 44100.0;
    x->si_fac = 1.0/x->sr;
    return (x);
}

t_int *phasemod_perform(t_int *w)
{
    
	float phs;
	
	t_phasemod *x = (t_phasemod *) (w[1]);
	t_float *frequency_vec = (t_float *)(w[2]);
	t_float *alpha_vec = (t_float *)(w[3]);
	t_float *out = (t_float *)(w[4]);
	int n = (int) w[5];
	
	short *connections = x->connections;
	float bendphs = x->bendphs;
	float *wavetab = x->wavetab;
	float si_fac = x->si_fac;
	
	float incr = x->frequency * si_fac ;
	float alpha = x->alpha;
	
	if( x->mute ){
		while(n--){
			*out++ = 0.0;
		}
		return (w + 6);
	}
	
	while (n--) {
		if( connections[1] ){
			alpha = *alpha_vec++;
		}
		if( alpha == 0 ){
			alpha = .000001;
		}
		
		if( connections[0] ){
			incr = *frequency_vec++ * si_fac ;
		}
		// NO NEGATIVE FREQUENCIES
		if( incr < 0 )
			incr = -incr;
        
		bendphs += incr ;
		while( bendphs > 1.0 )
			bendphs -= 1.0 ;
        phs =   FUNC_LEN * ( (1 - exp(bendphs * alpha))/(1 - exp(alpha))  );
        
		while( phs < 0.0 ) {
			phs += FUNC_LEN;
		}
		while( phs >= FUNC_LEN ){
			phs -= FUNC_LEN;
		}
		*out++ =  wavetab[(int) phs] ;
	}
    
	x->bendphs = bendphs;
	return (w+6);
}

void phasemod_dsp(t_phasemod *x, t_signal **sp)
{

	x->connections[0] = 1;
	x->connections[1] = 1;
    
	if(x->sr != sp[0]->s_sr){
		if(!sp[0]->s_sr){
			error("zero sampling rate");
			return;
		}
		x->sr = sp[0]->s_sr;
		x->si_fac = 1.0/x->sr;
	}
	dsp_add(phasemod_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,  sp[0]->s_n);
}

