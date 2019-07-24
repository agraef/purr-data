/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *disarray_class;

#define OBJECT_NAME "disarray~"


typedef struct _disarray
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
    t_float top_frequency;
	int *shuffle_in;
    int *shuffle_out;
    int shuffle_count;
    int max_bin;
	void *list_outlet;
	t_atom *list_data;
	short mute;
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_disarray;

void *disarray_new(t_symbol *msg, short argc, t_atom *argv);
void disarray_dsp(t_disarray *x, t_signal **sp);
t_int *disarray_perform(t_int *w);
void disarray_switch_count (t_disarray *x, t_floatarg i);
void disarray_topfreq (t_disarray *x, t_floatarg freq);
void disarray_fadetime (t_disarray *x, t_floatarg f);
void reset_shuffle( t_disarray *x );
void disarray_showstate( t_disarray *x );
void disarray_list (t_disarray *x, t_symbol *msg, short argc, t_atom *argv);
void disarray_setstate (t_disarray *x, t_symbol *msg, short argc, t_atom *argv);
void disarray_isetstate (t_disarray *x, t_symbol *msg, short argc, t_atom *argv);
int rand_index(int max);
void disarray_mute(t_disarray *x, t_floatarg toggle);
void copy_shuffle_array(t_disarray *x);
void interpolate_frames_to_channel(t_disarray *x);
void disarray_killfade(t_disarray *x);
void disarray_forcefade(t_disarray *x, t_floatarg toggle);
void disarray_init(t_disarray *x);
void disarray_free(t_disarray *x);
void disarray_fftinfo(t_disarray *x);
void disarray_force_switch(t_disarray *x, t_floatarg toggle);
void iswitch_count(t_disarray *x, t_int i);
void switch_count (t_disarray *x, t_floatarg i);

void disarray_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("disarray~"), (t_newmethod)disarray_new,
                  (t_method)disarray_free,sizeof(t_disarray), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_disarray, x_f);
	class_addmethod(c,(t_method)disarray_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)disarray_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)reset_shuffle, gensym("bang"), 0);
	class_addmethod(c,(t_method)disarray_showstate,gensym("showstate"),0);
	class_addmethod(c,(t_method)disarray_setstate, gensym("setstate"), A_GIMME, 0);
	class_addmethod(c,(t_method)disarray_topfreq, gensym("topfreq"), A_FLOAT, 0);
	class_addmethod(c,(t_method)switch_count, gensym("switch_count"), A_FLOAT, 0);
    disarray_class = c;
    fftease_announce(OBJECT_NAME);
}

void iswitch_count(t_disarray *x, t_int i)
{
	switch_count(x,(t_floatarg)i);
}

void switch_count (t_disarray *x, t_floatarg i)
{
	if( i < 0 ){
        i = 0;
	}
	if( i > x->fft->N2 ) {
		i = x->fft->N2;
	}
	x->shuffle_count = i;
}

void disarray_free(t_disarray *x)
{
	fftease_free(x->fft);
    free(x->fft);
	free(x->list_data);
	free(x->shuffle_in);
	free(x->shuffle_out);
}

void disarray_init(t_disarray *x )
{
	t_float curfreq;
	t_fftease *fft = x->fft;
	t_float c_fundamental;
 	fftease_init(fft);
	
	int N2 = fft->N2;
	int N = fft->N;
	short initialized = fft->initialized;
	
	c_fundamental = fft->c_fundamental;
	if(initialized == 0){
		x->mute = 0;
		x->bypass = 0;
		x->list_data = (t_atom *) calloc((N+2), sizeof(t_atom)) ;
		x->shuffle_in = (int *) calloc(N2, sizeof(int));
		x->shuffle_out = (int *) calloc(N2, sizeof(int));
	} else if (initialized == 1) {
		x->list_data = (t_atom *)realloc(x->list_data, (N+2) * sizeof(t_atom));
		x->shuffle_in = (int *) realloc(x->shuffle_in, N2 * sizeof(int));
		x->shuffle_out = (int *) realloc(x->shuffle_out, N2 * sizeof(int));
	}
	
	if(initialized != 2){
		if( x->top_frequency < c_fundamental || x->top_frequency > 20000) {
			x->top_frequency = 20000.0 ;
		}
		x->max_bin = 1;
		curfreq = 0;
		while( curfreq < x->top_frequency ) {
			++(x->max_bin);
			curfreq += c_fundamental ;
		}
		reset_shuffle(x); // set shuffle lookup
		x->shuffle_count = 0;
	}
}

