/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *cavoc27_class;

#define OBJECT_NAME "cavoc27~"

/* NOTE THIS IS A MORE COMPLEX CA WITH 3 DIFFERENT STATES  */

typedef struct _cavoc27
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_float *ichannel; //for interpolation
	t_float *tmpchannel; // for spectrum capture
	t_float frame_duration;
	int max_bin;
	t_float fundamental;
	t_float *last_frame;
	short left;
	short right;
	short center;
	short *rule;
	
	t_float start_breakpoint;
	int hold_frames;
	int frames_left;
	int set_count;
	short interpolate_flag;
	short capture_flag;
	short capture_lock;
	void *list_outlet;
	t_atom *list_data;
	short mute;
	
	short external_trigger;
	short trigger_value; // set to 1 when a bang is received
	t_float topfreq; // highest to synthesize - Nyquist by default
	t_float bottomfreq;
	t_float *freqs;
	t_float *amps;
	short manual_mode; // respond to outside
	short freeze; // flag to maintain current spectrum
	long fftsize_attr;
	long overlap_attr;
    t_float density;
    t_float hold_time; //hold time in seconds
} t_cavoc27;

void *cavoc27_new(t_symbol *s, int argc, t_atom *argv);
void cavoc27_dsp(t_cavoc27 *x, t_signal **sp);
t_int *cavoc27_perform(t_int *w);
void cavoc27_assist(t_cavoc27 *x, void *b, long m, long a, char *s);
void cavoc27_free( t_cavoc27 *x);
int cavoc27_apply_rule( short left, short right, short center, short *rule);
void cavoc27_rule (t_cavoc27 *x, t_symbol *msg, short argc, t_atom *argv);
void cavoc27_density (t_cavoc27 *x, t_floatarg density);
void cavoc27_hold_time (t_cavoc27 *x, t_floatarg hold_time);
void cavoc27_interpolate (t_cavoc27 *x, t_floatarg interpolate);
void cavoc27_capture_spectrum (t_cavoc27 *x, t_floatarg flag );
void cavoc27_capture_lock (t_cavoc27 *x, t_floatarg toggle );
void cavoc27_retune (t_cavoc27 *x, t_floatarg min, t_floatarg max);
void cavoc27_mute (t_cavoc27 *x, t_floatarg toggle);
void cavoc27_init(t_cavoc27 *x);
void cavoc27_rand_set_spectrum(t_cavoc27 *x);
void cavoc27_rand_set_rule(t_cavoc27 *x);
void cavoc27_fftinfo(t_cavoc27 *x);
void cavoc27_oscbank(t_cavoc27 *x, t_floatarg flag);
void cavoc27_transpose (t_cavoc27 *x, t_floatarg pfac);
void cavoc27_noalias(t_cavoc27 *x, t_floatarg flag);
void cavoc27_manual(t_cavoc27 *x, t_floatarg tog);
void cavoc27_trigger(t_cavoc27 *x);
void cavoc27_freeze(t_cavoc27 *x, t_floatarg tog);

