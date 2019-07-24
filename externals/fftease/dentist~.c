/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *dentist_class;

#define OBJECT_NAME "dentist~"

typedef struct _dentist
{
	t_object x_obj;
    float x_f;
    t_fftease *fft;
	short *bin_selection;
	short *last_bin_selection;
	int *active_bins;
	int tooth_count;
	int ramp_frames;
	int frames_left;
	t_float frame_duration;
	int max_bin;
	t_float topfreq;
	t_float funda;
	void *list_outlet;
	short direct_update;
	short mute;
	t_atom *list_data;
	short interpolate_singles;
	t_float sync;
    t_float ramp_ms;

} t_dentist;

void *dentist_new(t_symbol *msg, short argc, t_atom *argv);
void dentist_dsp(t_dentist *x, t_signal **sp);
t_int *dentist_perform(t_int *w);
void set_switch_bins (t_dentist *x, int i);
void reset_shuffle(t_dentist *x);
void dentist_showstate(t_dentist *x);
void dentist_direct_update(t_dentist *x, t_floatarg toggle);
void dentist_mute(t_dentist *x, t_floatarg toggle);
void dentist_setstate(t_dentist *x, t_symbol *msg, short argc, t_atom *argv);
void dentist_ramptime(t_dentist *x, t_floatarg ramp_ms);
int rand_index(int max);
void dentist_init(t_dentist *x);
void dentist_bins_pd (t_dentist *x, t_floatarg i);
void dentist_topfreq(t_dentist *x, t_floatarg f);
void dentist_free(t_dentist *x);
void dentist_toothcount(t_dentist *x, t_floatarg newcount);
void dentist_scramble(t_dentist *x);
void dentist_activate_bins(t_dentist *x, t_floatarg f);
void dentist_interpolate_singles(t_dentist *x, t_floatarg f);
void dentist_fftinfo(t_dentist *x);
void dentist_mute(t_dentist *x, t_floatarg toggle);

void dentist_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("dentist~"), (t_newmethod)dentist_new,
                  (t_method)dentist_free,sizeof(t_dentist), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_dentist, x_f);
	class_addmethod(c,(t_method)dentist_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)dentist_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)dentist_fftinfo,gensym("fftinfo"),0);
	class_addmethod(c,(t_method)dentist_showstate,gensym("showstate"),0);
	class_addmethod(c,(t_method)dentist_setstate, gensym("setstate"), A_GIMME, 0);
	class_addmethod(c,(t_method)dentist_ramptime, gensym("ramptime"), A_FLOAT, 0);
	class_addmethod(c,(t_method)dentist_topfreq, gensym("topfreq"), A_FLOAT, 0);
	class_addmethod(c,(t_method)dentist_toothcount, gensym("toothcount"), A_FLOAT, 0);
	class_addmethod(c,(t_method)dentist_interpolate_singles, gensym("interpolate_singles"), A_FLOAT, 0);
	class_addmethod(c,(t_method)dentist_scramble, gensym("scramble"), 0);
    
    dentist_class = c;
    fftease_announce(OBJECT_NAME);
}


void dentist_interpolate_singles(t_dentist *x, t_floatarg f)
{
    x->interpolate_singles = (short)f;
	//    post("singles interp: %d",x->interpolate_singles);
}

void dentist_free(t_dentist *x)
{
	fftease_free(x->fft);
    free(x->fft);
    free(x->bin_selection);
    free(x->active_bins);
    free(x->last_bin_selection);
    free(x->list_data);
}

void dentist_fftsize(t_dentist *x, t_floatarg f)
{	
	t_fftease *fft = x->fft;
	fft->N = (int) f;
	dentist_init(x);
}

void dentist_overlap(t_dentist *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	dentist_init(x);
}

void dentist_winfac(t_dentist *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->winfac = (int) f;
	dentist_init(x);
}

void dentist_fftinfo( t_dentist *x )
{
	fftease_fftinfo( x->fft, OBJECT_NAME );
}


void dentist_direct_update( t_dentist *x, t_floatarg toggle)
{
	x->direct_update = (short)toggle;
}

void dentist_mute( t_dentist *x, t_floatarg toggle )
{
	x->mute = (short)toggle;
}

void *dentist_new(t_symbol *msg, short argc, t_atom *argv)
{
    t_fftease *fft;
	t_dentist *x = (t_dentist *)pd_new(dentist_class);
	
	outlet_new(&x->x_obj, gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
    x->list_outlet = outlet_new(&x->x_obj, gensym("list"));
    
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	
	x->topfreq = 3000; // an attribute candidate
	x->ramp_ms = 1000.0;
    x->tooth_count = 3;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }

	return x;
}

void dentist_topfreq(t_dentist *x, t_floatarg f)
{
	float funda = x->funda;
	float curfreq;
	t_fftease *fft = x->fft;
    if(f < 50 || f > fft->R/2.0)
        return;
	
    x->topfreq = f;
    if(! x->fft->initialized){
        return;
    }
    x->max_bin = 1;  
    curfreq = 0;
    while(curfreq < x->topfreq) {
        ++(x->max_bin);
        curfreq += funda ;
    }
}