void disarray_topfreq (t_disarray *x, t_floatarg freq)
{
	t_float funda = (t_float) x->fft->R / (t_float) x->fft->N;
	t_float curfreq;
	
	if( freq < funda || freq > 20000) {
        post("freq %f is out of range", freq);
        return;
	}
    if(! x->fft->initialized){
        return;
    }
	x->max_bin = 1;
	curfreq = 0;
	while( curfreq < freq ) {
		++(x->max_bin);
		curfreq += funda ;
	}
}

void *disarray_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft;
    
	t_disarray *x = (t_disarray *)pd_new(disarray_class);
	outlet_new(&x->x_obj, gensym("signal"));
    x->list_outlet = outlet_new(&x->x_obj, gensym("list"));
    
	srand(time(0));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;

	fft->initialized = 0;
	x->top_frequency = 15000;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}


void disarray_mute(t_disarray *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void disarray_bypass(t_disarray *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void disarray_fftsize(t_disarray *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->N = (int) f;
	disarray_init(x);
}

void disarray_overlap(t_disarray *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->overlap = (int) f;
	disarray_init(x);
}

void disarray_winfac(t_disarray *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->winfac = (int) f;
	disarray_init(x); /* calling lighter reinit routine */
}

void disarray_fftinfo( t_disarray *x )
{
	fftease_fftinfo( x->fft, OBJECT_NAME );
}

void do_disarray(t_disarray *x)
{
	t_fftease *fft = x->fft;
	t_float *channel = fft->channel;
	int		i;
	t_float tmp;
	int shuffle_count = x->shuffle_count;
    int *shuffle_in = x->shuffle_in;
    int *shuffle_out = x->shuffle_out;
  	
	fftease_fold(fft);
	fftease_rdft(fft,1);
	fftease_leanconvert(fft);
	for( i = 0; i < shuffle_count ; i++){
		tmp = channel[ shuffle_in[ i ] * 2 ];
		channel[ shuffle_in[ i ] * 2]  = channel[ shuffle_out[ i ] * 2];
		channel[ shuffle_out[ i ] * 2]  = tmp;
	}
	fftease_leanunconvert(fft);
	fftease_rdft(fft,-1);
	fftease_overlapadd(fft);
}


t_int *disarray_perform(t_int *w)
{
	int i,j;
	t_disarray *x = (t_disarray *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *MSPOutputVector = (t_float *)(w[3]);
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
		return w+4;
	}

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_disarray(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_disarray(x);
			
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
            
			do_disarray(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+4;
}


void reset_shuffle (t_disarray *x)
{
    int i;
    int temp, p1, p2;
    int max;
    
    //post("max bin %d",x->max_bin);
    max = x->max_bin;
	for( i = 0; i < x->fft->N2; i++ ) {
		x->shuffle_out[i] = x->shuffle_in[i] = i ;
	}
	
	for( i = 0; i < 10000; i++ ) {
		p1 = x->shuffle_out[ rand_index( max ) ];
		p2 = x->shuffle_out[ rand_index( max ) ];
		temp = x->shuffle_out[ p1 ];
		x->shuffle_out[ p1 ] = x->shuffle_out[ p2 ];
		x->shuffle_out[ p2 ] = temp;
	}
	
}

int rand_index(int max) {
	
	return (rand() % max);
}



void disarray_list (t_disarray *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int ival;
	x->shuffle_count = argc;
	for (i=0; i < argc; i++) {
        
        ival = (int)atom_getfloatarg(i,argc,argv);
        
        
		if ( ival < x->fft->N2 ) {
			x->shuffle_out[ i ] = ival;
		} else {
			post ("%d out of range",ival);
		}
	}
}

void disarray_setstate (t_disarray *x, t_symbol *msg, short argc, t_atom *argv) {
    short i;
    int ival;
    
    x->shuffle_count = argc;
    for (i=0; i < argc; i++) {
        ival = atom_getfloatarg(i,argc,argv);
        
        if ( ival < x->fft->N2 && ival >= 0) {
            x->shuffle_out[ i ] = ival;
        } else {
            error("%s: %d is out of range",OBJECT_NAME, ival);
        }
    }
}

void disarray_showstate (t_disarray *x ) {
    
    t_atom *list_data = x->list_data;
    
    short i;
    // post("showstate: %d", x->shuffle_count);
    for( i = 0; i < x->shuffle_count; i++ ) {
        SETFLOAT(list_data+i,(t_float)x->shuffle_out[i]);
        // post(x->shuffle_out[i]);
    }
    outlet_list(x->list_outlet,0,x->shuffle_count,list_data);
}

void disarray_dsp(t_disarray *x, t_signal **sp)
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
        disarray_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(disarray_perform, 3, x, sp[0]->s_vec, sp[1]->s_vec);
    }
}