void cavoc27_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("cavoc27~"), (t_newmethod)cavoc27_new,
                  (t_method)cavoc27_free,sizeof(t_cavoc27), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_cavoc27, x_f);
	class_addmethod(c,(t_method)cavoc27_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)cavoc27_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc27_oscbank,gensym("oscbank"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc27_transpose,gensym("transpose"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc27_fftinfo,gensym("fftinfo"),0);
	class_addmethod(c,(t_method)cavoc27_rule,gensym("rule"),A_GIMME,0);
	class_addmethod(c,(t_method)cavoc27_interpolate,gensym("interpolate"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc27_retune,gensym("retune"),A_FLOAT,A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc27_capture_spectrum,gensym("capture_spectrum"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc27_noalias,gensym("noalias"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc27_manual,gensym("manual"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc27_freeze,gensym("freeze"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc27_trigger,gensym("trigger"),0);
    class_addmethod(c,(t_method)cavoc27_density,gensym("density"),A_FLOAT,0);
    class_addmethod(c,(t_method)cavoc27_hold_time,gensym("hold_time"),A_FLOAT,0);
    cavoc27_class = c;
    fftease_announce(OBJECT_NAME);
}

void cavoc27_rand_set_rule(t_cavoc27 *x)
{
	int i;
	float rval;
	for( i = 0; i < 27; i++ ){
		rval = fftease_randf(0.0,1.0);
		if( rval < .333 )
			x->rule[i] = 0;
		else if(rval < .666 )
			x->rule[i] = 1;
		else x->rule[i] = 2;
	}
}

void cavoc27_freeze(t_cavoc27 *x, t_floatarg tog)
{
	x->freeze = (short) tog;
}

void cavoc27_manual(t_cavoc27 *x, t_floatarg tog)
{
	x->manual_mode = (short) tog;
}

void cavoc27_trigger(t_cavoc27 *x)
{
	x->external_trigger = 1;
}


void cavoc27_retune(t_cavoc27 *x, t_floatarg min, t_floatarg max)
{
	int i;
	t_fftease *fft = x->fft;
	t_float *tmpchannel = x->tmpchannel;
	t_float *last_frame = x->last_frame;
	if( max <= 0 || min <= 0 || min > max ){
		error("bad values for min and max multipliers");
		return;
	}
	if( min < .1 )
		min = 0.1;
	if( max > 2.0 )
		max = 2.0;
	for( i = 1; i < fft->N + 1; i += 2 ){
		 last_frame[i] = tmpchannel[i] = fft->c_fundamental * (float) (i / 2) * fftease_randf(min, max);
	}
	
}

void cavoc27_transpose (t_cavoc27 *x, t_floatarg pfac)
{
	t_fftease *fft = x->fft;
	fft->P = (float) pfac;
	fft->pitch_increment = fft->P*fft->L/fft->R;
}

void cavoc27_mute (t_cavoc27 *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}


void cavoc27_interpolate(t_cavoc27 *x, t_floatarg flag)
{
	x->interpolate_flag = (short) flag;
}

void cavoc27_capture_spectrum(t_cavoc27 *x, t_floatarg flag )
{
	x->capture_lock = (short)flag;
    post("capture flag: %d", x->capture_lock);
}

void cavoc27_capture_lock(t_cavoc27 *x, t_floatarg flag )
{
	x->capture_lock = (short)flag; 
}

void cavoc27_rule (t_cavoc27 *x, t_symbol *msg, short argc, t_atom *argv)
{
	int i;
	short *rule = x->rule;
	if( argc != 27 ){
		error("the rule must be size 18");
		return;
	}
	
	for( i = 0; i < 27; i++ ){
		rule[i] = (short) atom_getfloatarg( i, argc, argv);
	}
}

void cavoc27_free( t_cavoc27 *x ){
    fftease_free(x->fft);
    free(x->fft);
	free(x->ichannel);
	free(x->tmpchannel);
	free(x->last_frame);
	free(x->rule);
}

void *cavoc27_new(t_symbol *s, int argc, t_atom *argv)
{
t_fftease *fft;

	t_cavoc27 *x = (t_cavoc27 *)pd_new(cavoc27_class);
	outlet_new(&x->x_obj, gensym("signal"));
	x->fft = (t_fftease *) calloc(1, sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	x->hold_time = 1000.0;
	x->density = 0.1;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	x->freeze = 0;
	x->start_breakpoint = 1.0 - x->density;
	fft->obank_flag = 0;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
    if(argc > 2){ x->density = atom_getfloatarg(2, argc, argv); }
    if(argc > 3){ x->hold_time = atom_getfloatarg(3, argc, argv); }
	return x;
}

void cavoc27_noalias(t_cavoc27 *x, t_floatarg flag)
{
	x->fft->noalias = (short) flag;
}

void cavoc27_oscbank(t_cavoc27 *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void cavoc27_fftsize(t_cavoc27 *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	cavoc27_init(x);
}

void cavoc27_overlap(t_cavoc27 *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	cavoc27_init(x);
}

void cavoc27_winfac(t_cavoc27 *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	cavoc27_init(x);
}

void cavoc27_fftinfo(t_cavoc27 *x)
{
	fftease_fftinfo( x->fft, OBJECT_NAME );	
	post("frames left %d", x->frames_left);
	post("no alias: %d", x->fft->noalias);
}


void cavoc27_init(t_cavoc27 *x)
{
	int i;
	t_fftease *fft = x->fft;
	short initialized = fft->initialized;
	fftease_init(fft);	
	if(! fft->R ){
		error("cavoc27~: zero sampling rate!");
		return;
	}
	x->frame_duration = (float)fft->D/(float) fft->R;
	x->hold_frames = (int) (x->hold_time/x->frame_duration);
	x->frames_left = x->hold_frames;
	x->trigger_value = 0;
	x->set_count = 0;
	
	if(!initialized){
		srand(time(0));
		x->interpolate_flag = 0;
		x->capture_lock = 0;
		
		x->mute = 0;

		x->ichannel = (t_float *) calloc((fft->N+2), sizeof(t_float));
		x->tmpchannel = (t_float *) calloc((fft->N+2), sizeof(t_float));
		x->last_frame = (t_float *) calloc((fft->N+2), sizeof(t_float));
		x->rule = (short *) calloc(27, sizeof(short));
	}
    else {
		x->ichannel = (t_float *)realloc(x->ichannel,(fft->N+2)*sizeof(t_float));
		x->tmpchannel = (t_float *)realloc(x->tmpchannel,(fft->N+2)*sizeof(t_float));
		x->last_frame = (t_float *)realloc(x->last_frame,(fft->N+2)*sizeof(t_float));
	}
    if(x->frame_duration != 0){
        x->hold_frames = (int) ( (x->hold_time/1000.0) / x->frame_duration);
    } else {
        post("%s: zero FFT frame duration", OBJECT_NAME);
    }
    if( x->hold_frames < 1 )
        x->hold_frames = 1;
    
	cavoc27_rand_set_rule(x);
	cavoc27_rand_set_spectrum(x);
	for( i = 0; i < fft->N+2; i++ ){
		x->last_frame[i] = fft->channel[i];
	}
}

void cavoc27_rand_set_spectrum(t_cavoc27 *x)
{
	int i;
	float rval;
	t_fftease *fft = x->fft;
	t_float *channel = x->tmpchannel;
	//set spectrum

	for( i = 0; i < fft->N2 + 1; i++ ){
		if( fftease_randf(0.0, 1.0) > x->start_breakpoint){
			rval = fftease_randf(0.0, 1.0);
			if( rval < 0.5 ){
				channel[ i * 2 ] = 1;
			}
			else {
				channel[ i * 2 ] = 2;
			}
			++(x->set_count);
		} else {
			channel[ i * 2 ] = 0;
		}
		channel[ i * 2 + 1 ] = fft->c_fundamental * (float) i * fftease_randf(.9,1.1);
	}
}

void do_cavoc27(t_cavoc27 *x)
{
	t_fftease *fft = x->fft;
	int i;
	int frames_left = x->frames_left;

	int N = fft->N;
	t_float *tmpchannel = x->tmpchannel;
	t_float *ichannel = x->ichannel;

	int hold_frames = x->hold_frames;
	short *rule = x->rule;
	short left = x->left;
	short right = x->right;
	short center = x->center;
	t_float *last_frame = x->last_frame;
	t_float frak;
	short manual_mode = x->manual_mode;
	short trigger;
	short interpolate_flag = x->interpolate_flag;
	t_float *channel = fft->channel;
	
	if( manual_mode ){
		trigger = x->external_trigger;
	} else {
		trigger = 0;
	}
	
	if( x->capture_flag || (x->capture_lock && ! x->freeze)) {
		
		fftease_fold(fft);
		fftease_rdft(fft,1);
		fftease_convert(fft);
		for( i = 1; i < fft->N+1; i += 2){
			tmpchannel[i] = channel[i];
		}
	}
	if( ! manual_mode ){
		if( --frames_left <= 0 ){
			trigger = 1;
		}
	}
	if(trigger && ! x->freeze){
		for( i = 0; i < fft->N+1; i++ ){
			last_frame[i] = tmpchannel[i];
		}
		frames_left = hold_frames;
		for( i = 2; i < fft->N; i+=2 ){
			left = last_frame[ i - 2 ];
			center = last_frame[i] ;
			right = last_frame[i+2];
			tmpchannel[i] = cavoc27_apply_rule(left, right, center, rule );
		}
		// boundary cases 
		center = last_frame[0];
		right = last_frame[2];
		left = last_frame[N];
		tmpchannel[0] = cavoc27_apply_rule(left, right, center, rule );
		
		center = last_frame[N];
		right = last_frame[0];
		left = last_frame[N - 2];
		tmpchannel[N] = cavoc27_apply_rule(left, right, center, rule );
		x->external_trigger = trigger = 0 ;
	}
	if( interpolate_flag && ! x->freeze){
		frak = 1.0 - ((float) frames_left / (float) hold_frames);

		for( i = 0; i <N+2; i += 2 ){
			ichannel[i] = last_frame[i] + frak * ( tmpchannel[i] - last_frame[i] ); 
			ichannel[i+1] = last_frame[i+1];
		}	
		for( i = 0; i < N+2; i++ ){
			channel[i] = ichannel[i];
		}	
	} else {
		for( i = 0; i < N+2; i++){
			channel[i] = tmpchannel[i];
		}
	}	
	if(x->freeze){
		for( i = 0; i < N+2; i++){
			channel[i] = tmpchannel[i];
		}
	}
	
	if(fft->obank_flag){
		fftease_oscbank(fft);
	} else {
		fftease_unconvert(fft);
		fftease_rdft(fft, -1);
		fftease_overlapadd(fft);
	}

	x->frames_left = frames_left;
	
}

t_int *cavoc27_perform(t_int *w)
{
	int	i,j;

	////////////
	t_cavoc27 *x = (t_cavoc27 *) (w[1]);
    t_float *MSPInputVector = (t_float *) (w[2]);
	t_float *MSPOutputVector = (t_float *) (w[3]);
    t_fftease *fft = x->fft;
	int D = fft->D;
	int Nw = fft->Nw;
    t_float *input = fft->input;
	t_float *output = fft->output;
	t_float mult = fft->mult;
	int MSPVectorSize = fft->MSPVectorSize;
    t_float *internalInputVector = fft->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;	
	
    
	if(x->mute){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
        return w+4;
	}
	if(fft->obank_flag){
		mult *= FFTEASE_OSCBANK_SCALAR;
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
		do_cavoc27(x);
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
    }
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
			do_cavoc27(x);
			for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
            memcpy(output, output + D, (Nw-D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        memcpy(internalInputVector + (operationCount * MSPVectorSize), MSPInputVector,MSPVectorSize * sizeof(t_float));
        memcpy(MSPOutputVector, internalOutputVector + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_float));
		operationCount = (operationCount + 1) % operationRepeat;
		if( operationCount == 0 ) {
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw - D), internalInputVector, D * sizeof(t_float));
			do_cavoc27(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
        fft->operationCount = operationCount;
	}
    return w+4;
}		

int cavoc27_apply_rule( short left, short right, short center, short *rule){
	
	if( left == 0 && center == 0 && right == 0 )
		return rule[0];
	if( left == 1 && center == 0 && right == 1 )
		return rule[1];
	if( left == 1 && center == 0 && right == 0 )
		return rule[2];
	if( left == 0 && center == 0 && right == 1 )
		return rule[3];
	if( left == 2 && center == 0 && right == 2 )
		return rule[4];
	if( left == 2 && center == 0 && right == 0 )
		return rule[5];
	if( left == 0 && center == 0 && right == 2 )
		return rule[6];
	if( left == 2 && center == 0 && right == 1 )
		return rule[7];
	if( left == 1 && center == 0 && right == 2 )
		return rule[8];
	
	if( left == 0 && center == 1 && right == 0 )
		return rule[9];
	if( left == 1 && center == 1 && right == 1 )
		return rule[10];
	if( left == 1 && center == 1 && right == 0 )
		return rule[11];
	if( left == 0 && center == 1 && right == 1 )
		return rule[12];
	if( left == 2 && center == 1 && right == 2 )
		return rule[13];
	if( left == 2 && center == 1 && right == 0 )
		return rule[14];
	if( left == 0 && center == 1 && right == 2 )
		return rule[15];
	if( left == 2 && center == 1 && right == 1 )
		return rule[16];
	if( left == 1 && center == 1 && right == 2 )
		return rule[17];
	
	if( left == 0 && center == 2 && right == 0 )
		return rule[18];
	if( left == 1 && center == 2 && right == 1 )
		return rule[19];
	if( left == 1 && center == 2 && right == 0 )
		return rule[20];
	if( left == 0 && center == 2 && right == 1 )
		return rule[21];
	if( left == 2 && center == 2 && right == 2 )
		return rule[22];
	if( left == 2 && center == 2 && right == 0 )
		return rule[23];
	if( left == 0 && center == 2 && right == 2 )
		return rule[24];
	if( left == 2 && center == 2 && right == 1 )
		return rule[25];
	if( left == 1 && center == 2 && right == 2 )
		return rule[26];
	return 0; //should never happen  
}


void cavoc27_density(t_cavoc27 *x, t_floatarg density)
{
    int i;
    t_fftease *fft = x->fft;
    t_float *channel =  x->tmpchannel;
    
    if( density < 0.0001 ){
        density = .0001;
    } else if( density > .9999 ){
        density = 1.0;
    }
    x->density = density;
    x->start_breakpoint = 1.0 - x->density;
    for( i = 0; i < fft->N2 + 1; i++ ){
        if( fftease_randf(0.0, 1.0) > x->start_breakpoint ){
            if( fftease_randf(0.0,1.0) > 0.5 ){
                channel[ i * 2 ] = 1;
            }
            else {
                channel[ i * 2 ] = 2;
            }
            ++(x->set_count);
        } else {
            channel[ i * 2 ] = 0;
        }
    }
    for( i = 0; i < fft->N+2; i++ ){
        x->last_frame[i] = channel[i];
    }
}

void cavoc27_hold_time(t_cavoc27 *x, t_floatarg hold_time)
{
    if(hold_time <= 0){
        post("illegal hold time %f",hold_time);
        return;
    }
    x->hold_time = hold_time;
    if(! x->fft->initialized){
        return;
    }
    if(! x->frame_duration){
        error("%s: zero frame duration",OBJECT_NAME);
        return;
    }
    x->hold_frames = (int) ( (x->hold_time/1000.0) / x->frame_duration);
    if( x->hold_frames < 1 )
        x->hold_frames = 1;    
}

void cavoc27_dsp(t_cavoc27 *x, t_signal **sp)
{
    int reset_required = 0;
    int maxvectorsize = sys_getblksize();
    int samplerate = sys_getsr();
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        cavoc27_init(x);
    }
    dsp_add(cavoc27_perform, 3, x, sp[0]->s_vec, sp[1]->s_vec);
}

