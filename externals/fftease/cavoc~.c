/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *cavoc_class;

#define OBJECT_NAME "cavoc~"

typedef struct _cavoc
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_float frame_duration;
	int max_bin;
	
	t_float fundamental;
	short left;
	short right;
	short center;
	short *rule;
	
	t_float start_breakpoint;
	int hold_frames;
	int frames_left;
	int set_count;
	void *list_outlet;
	t_atom *list_data;
	short mute;
	short external_trigger;
	short trigger_value; // set to 1 when a bang is received
	
	t_float topfreq; // highest to synthesize - Nyquist by default
	t_float bottomfreq;
	t_float *freqs;
	t_float *amps;
	t_float *cavoc;
    t_float density; // treat as attribute
    t_float hold_time; // treat as attribute
} t_cavoc;

void *cavoc_new(t_symbol *msg, short argc, t_atom *argv);
void cavoc_dsp(t_cavoc *x, t_signal **sp);
t_int *cavoc_perform(t_int *w);
void cavoc_free( t_cavoc *x );
int cavoc_apply_rule( short left, short right, short center, short *rule);
float cavoc_randf(float min, float max);
void cavoc_rule (t_cavoc *x, t_symbol *msg, short argc, t_atom *argv);
void cavoc_retune (t_cavoc *x, t_floatarg min, t_floatarg max);
void cavoc_mute (t_cavoc *x, t_floatarg toggle);
void cavoc_external_trigger(t_cavoc *x, t_floatarg toggle);
void cavoc_init(t_cavoc *x);
void cavoc_fftinfo(t_cavoc *x);
void cavoc_bang(t_cavoc *x);
void cavoc_topfreq(t_cavoc *x, t_floatarg tf);
void cavoc_oscbank(t_cavoc *x, t_floatarg flag);
void cavoc_density(t_cavoc *x, t_floatarg f);
void cavoc_hold_time(t_cavoc *x, t_floatarg f);
void build_spectrum(t_cavoc *x, float min, float max);
void cavoc_bottomfreq(t_cavoc *x, t_floatarg bf);


void cavoc_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("cavoc~"), (t_newmethod)cavoc_new,
                  (t_method)cavoc_free,sizeof(t_cavoc), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_cavoc, x_f);
	class_addmethod(c,(t_method)cavoc_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)cavoc_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc_oscbank,gensym("oscbank"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc_rule,gensym("rule"),A_GIMME,0);
	class_addmethod(c,(t_method)cavoc_external_trigger,gensym("external_trigger"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc_bang,gensym("bang"),0);
	class_addmethod(c,(t_method)cavoc_retune,gensym("retune"),A_FLOAT,A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc_topfreq,gensym("topfreq"),A_FLOAT,0);
	class_addmethod(c,(t_method)cavoc_bottomfreq,gensym("bottomfreq"),A_FLOAT,0);
    class_addmethod(c,(t_method)cavoc_density,gensym("density"),A_FLOAT,0);
    class_addmethod(c,(t_method)cavoc_hold_time,gensym("hold_time"),A_FLOAT,0);
    cavoc_class = c;
    fftease_announce(OBJECT_NAME);
}


void cavoc_fftinfo( t_cavoc *x )
{
t_fftease *fft = x->fft;
	fftease_fftinfo( fft, OBJECT_NAME );
}

void cavoc_density(t_cavoc *x, t_floatarg density)
{
    int i;
    t_fftease *fft = x->fft;
    
    if( density < 0.0001 ){
        density = .0001;
    } else if( density > .9999 ){
        density = 1.0;
    }
    x->density = density;
    x->start_breakpoint = 1.0 - x->density;
    for( i = 0; i < fft->N2 + 1; i++ ){
        if( cavoc_randf(0.0, 1.0) > x->start_breakpoint ){
            x->amps[ i ] = 1;
            ++(x->set_count);
        } else {
            x->amps[ i ] = 0;
        }
    }
}

void cavoc_hold_time(t_cavoc *x, t_floatarg f)
{
    if(f <= 0)
        return;
    x->hold_time = f;
    x->hold_frames = (int) ((x->hold_time/1000.0) / x->frame_duration);
    if( x->hold_frames < 1 )
        x->hold_frames = 1;
    x->frames_left = x->hold_frames;
}

void cavoc_external_trigger(t_cavoc *x, t_floatarg toggle)
{
	x->external_trigger = (short)toggle;
}

