/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *disarrain_class;

#define OBJECT_NAME "disarrain~"

typedef struct _disarrain
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_float *last_channel;
	t_float *composite_channel;
	int *shuffle_mapping;
	int *last_shuffle_mapping;
	int *shuffle_tmp; // work space for making a new distribution
	int shuffle_count;// number of bins to swap
	int last_shuffle_count;// ditto from last shuffle mapping
	int max_bin;
	void *list_outlet;
	t_atom *list_data;
	short mute;
	short bypass;
	t_float frame_duration; // duration in seconds of a single frame
	t_float interpolation_duration; // duration in seconds of interpolation
	int interpolation_frames; // number of frames to interpolate
	int frame_countdown; // keep track of position in interpolation
	int perform_method;// 0 for lean, 1 for full conversion
	float ival;
	short lock;// lock for switching mapping arrays, but not used now
	short force_fade; // new fadetime set regardless of situation
	short force_switch;// binds new distribution to change of bin count
	long fftsize_attr;
	long overlap_attr;
    t_float top_frequency;// for remapping spectrum (NOW AN ATTRIBUTE)
	short reset_flag; // call for reset 
	short switchcount_flag; // call for switch count
    int switchcount;
	int new_shuffle_count; // call to change switch count
} t_disarrain;


void *disarrain_new(t_symbol *msg, short argc, t_atom *argv);
void disarrain_dsp(t_disarrain *x, t_signal **sp);
t_int *disarrain_perform(t_int *w);
void disarrain_switch_count(t_disarrain *x, t_floatarg i);
void disarrain_fadetime(t_disarrain *x, t_floatarg f);
void disarrain_topfreq(t_disarrain *x, t_floatarg f);
void reset_shuffle( t_disarrain *x );
void disarrain_showstate( t_disarrain *x );
void disarrain_list (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv);
void disarrain_setstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv);
void disarrain_isetstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv);
int rand_index(int max);
void disarrain_mute(t_disarrain *x, t_floatarg toggle);
void copy_shuffle_array(t_disarrain *x);
void interpolate_frames_to_channel(t_disarrain *x);
void disarrain_killfade(t_disarrain *x);
void disarrain_forcefade(t_disarrain *x, t_floatarg toggle);
void disarrain_init(t_disarrain *x);
void disarrain_free(t_disarrain *x);
void disarrain_fftinfo(t_disarrain *x);
void disarrain_force_switch(t_disarrain *x, t_floatarg toggle);

void disarrain_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("disarrain~"), (t_newmethod)disarrain_new,
                  (t_method)disarrain_free,sizeof(t_disarrain), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_disarrain, x_f);
	class_addmethod(c,(t_method)disarrain_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)disarrain_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)reset_shuffle, gensym("bang"), 0);
	class_addmethod(c,(t_method)disarrain_showstate,gensym("showstate"),0);
	class_addmethod(c,(t_method)disarrain_setstate, gensym("setstate"), A_GIMME, 0);
	class_addmethod(c,(t_method)disarrain_isetstate, gensym("isetstate"), A_GIMME, 0);
	class_addmethod(c,(t_method)disarrain_topfreq, gensym("topfreq"), A_FLOAT, 0);
	class_addmethod(c,(t_method)disarrain_fadetime, gensym("fadetime"), A_FLOAT, 0);
	class_addmethod(c,(t_method)disarrain_switch_count, gensym("switch_count"), A_FLOAT, 0);
	class_addmethod(c,(t_method)disarrain_killfade, gensym("killfade"), 0);
    disarrain_class = c;
    fftease_announce(OBJECT_NAME);
}

void disarrain_free(t_disarrain *x)
{
	fftease_free(x->fft);
    free(x->fft);
	free(x->last_channel);
	free(x->composite_channel);
	free(x->shuffle_mapping);
	free(x->last_shuffle_mapping);
	free(x->shuffle_tmp);
	free(x->list_data);
}