void dentist_init(t_dentist *x)
{
	int i;
	t_fftease *fft = x->fft;
	short initialized = fft->initialized;
 	fftease_init(fft);
	   
	if(!initialized){
		x->sync = 0;
		x->mute = 0;
		x->direct_update = 0;
		if(x->topfreq < 100)
			x->topfreq = 100.0;
		x->bin_selection = (short *) calloc(fft->N, sizeof(short));
		x->active_bins = (int *) calloc(fft->N2, sizeof(int));
		x->last_bin_selection = (short *) calloc(fft->N2, sizeof(short)) ;
		x->list_data = (t_atom *) calloc((fft->N + 2), sizeof(t_atom));
		
		x->interpolate_singles = 1;
		x->ramp_frames = 0;
		
	} else {
		x->bin_selection = (short *) realloc((void *)x->bin_selection, fft->N * sizeof(short));
		x->active_bins = (int *) realloc((void *)x->active_bins, fft->N2 * sizeof(int));
		x->last_bin_selection = (short *) realloc((void *)x->last_bin_selection, fft->N2 * sizeof(short)) ;
		x->list_data = (t_atom *) realloc((void *)x->list_data, (fft->N + 2) * sizeof(t_atom));
	}

	dentist_scramble(x);
	
    fft->mult = 1. / (t_float) fft->N;
    x->frame_duration = (t_float) fft->D / (t_float) fft->R;
    x->frames_left = x->ramp_frames = (int)(x->ramp_ms * .001 / x->frame_duration);
    x->funda = (t_float) fft->R / (t_float) fft->N;
    x->max_bin = 1;  

    if(!x->funda){
    	error("%s: zero sampling rate!",OBJECT_NAME);
    	return;
    }
    x->max_bin = (int) (x->topfreq / x->funda);
    if(x->max_bin < 1)
    	x->max_bin = 1;
     
    for( i = 0; i < fft->N2; i++) {
        x->last_bin_selection[i] = x->bin_selection[i];
    }
    dentist_toothcount(x, x->tooth_count);
}

void do_dentist(t_dentist *x)
{
	int	i;
	t_float oldfrac,newfrac;
	t_fftease *fft = x->fft;
	t_float *channel = fft->channel;
	int frames_left = x->frames_left;
	int ramp_frames = x->ramp_frames;
	short *bin_selection = x->bin_selection;
	short *last_bin_selection = x->last_bin_selection;
	int N2 = fft->N2;
	float sync = x->sync;

	fftease_fold(fft);
	fftease_rdft(fft,1);
	fftease_leanconvert(fft);
	
	if(frames_left > 0 && ramp_frames > 0) {
		// INTERPOLATE ACCORDING TO POSITION IN RAMP
		oldfrac = (float) frames_left / (float) ramp_frames ;
		sync = newfrac = 1.0 - oldfrac;
		for( i = 0; i < N2 ; i++){
			if( (! bin_selection[i]) && (! last_bin_selection[i]) ){
				channel[i * 2]  = 0;
			} 
			else if (bin_selection[i]) {
				channel[i * 2]  *= newfrac;
			} 
			else if (last_bin_selection[i]) {
				channel[i * 2]  *= oldfrac;
			}
		}
		--frames_left;
		if( ! frames_left ){
			// Copy current to last
			for( i = 0; i < N2; i++) {
				last_bin_selection[i] = bin_selection[i];
			}
		}
	} else {
		for( i = 0; i < N2 ; i++){
			if( ! bin_selection[ i ] ){
				channel[ i * 2 ]  = 0;
			}
		}
		oldfrac = 0.0;
		sync = 1.0;
	}
	
	fftease_leanunconvert(fft);
	fftease_rdft(fft,-1);
	fftease_overlapadd(fft);
	x->frames_left = frames_left;
	x->sync = sync;
}

