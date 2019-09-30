#include "MSPd.h"
/* internal metronome now redundant so disabled */
// LyonPotpourri 3.0 - Max references removed


static t_class *adsr_class;

#define OBJECT_NAME "adsr~"

typedef struct _adsr
{
	t_object x_obj;
	float x_f;
	// Variables Here
	float a;
	float d;
	float s;
	float r;
	int ebreak1;
	int ebreak2;
	int ebreak3;
	int asamps;
	int dsamps;
	int ssamps;
	int rsamps;
	int asamps_last;
	int dsamps_last;
	int ssamps_last;
	int rsamps_last;
	float tempo;
	float egain1;
	float egain2;
	int tempomode;
	int beat_subdiv;
	int tsamps;
	int counter;
	float srate;
	short manual_override;
	float click_gain; // input click sets volume too
	short mute;
} t_adsr;

static void *adsr_new(t_symbol *s, int argc, t_atom *argv);

t_int *adsr_perform(t_int *w);
void adsr_dsp(t_adsr *x, t_signal **sp);
void adsr_assist(t_adsr *x, void *b, long m, long a, char *s);
void adsr_bang(t_adsr *x);
void adsr_manual_override(t_adsr *x, t_floatarg toggle);
void adsr_list (t_adsr *x, t_atom *msg, short argc, t_atom *argv);
void adsr_tempomode(t_adsr *x, t_atom *msg, short argc, t_atom *argv);
void adsr_set_a(t_adsr *x, t_floatarg f);
void adsr_set_d(t_adsr *x, t_floatarg f);
void adsr_set_s(t_adsr *x, t_floatarg f);
void adsr_set_r(t_adsr *x, t_floatarg f);
void adsr_set_gain1(t_adsr *x, t_floatarg f);
void adsr_set_gain2(t_adsr *x, t_floatarg f);
void set_tempo(t_adsr *x, t_floatarg f);
void adsr_mute(t_adsr *x, t_floatarg f);

void atom_arg_getfloat(float *c, long idx, long ac, t_atom *av);
void atom_arg_getsym(t_symbol **c, long idx, long ac, t_atom *av);