void disarrain_init(t_disarrain *x)
{
	int i;
	t_float curfreq;
	
	t_fftease *fft = x->fft;

	t_float c_fundamental;
	short initialized = fft->initialized;
	
 	fftease_init(fft);

	int N2 = fft->N2;
	int N = fft->N;
	int D = fft->D;
	int R = fft->R;
	c_fundamental = fft->c_fundamental;
	if(initialized == 0){
		x->mute = 0;
		x->bypass = 0;
		x->force_fade = 0;
		x->interpolation_duration = 0.1; //seconds
		x->shuffle_mapping = (int *) calloc( N2, sizeof(int) ) ;
		x->last_shuffle_mapping = (int *) calloc( N2, sizeof(int) ) ;
		x->shuffle_tmp = (int *) calloc( N2, sizeof(int) ) ;
		x->list_data = (t_atom *) calloc((N+2), sizeof(t_atom) ) ;
		x->last_channel = (t_float *) calloc((N+2), sizeof(float));
		x->composite_channel = (t_float *) calloc((N+2), sizeof(float));
		x->reset_flag = 0;
		x->new_shuffle_count = 0;
	} else {
		x->shuffle_mapping = (int *)realloc(x->shuffle_mapping, N2 * sizeof(int));
		x->last_shuffle_mapping = (int *)realloc(x->last_shuffle_mapping, N2 * sizeof(int));
		x->shuffle_tmp = (int *)realloc(x->shuffle_tmp, N2 * sizeof(int));
		x->list_data = (t_atom *)realloc(x->list_data, (N+2) * sizeof(t_atom));
		x->last_channel = (t_float *)realloc(x->last_channel,(N+2) * sizeof(t_float));
		x->composite_channel = (t_float *)realloc(x->composite_channel, (N+2) * sizeof(t_float));

	}
	
	if( x->top_frequency < c_fundamental || x->top_frequency > 20000) {
		x->top_frequency = 20000.0 ;
	}
	x->max_bin = 1;  
	curfreq = 0;
	while( curfreq < x->top_frequency ) {
		++(x->max_bin);
		curfreq += c_fundamental ;
	}
	for( i = 0; i < N2; i++ ) {
		x->shuffle_mapping[i] = x->last_shuffle_mapping[i] = i*2;
	}
	reset_shuffle(x); // set shuffle lookup
	copy_shuffle_array(x);// copy it to the last lookup (for interpolation)
	x->frame_duration = (float) D / (float) R;
	x->interpolation_frames = x->interpolation_duration / x->frame_duration;
	x->frame_countdown = 0;
	x->shuffle_count = 0;
	x->last_shuffle_count = 0;
    if(x->switchcount > 0){
        disarrain_switch_count (x, (t_float)x->switchcount);
    }
}


void disarrain_force_switch(t_disarrain *x, t_floatarg f)
{
	x->force_switch = (short)f;
}



void disarrain_fadetime (t_disarrain *x, t_floatarg f)
{
	int frames;
	float duration;
	

    if(f > 0.0){
        x->interpolation_duration = f * 0.001;
    } else {
        return;
    }
    if(! x->fft->initialized){
        return;
    }
	if(x->frame_duration <= 0.0){
		// error("%s: frame duration %f is too low", OBJECT_NAME, x->frame_duration);
		return;
	}
	// duration = f * .001;
	frames = x->interpolation_duration / x->frame_duration;
	if( frames <= 1){
		error("%s: fadetime too short",OBJECT_NAME);
		return;
	}
	x->interpolation_frames = frames;
}

void disarrain_killfade(t_disarrain *x)
{
	x->frame_countdown = 0;
	
}