t_int *dentist_perform(t_int *w)
{
	int	i,j;
    t_dentist *x = (t_dentist *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *MSPOutputVector = (t_float *)(w[3]);
	t_float *sync_vec = (t_float *)(w[4]);
	t_fftease *fft = x->fft;
	t_float *input = fft->input;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float *output = fft->output;
	t_float mult = fft->mult ;
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
        
		do_dentist(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_dentist(x);
			
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
            
			do_dentist(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
	for(i = 0; i < MSPVectorSize; i++){
		sync_vec[i] = x->sync;
	}
	return w+5;
}		


void set_switch_bins (t_dentist *x, int i)
{
	if( i < 0 ){
		i = 0;
	}
	if( i > x->fft->N2 ) {
		i = x->fft->N2;
	}
	x->tooth_count = i;
	if( x->direct_update ){
		reset_shuffle(x);
	}
	return;
}
//identical function for Pd
void dentist_bins_pd (t_dentist *x, t_floatarg i)
{
	if( i < 0 ){
		i = 0;
	}
	if( i > x->fft->N2 ) {
		i = x->fft->N2;
	}
	x->tooth_count = (int)i;
	if(x->direct_update){
		reset_shuffle(x);
	}
	return;
}

// experimental, not to be used
void dentist_activate_bins(t_dentist *x, t_floatarg f)
{
    if(f < 0 || f > x->max_bin){
        post("* %d bin out of range",(int)f);
        return;
    }
    x->tooth_count = (int)f;
}

void dentist_scramble(t_dentist *x)
{
	short *last_bin_selection = x->last_bin_selection;
	short *bin_selection = x->bin_selection;
	int *active_bins = x->active_bins;
	int N2 = x->fft->N2;
	int i,tmp,b1,b2;
	int maxswap = x->max_bin;

    if(!x->fft->initialized){
        return;
    }
	for(i=0; i<N2; i++){
		bin_selection[i] = 0;
		active_bins[i] = i;
	}
	while(maxswap > 0){
		b1 = maxswap;
		b2 = rand_index(maxswap);
		tmp = active_bins[b1];
		active_bins[b1] = active_bins[b2];
		active_bins[b2] = tmp;
		--maxswap;
	}
	for( i = 0; i < x->tooth_count; i++ ) {
		x->bin_selection[active_bins[i]] = 1;
	}
	x->frames_left = x->ramp_frames;
	if(! x->ramp_frames) {
		for(i = 0; i < N2; i++){
			last_bin_selection[i] = bin_selection[i];
		}
	}    
}



void dentist_toothcount(t_dentist *x, t_floatarg newcount)
{
	int i;
	int nc = (int) newcount;
	int tooth_count = x->tooth_count;
    if(! x->fft->initialized){
        x->tooth_count = newcount;
        return;
    }
	if(nc < 0 || nc > x->fft->N2){
		error("dentist~: %d out of range",nc);
		return;
	}

	if(nc < x->tooth_count){
		for(i = nc; i < tooth_count; i++){
			x->bin_selection[x->active_bins[i]] = 0;
		}
	}
    else {
		for(i = tooth_count; i < nc; i++){
			x->bin_selection[x->active_bins[i]] = 1;
		}
	}
	// if immediate reset
	if(x->interpolate_singles){
		//  post("setting frames left");
		x->frames_left = x->ramp_frames;
	}
	if(! x->ramp_frames) {
		for(i = 0; i < x->fft->N2; i++){
			x->last_bin_selection[i] = x->bin_selection[i];
		}
	}
	x->tooth_count = nc;
}


void reset_shuffle (t_dentist *x)
{
	int i;
	int max;
	
	max = x->max_bin;
	for(i = 0; i < x->fft->N2; i++){
		x->last_bin_selection[i] = x->bin_selection[i];
		x->bin_selection[i] = 0;
	}
	for(i = 0; i < x->max_bin; i++) {
		x->active_bins[i] = rand_index(max);
		x->bin_selection[x->active_bins[i]] = 1;
	}
	x->frames_left = x->ramp_frames;
	if(! x->ramp_frames) { // Ramp Off - Immediately set last to current
		for( i = 0; i < x->fft->N2; i++ ){
			x->last_bin_selection[ i ] = x->bin_selection[ i ];
		}
	}
}

int rand_index(int max) {
	return (rand() % max);
}

void dentist_setstate (t_dentist *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int selex;
	
	short *last_bin_selection = x->last_bin_selection;
	short *bin_selection = x->bin_selection;
	int *active_bins = x->active_bins;
	x->tooth_count = argc;
	int N2 = x->fft->N2;
	
	for(i = 0; i < N2; i++){
		last_bin_selection[i] = bin_selection[i]; // needed here
		bin_selection[i] = 0;
	}
	
	for (i=0; i < argc; i++) {
		selex = atom_getfloatarg(i,argc,argv);
		if (selex < N2 && selex >= 0 ) {
			active_bins[i] = selex;
			bin_selection[selex] = 1;
		} else {
			post ("%d out of range bin",selex);
		}		
	}
	
	
	x->frames_left = x->ramp_frames;
	if(! x->ramp_frames) { // Ramp Off - Immediately set last to current
		for(i = 0; i < N2; i++){
			last_bin_selection[i] = bin_selection[i];
		}
	}
	
	return;
}
void dentist_ramptime (t_dentist *x, t_floatarg ramp_ms) {
	
	if(ramp_ms <= 0){
		return;
	}
    x->ramp_ms = ramp_ms;
    if(!x->fft->initialized){
        return;
    }
	x->frames_left = x->ramp_frames = (int)(x->ramp_ms * .001 / x->frame_duration);
	return;
}
// REPORT CURRENT SHUFFLE STATUS
void dentist_showstate (t_dentist *x) {
	
	t_atom *list_data = x->list_data;
	
	short i, count;
	float data;
	
	count = 0;
	for(i = 0; i < x->tooth_count; i++ ) {
		data = x->active_bins[i];
		SETFLOAT(list_data+count,(t_float)x->active_bins[i]);
		++count;
	}	
	outlet_list(x->list_outlet,0,x->tooth_count,list_data);
	return;
}

void dentist_dsp(t_dentist *x, t_signal **sp)
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
        dentist_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(dentist_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
    }
}