void adsr_tilde_setup(void){
	adsr_class = class_new(gensym("adsr~"), (t_newmethod)adsr_new, 
						   0,sizeof(t_adsr), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(adsr_class, t_adsr, x_f);
	class_addmethod(adsr_class,(t_method)adsr_dsp,gensym("dsp"),0);
	class_addmethod(adsr_class,(t_method)adsr_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(adsr_class,(t_method)adsr_list,gensym("list"),A_GIMME,0);
	class_addmethod(adsr_class,(t_method)adsr_set_a,gensym("set_a"),A_FLOAT,0);
	class_addmethod(adsr_class,(t_method)adsr_set_d,gensym("set_d"),A_FLOAT,0);
	class_addmethod(adsr_class,(t_method)adsr_set_s,gensym("set_s"),A_FLOAT,0);
	class_addmethod(adsr_class,(t_method)adsr_set_r,gensym("set_r"),A_FLOAT,0);
	class_addmethod(adsr_class,(t_method)adsr_set_gain1,gensym("set_gain1"),A_FLOAT,0);
	class_addmethod(adsr_class,(t_method)adsr_set_gain2,gensym("set_gain2"),A_FLOAT,0);
	class_addbang(adsr_class,(t_method)adsr_bang);
	potpourri_announce(OBJECT_NAME);
}


void adsr_mute(t_adsr *x, t_floatarg f)
{
    x->mute = (short)f;
}

void adsr_set_gain1(t_adsr *x, t_floatarg f)
{
	x->egain1 = f;
	return;
}

void adsr_set_gain2(t_adsr *x, t_floatarg f)
{
	x->egain2 = f;
	return;
}
void adsr_bang(t_adsr *x) {
	x->counter = 0;
	return;
}
void adsr_set_a(t_adsr *x, t_floatarg f)
{
	
	f /= 1000.0;
	
	x->a = f;
	x->asamps = x->a * x->srate;
	
	if( x->tempomode) {
		x->rsamps = x->tsamps - (x->asamps+x->dsamps+x->ssamps);	
		if( x->rsamps < 0 ) {
			x->rsamps = 0;
		}
	} else {	
		x->tsamps = x->asamps+x->dsamps+x->ssamps+x->rsamps;
	}
	x->ebreak1 = x->asamps;
	x->ebreak2 = x->asamps+x->dsamps;
	x->ebreak3 = x->asamps+x->dsamps+x->ssamps;
	return ;
}

void adsr_set_d(t_adsr *x, t_floatarg f)
{
	f /= 1000.0 ;
	
	x->d = f;
	x->dsamps = x->d * x->srate;
	
	if( x->tempomode) {
		x->rsamps = x->tsamps - (x->asamps+x->dsamps+x->ssamps);	
		if( x->rsamps < 0 ) {
			x->rsamps = 0;
		}
	} else {	
		x->tsamps = x->asamps+x->dsamps+x->ssamps+x->rsamps;
	}
	x->ebreak2 = x->asamps+x->dsamps;
	x->ebreak3 = x->asamps+x->dsamps+x->ssamps;
	return ;
}

void adsr_set_s(t_adsr *x, t_floatarg f)
{
	
	f /= 1000.0;
	
	x->s = f;
	x->ssamps = x->s * x->srate;
	
	if( x->tempomode) {
		x->rsamps = x->tsamps - (x->asamps+x->dsamps+x->ssamps);	
		if( x->rsamps < 0 ) {
			x->rsamps = 0;
		}
	} else {	
		x->tsamps = x->asamps+x->dsamps+x->ssamps+x->rsamps;
	}
	
	x->ebreak3 = x->asamps+x->dsamps+x->ssamps;
	return ;
}

void adsr_set_r(t_adsr *x, t_floatarg f)
{
	
	f /= 1000.0;
	
	if( x->tempomode) {
		return;
	} else {	
		x->r = f;
		x->rsamps = x->r * x->srate;		
		x->tsamps = x->asamps+x->dsamps+x->ssamps+x->rsamps;
	}
	
	return ;
}

void adsr_list (t_adsr *x, t_atom *msg, short argc, t_atom *argv)
{
    t_atom *fraud; // make compiler happy
    fraud = msg;
	x->rsamps = x->tsamps - (x->asamps+x->dsamps+x->ssamps);	
	if( x->rsamps < 0 ) 
		x->rsamps = 0;
	
	x->a = (atom_getfloatarg(0,argc,argv)) * .001;
	x->d = (atom_getfloatarg(1,argc,argv)) * .001;
	x->s = (atom_getfloatarg(2,argc,argv)) * .001;
	x->r = (atom_getfloatarg(3,argc,argv)) * .001;
	
	x->asamps = x->a * x->srate;
	x->dsamps = x->d * x->srate;
	x->ssamps = x->s * x->srate;
	x->rsamps = x->r * x->srate;
	
	
    x->tsamps = x->asamps+x->dsamps+x->ssamps+x->rsamps;
    x->ebreak1 = x->asamps;
    x->ebreak2 = x->asamps+x->dsamps;
    x->ebreak3 = x->asamps+x->dsamps+x->ssamps;
	
}

static void *adsr_new(t_symbol *s, int argc, t_atom *argv)
{

	t_adsr *x = (t_adsr *)pd_new(adsr_class);
    t_symbol *fraud; // make compiler happy
    fraud = s;
	outlet_new(&x->x_obj, gensym("signal"));
	
   
	x->srate = sys_getsr();
	if(!x->srate){
		error("zero sampling rate, setting to 44100");
		x->srate = 44100;
	}
	
	x->a = 10;
	x->d = 50;
	x->s = 100;
	x->r = 100;
	x->egain1 = .7;
	x->egain2 = .1;
	atom_arg_getfloat(&x->a,0,argc,argv);
	atom_arg_getfloat(&x->d,1,argc,argv);
	atom_arg_getfloat(&x->s,2,argc,argv);
	atom_arg_getfloat(&x->r,3,argc,argv);
	atom_arg_getfloat(&x->egain1,4,argc,argv);
	atom_arg_getfloat(&x->egain2,5,argc,argv);

	x->a *= .001;
	x->d *= .001;
	x->s *= .001;
	x->r *= .001;
	
	x->asamps = x->a * x->srate;
	x->dsamps = x->d * x->srate;
	x->ssamps = x->s * x->srate;
	x->rsamps = x->r * x->srate;
	x->tsamps = x->asamps+x->dsamps+x->ssamps+x->rsamps;
	x->ebreak1 = x->asamps;
	x->ebreak2 = x->asamps+x->dsamps;
	x->ebreak3 = x->asamps+x->dsamps+x->ssamps;

	x->counter = 0;
	x->click_gain = 0.0;
	x->mute = 0;
	return x;
}

t_int *adsr_perform(t_int *w)
{
	t_adsr *x = (t_adsr *) (w[1]);
	t_float *in = (t_float *)(w[2]);
	t_float *out = (t_float *)(w[3]);
	int  n = (int) w[4];
	int tsamps = x->tsamps;
	int counter = x->counter;
	int ebreak1 = x->ebreak1;
	int ebreak2 = x->ebreak2;
	int ebreak3 = x->ebreak3;
	float egain1 = x->egain1;
	float egain2 = x->egain2;
	int asamps = x->asamps;
	int dsamps = x->dsamps;
	int ssamps = x->ssamps;
	int rsamps = x->rsamps;
	//  short manual_override = x->manual_override;
	float click_gain = x->click_gain;
	float etmp;
	float env_val;
	float input_val;
	/*********************************************/	
	if(x->mute){
		while(n--) *out++ = 0.0; 
		return w+5;
	}
	while(n--) {
		input_val = *in++;
		if(input_val){
			click_gain = input_val;
			counter = 0;
		}
		
		
		if( counter < ebreak1 ){
			env_val = (float) counter / (float) asamps;
		} else if (counter < ebreak2) {
			etmp = (float) (counter - ebreak1) / (float) dsamps;
			env_val = (1.0 - etmp) + (egain1 * etmp);
		} else if (counter < ebreak3) {
			etmp = (float) (counter - ebreak2) / (float) ssamps;
			env_val = (egain1 * (1.0 - etmp)) + (egain2 * etmp);
		} else if( counter < tsamps ){
			env_val = ((float)(tsamps-counter)/(float)rsamps) * egain2 ;
		} else {
			env_val = 0.0;
		}
		if(click_gain && env_val && (click_gain != 1.0) ){
			env_val *= click_gain;
		}
		*out++ = env_val;
		if(counter < tsamps)
			counter++;
	}
	x->counter = counter;
	x->click_gain = click_gain;
	return (w+5);
}		

void adsr_dsp(t_adsr *x, t_signal **sp)
{
	if(x->srate != sp[0]->s_sr ){
		x->srate = sp[0]->s_sr;
		x->asamps = x->a * x->srate;
		x->dsamps = x->d * x->srate;
		x->ssamps = x->s * x->srate;
		x->rsamps = x->r * x->srate;
		x->tsamps = x->asamps+x->dsamps+x->ssamps+x->rsamps;
		x->ebreak1 = x->asamps;
		x->ebreak2 = x->asamps+x->dsamps;
		x->ebreak3 = x->asamps+x->dsamps+x->ssamps;
		x->counter = 0;	
	}
	dsp_add(adsr_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}