void *disarrain_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft;
	t_disarrain *x = (t_disarrain *)pd_new(disarrain_class);
	outlet_new(&x->x_obj, gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
    x->list_outlet = outlet_new(&x->x_obj, gensym("list"));
	srand(time(0));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;

	fft->initialized = 0;
	x->top_frequency = 3000;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void disarrain_forcefade(t_disarrain *x, t_floatarg toggle)
{
	x->force_fade = (short)toggle;	
}

void disarrain_mute(t_disarrain *x, t_floatarg toggle)
{
	x->mute = (short)toggle;	
}

void disarrain_bypass(t_disarrain *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;	
}

void disarrain_fftsize(t_disarrain *x, t_floatarg f)
{	
	t_fftease *fft = x->fft;
	fft->N = (int) f;
	disarrain_init(x);
}

void disarrain_overlap(t_disarrain *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->overlap = (int) f;
	disarrain_init(x);
}

void disarrain_winfac(t_disarrain *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->winfac = (int) f;
	disarrain_init(x); /* calling lighter reinit routine */
}

void disarrain_fftinfo( t_disarrain *x )
{
	fftease_fftinfo( x->fft, OBJECT_NAME );
}

void do_disarrain(t_disarrain *x)
{
	t_fftease *fft = x->fft;
	t_float *channel = fft->channel;
	int N = fft->N;
	int  N2 = fft->N2;
	t_float *last_channel = x->last_channel;
	int		i,j;
	int max = x->max_bin;
	int temp, p1, p2;
	float tmp;
	int *shuffle_mapping = x->shuffle_mapping;
	int shuffle_count = x->shuffle_count;
	int *last_shuffle_mapping = x->last_shuffle_mapping;
	int *shuffle_tmp = x->shuffle_tmp;
	int last_shuffle_count = x->last_shuffle_count;	
	int frame_countdown = x->frame_countdown; // will read from variable
	int interpolation_frames = x->interpolation_frames;
	float ival = x->ival;
	int new_shuffle_count = x->new_shuffle_count;

	
	if(x->switchcount_flag){
		
		if( new_shuffle_count < 0 ){
			new_shuffle_count = 0;
		}
		if( new_shuffle_count > N2 ) {
			new_shuffle_count = N2;
		}
		if( new_shuffle_count > x->max_bin){
			new_shuffle_count = x->max_bin;
			post("disarrain~: switch constrained to %d", x->max_bin);
		}		

		memcpy(last_shuffle_mapping, shuffle_mapping,  N2 * sizeof(int));
        
		x->last_shuffle_count = x->shuffle_count;
		x->shuffle_count = new_shuffle_count;
		x->frame_countdown = x->interpolation_frames; // force interpolation

		shuffle_count = x->shuffle_count;
		frame_countdown = x->frame_countdown;
		
		x->switchcount_flag = 0;
		x->reset_flag = 0;
	}
	else if(x->reset_flag){
        
		memcpy(last_shuffle_mapping, shuffle_mapping,  N2 * sizeof(int));
		last_shuffle_count = shuffle_count;
		shuffle_count = new_shuffle_count;
		// post("%d %d %d", last_shuffle_count, shuffle_count, new_shuffle_count);
		
		for( i = 0; i < N2; i++ ) {
			shuffle_tmp[i] = i;
		}
		
		// crashed before here
		for( i = 0; i < max; i++ ) {
			p1 = rand() % max;
			p2 = rand() % max;
			if(p1 < 0 || p1 > max || p2 < 0 || p2 > max){
				error("disarrain~: bad remaps: %d %d against %d", p1, p2, max);
			} else {
				temp = shuffle_tmp[p1];
				shuffle_tmp[ p1 ] = shuffle_tmp[ p2 ];
				shuffle_tmp[ p2 ] = temp;
			}
		}
		for( i = 0; i < N2; i++ ) {
			shuffle_tmp[i] *= 2;
		}
		frame_countdown = interpolation_frames;	
		// post("in: countdown: %d, frames: %d", frame_countdown, interpolation_frames);
        // dangerous???
        
		memcpy(shuffle_mapping, shuffle_tmp,  N2 * sizeof(int));

		x->reset_flag = 0;
	}
	
	fftease_fold(fft);
	fftease_rdft(fft,1);
	fftease_leanconvert(fft);
	
	// first time for interpolation, just do last frame 
	
	if(frame_countdown == interpolation_frames){
		
		for( i = 0, j = 0; i < last_shuffle_count ; i++, j+=2){
			tmp = channel[j];
			channel[j] = channel[last_shuffle_mapping[i]];
			channel[last_shuffle_mapping[i]] = tmp;
		}
		--frame_countdown;
	} 
	// test only
	else if( frame_countdown > 0 ){
		ival = (float)frame_countdown/(float)interpolation_frames;
		// copy current frame to lastframe
		for(j = 0; j < N; j+=2){
			last_channel[j] = channel[j];
		}	
		// make last frame swap
		for(i = 0, j = 0; i < last_shuffle_count ; i++, j+=2){
			tmp = last_channel[j];
			last_channel[j] = last_channel[last_shuffle_mapping[i]];
			last_channel[last_shuffle_mapping[i]] = tmp;
			
		}	
		// make current frame swap
		for( i = 0, j = 0; i < shuffle_count ; i++, j+=2){
			tmp = channel[j];
			channel[j]  = channel[shuffle_mapping[i]];
			channel[shuffle_mapping[i]]  = tmp;
			
		}
		// now interpolate between the two
		
		for(j = 0; j < N; j+=2){
			channel[j] = channel[j] + ival * (last_channel[j] - channel[j]);
		}
		
		--frame_countdown;
		if(frame_countdown <= 0){
			for(i = 0; i<N2; i++){
				last_shuffle_mapping[i] = shuffle_mapping[i];
			}
			last_shuffle_count = shuffle_count;
		}
	} else {
		// otherwise straight swapping
		for( i = 0, j = 0; i < shuffle_count ; i++, j+=2){
			tmp = channel[j];
			channel[j]  = channel[ shuffle_mapping[i]];
			channel[shuffle_mapping[i]] = tmp;     
		}
		ival = 0.0;
	}
	
	fftease_leanunconvert(fft);
	fftease_rdft(fft,-1);
	fftease_overlapadd(fft);


	
	x->frame_countdown = frame_countdown;
	x->last_shuffle_count = last_shuffle_count;
	x->shuffle_count = shuffle_count;
	x->ival = ival;
}

t_int *disarrain_perform(t_int *w)
{
	int i,j;
	
    t_disarrain *x = (t_disarrain *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *MSPOutputVector = (t_float *)(w[3]);
	t_float *sync_vec = (t_float *)(w[4]);
    t_fftease *fft = x->fft;
	t_float *input = fft->input;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float *output = fft->output;
	t_float mult = fft->mult;
	int MSPVectorSize = fft->MSPVectorSize;
	t_float *internalInputVector = fft->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;		
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;	
	
	if(x->mute){
		for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		for(i=0; i < MSPVectorSize; i++){ sync_vec[i] = 0.0; }
		return w+5;
	}	

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_disarrain(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_disarrain(x);
			
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
            
			do_disarrain(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
	/* send out sync signal */
	for(j = 0; j < MSPVectorSize; j++){
		sync_vec[j] = 1.0 - x->ival;
	}
    return w+5;
}		


void interpolate_frames_to_channel(t_disarrain *x)
{
	float ival;
	float tmp;
	int i,j;
	int frame_countdown = x->frame_countdown;
	int interpolation_frames = x->interpolation_frames;
	t_float *channel = x->fft->channel;
	t_float *last_channel = x->last_channel;
	int *shuffle_mapping = x->shuffle_mapping;
	int shuffle_count = x->shuffle_count;
	int *last_shuffle_mapping = x->last_shuffle_mapping;
	int last_shuffle_count = x->last_shuffle_count;	
	int local_max_bins;
	int N = x->fft->N;
	
	ival = (t_float)frame_countdown/(t_float)interpolation_frames;
	
	local_max_bins = (shuffle_count > last_shuffle_count)? shuffle_count : last_shuffle_count;
	for(j = 0; j < N; j+=2){
		last_channel[j] = channel[j];
	}
	// make last frame
	for( i = 0, j = 0; i < last_shuffle_count ; i++, j+=2){
		tmp = last_channel[j];
		last_channel[j] = last_channel[last_shuffle_mapping[i]];
		last_channel[last_shuffle_mapping[i]] = tmp;
	}
	// make current frame
	for( i = 0, j = 0; i < shuffle_count ; i++, j+=2){
		tmp = channel[j];
		channel[j]  = channel[shuffle_mapping[i]];
		channel[shuffle_mapping[i]]  = tmp;
	}
	// now interpolate between the two
	
	for(j = 0; j < N; j+=2){
		channel[j] += ival * (last_channel[j] - channel[j]);
	}
}



void disarrain_switch_count (t_disarrain *x, t_floatarg f)
{
	int i = f;
    x->switchcount = i;
    if(! x->fft->initialized){
        return;
    }
	if( i < 0 ){
		i = 0;
	}
	if( i > x->max_bin ) {
		i = x->max_bin;
	}
	x->reset_flag = 1;
	x->new_shuffle_count = i;
}


void reset_shuffle (t_disarrain *x)
{	
	x->reset_flag = 1;
}

void copy_shuffle_array(t_disarrain *x)
{
	int N2 = x->fft->N2;
	int *shuffle_mapping = x->shuffle_mapping;
	int *last_shuffle_mapping = x->last_shuffle_mapping;	

	memcpy(shuffle_mapping, last_shuffle_mapping,  N2 * sizeof(int));
	x->last_shuffle_count = x->shuffle_count;
	
}


int rand_index(int max) {
	return (rand() % max);
}


void disarrain_topfreq (t_disarrain *x, t_floatarg freq)
{
    t_float funda = (t_float) x->fft->R / (t_float) x->fft->N;
    t_float curfreq;
    if(freq  <= 0 || freq > 22050){
        post("freq %f is out of range", freq);
        return;
    }
    x->top_frequency = freq;
    if(! x->fft->initialized){
        return;
    }
    funda = (t_float) x->fft->R / (t_float) x->fft->N;
    x->max_bin = 1;
    curfreq = 0;
    while( curfreq < freq ) {
        ++(x->max_bin);
        curfreq += funda ;
    }
}

void disarrain_list (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int ival;
	x->shuffle_count = argc;
	for (i=0; i < argc; i++) {
		ival = (int)argv[i].a_w.w_float;
		if (ival < x->fft->N2) {
			x->shuffle_mapping[i] = ival;
		}
	}
	return;
}


void disarrain_isetstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int ival;
	int N2 = x->fft->N2;
	
	copy_shuffle_array(x);
	x->shuffle_count = argc;
    
	for (i=0; i < argc; i++) {
		ival = 2 * atom_getfloatarg(i,argc,argv);
		
		if ( ival < N2 && ival >= 0) {
			x->shuffle_mapping[ i ] = ival;
		}else {
			error("%s: %d is out of range",OBJECT_NAME, ival);
		}
	}

	x->frame_countdown = x->interpolation_frames;
	return;
}

void disarrain_setstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int ival;
	int N2 = x->fft->N2;
	x->shuffle_count = argc;
	for (i=0; i < argc; i++) {
		ival = 2 *atom_getfloatarg(i,argc,argv);
		
		if ( ival < N2 && ival >= 0) {
			x->shuffle_mapping[ i ] = ival;
		} else {
			error("%s: %d is out of range",OBJECT_NAME, ival);
		}
	}
	return;
}

// REPORT CURRENT SHUFFLE STATUS
void disarrain_showstate (t_disarrain *x ) {
	
	t_atom *list_data = x->list_data;
	short i;
	for( i = 0; i < x->shuffle_count; i++ ) {
		SETFLOAT(list_data+i,(t_float)x->shuffle_mapping[i]/2);
	}
	outlet_list(x->list_outlet,0,x->shuffle_count,list_data);
	return;
}

void disarrain_dsp(t_disarrain *x, t_signal **sp)
{
    int reset_required = 0;
    int maxvectorsize = sys_getblksize();
    int samplerate = sys_getsr();
    
    if(!samplerate)
        return;
	t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate){
		fft->R = samplerate;
	}
    if(reset_required){
        disarrain_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(disarrain_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
    }
}