void cavoc_mute (t_cavoc *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void cavoc_retune(t_cavoc *x, t_floatarg min, t_floatarg max)
{	
	if( max <= 0 || min <= 0 || min > max ){
		error("bad values for min and max multipliers");
		return;
	}
	if( min < .1 )
		min = 0.1;
	if( max > 2.0 )
		max = 2.0;
	build_spectrum(x, (float)min, (float)max);
}



void cavoc_bang(t_cavoc *x)
{
	if(x->external_trigger)
		x->trigger_value = 1;
}

void cavoc_oscbank(t_cavoc *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void cavoc_rule (t_cavoc *x, t_symbol *msg, short argc, t_atom *argv)
{
	int i;
	short *rule = x->rule;
	if( argc != 8 ){
		error("the rule must be size 8");
		return;
	}
	
	for( i = 0; i < 8; i++ ){
		rule[i] = (short) atom_getfloatarg(i, argc, argv);
	}
}

void cavoc_free( t_cavoc *x ){
	fftease_free(x->fft);
    free(x->fft);
	free(x->amps);
	free(x->freqs);
	free(x->rule);
}

void *cavoc_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_cavoc *x = (t_cavoc *)pd_new(cavoc_class);
    t_fftease *fft;
	outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
    fft = x->fft;
    x->fft->initialized = 0;
	x->density = 0.1;
	x->hold_time = 500.0; // convert from ms
	x->fft->obank_flag = 0;
	x->fft->N = FFTEASE_DEFAULT_FFTSIZE;
	x->fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	x->fft->winfac = FFTEASE_DEFAULT_WINFAC;

    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
    if(argc > 2){ x->density = atom_getfloatarg(2, argc, argv); }
    if(argc > 3){ x->hold_time = atom_getfloatarg(3, argc, argv); }

	return x;
}

void cavoc_init(t_cavoc *x)
{
	t_fftease *fft = x->fft;
	short initialized = fft->initialized;
	fftease_init(fft);

	fft->lo_bin = 0; 
	fft->hi_bin = fft->N2 - 1; 
			
	if(! fft->R ){
		error("zero sampling rate!");
		return;
	}
	x->frame_duration = (float)fft->D/(float) fft->R;
	if(x->hold_time <= 0.0)
		x->hold_time = 150;
	x->hold_frames = (int) ((x->hold_time * 0.001) / x->frame_duration) ;
	x->frames_left = x->hold_frames;
	x->trigger_value = 0;
	x->topfreq = fft->R / 2.0;
	x->bottomfreq = 0.0;

	if(!initialized){
		
		srand(time(0));
		x->mute = 0;
		
		x->external_trigger = 0;
		if( x->density < 0.0 ){
			x->density = 0;
		} else if( x->density > 1.0 ){
			x->density = 1.0;
		}
		x->start_breakpoint = 1.0 - x->density;
		x->freqs = (t_float *) calloc((fft->N2 + 1), sizeof(t_float));
		x->amps = (t_float *) calloc((fft->N2 + 1), sizeof(t_float));
		x->cavoc = (t_float *) calloc((fft->N + 2), sizeof(t_float));
		x->rule = (short *)  calloc(8, sizeof(short));
		
		x->rule[2] = x->rule[3] = x->rule[5] = x->rule[6] = 1;
		x->rule[0] = x->rule[1] = x->rule[4] = x->rule[7] = 0;
		
		
	} else {    
		x->freqs = (t_float *) realloc(x->freqs, (fft->N2 + 1) * sizeof(t_float));
		x->amps = (t_float *) realloc(x->amps, (fft->N2 + 1) * sizeof(t_float));
	}
	build_spectrum(x, 0.9, 1.1);
}


void build_spectrum(t_cavoc *x, float min, float max)
{
t_fftease *fft = x->fft;
float basefreq;
int i;
	x->set_count = 0;
	for(i = 0; i < fft->N2 + 1; i++){
		if(cavoc_randf(0.0, 1.0) > x->start_breakpoint){
			x->amps[i] = 1;
			++(x->set_count);
		} else {
			x->amps[i] = 0;
		}
		basefreq = x->bottomfreq + (( (x->topfreq - x->bottomfreq) / (float) fft->N2 ) * (float) i );
		x->freqs[i] = basefreq * cavoc_randf(min,max);
	}
	for( i = 0; i < fft->N2 + 1; i++ ){
		fft->channel[i * 2] = x->amps[i];
		fft->channel[i * 2 + 1] = x->freqs[i];
	}

}

void cavoc_topfreq(t_cavoc *x, t_floatarg tf)
{
	t_fftease *fft = x->fft;
	if(tf < 100 || tf > fft->R / 2.0){
		error("%s: top frequency out of range: %f",OBJECT_NAME,  tf);
		return;
	}
	x->topfreq = (float) tf;
	build_spectrum(x, 0.9, 1.1);
}

void cavoc_bottomfreq(t_cavoc *x, t_floatarg bf)
{
	if(bf < 0 && bf > x->topfreq){
		error("%s: bottom frequency out of range: %f",OBJECT_NAME,  bf);
		return;
	}
	x->bottomfreq = (float) bf;
	build_spectrum(x, 0.9, 1.1);
}

void do_cavoc(t_cavoc *x)
{
	int i;
	t_fftease *fft = x->fft;
	int N = fft->N;
	int N2 = fft->N2;
	t_float *channel = fft->channel;
	int hold_frames = x->hold_frames;
	short *rule = x->rule;
	short left = x->left;
	short right = x->right;
	short center = x->center;
	short external_trigger = x->external_trigger;
	short new_event = 0;
	t_float *amps = x->amps;
	t_float *freqs = x->freqs;
			
	if(external_trigger){// only accurate to within a vector because of FFT
		if(x->trigger_value){
			x->trigger_value = 0;
			new_event = 1;
		}
	} else if(--(x->frames_left) <= 0){
		x->frames_left = hold_frames;
		new_event = 1;
	}
	if(new_event){
		for( i = 1; i < N2; i++ ){
			left = amps[i - 1];
			center = amps[i] ;
			right = amps[i + 1];
			channel[i * 2] = cavoc_apply_rule(left, right, center, rule);
		}
		center = amps[0];
		right = amps[1];
		left = amps[N2];
		channel[0] = cavoc_apply_rule(left, right, center, rule);
		
		center = amps[N2];
		right = amps[0];
		left = amps[N2 - 1];
		channel[N] = cavoc_apply_rule(left, right, center, rule);
		for(i = 0; i < N2 + 1; i++){
			channel[(i*2) + 1] = freqs[i];
			amps[i] = channel[i * 2];
		}	
	}
		
	if(fft->obank_flag){
		for(i = 0; i < N2 + 1; i++){
			channel[(i*2) + 1] = freqs[i];
			channel[i * 2] = amps[i];
		}
		fftease_oscbank(fft);
	} else {
		fftease_unconvert(fft);
		fftease_rdft(fft, -1);
		fftease_overlapadd(fft);
	}		
}


t_int *cavoc_perform(t_int *w)
{
	int	i,j;
    t_cavoc *x = (t_cavoc *) (w[1]);
    t_float *MSPOutputVector = (t_float *)(w[2]);
	t_fftease *fft = x->fft;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float *output = fft->output;
	float mult = fft->mult ;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	t_float *internalOutputVector = fft->internalOutputVector;
	int MSPVectorSize = fft->MSPVectorSize;
	
	if(fft->obank_flag){
		mult *= FFTEASE_OSCBANK_SCALAR;
	}
    
	if( x->mute){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
        return w+3;
	}
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        
		do_cavoc(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
			do_cavoc(x);
			for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
            memcpy(output, output + D, (Nw-D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
		memcpy(MSPOutputVector, internalOutputVector + (operationCount * MSPVectorSize), MSPVectorSize * sizeof(float));
		operationCount = (operationCount + 1) % operationRepeat;
		if( operationCount == 0 ) {
			do_cavoc(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
        fft->operationCount = operationCount;
	}
	return w+3;
}		

int cavoc_apply_rule(short left, short right, short center, short *rule){
	
	if( ! center ){
		if( ! left && ! right){
			return  rule[0];
		} else if ( ! left && right ){ 
			return rule[1];
		} else if ( left && ! right ) {
			return rule[2];
		} else if (left && right) {
			return rule[3];
		}
	} else {
		if( ! left && ! right){
			return rule[4];
		} else if ( ! left && right ){ 
			return rule[5];
		} else if ( left && ! right ) {
			return rule[6];
		} else if (left && right) {
			return rule[7];
		}
	}
	return 0;
}

float cavoc_randf(float min, float max)
{
	float randv;
	randv = (float) (rand() % 32768) / 32768.0 ;
	return (min + ((max-min) * randv))  ;
}

void cavoc_dsp(t_cavoc *x, t_signal **sp)
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
        cavoc_init(x);
    }
    dsp_add(cavoc_perform, 2, x, sp[1]->s_vec);
}
