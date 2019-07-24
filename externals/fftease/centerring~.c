/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

#define OBJECT_NAME "centerring~"
#define MAX_WARP 16.0

static t_class *centerring_class;

#define OBJECT_NAME "centerring~"

typedef struct _centerring
{

	t_object x_obj;
    float x_f;
	t_fftease *fft;
    int bufferLength;
    int recalc;
    int	seed;
    t_float baseFreq;
    t_float constFreq;
    t_float bandFreq;
    t_float frameR;
    t_float *ringPhases;
    t_float *ringIncrements;
    t_float *sineBuffer;
    t_float *bufferOne;
    t_float *channelOne;
	short connected[8];
	short mute;
	short bypass;
} t_centerring;

void *centerring_new(t_symbol *s, int argc, t_atom *argv);
t_int *centerring_perform(t_int *w);
void centerring_dsp(t_centerring *x, t_signal **sp);
void centerring_dest(t_centerring *x, t_float f);
void centerring_messages(t_centerring *x, t_symbol *s, short argc, t_atom *argv);
void centerring_adjust( t_centerring *x );
void centerring_zerophases( t_centerring *x );
void centerring_randphases( t_centerring *x );
void centerring_free(t_centerring *x);
void centerring_init(t_centerring *x);
void centerring_mute(t_centerring *x, t_floatarg toggle);
void centerring_fftinfo( t_centerring *x );

void centerring_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("centerring~"), (t_newmethod)centerring_new,
                  (t_method)centerring_free,sizeof(t_centerring), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_centerring, x_f);
	class_addmethod(c,(t_method)centerring_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)centerring_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)centerring_fftinfo,gensym("fftinfo"),0);
	class_addmethod(c,(t_method)centerring_messages,gensym("seed"), A_GIMME, 0);
	class_addmethod(c,(t_method)centerring_messages,gensym("zerophases"), 0);
	class_addmethod(c,(t_method)centerring_messages,gensym("randphases"), 0);
    centerring_class = c;
    fftease_announce(OBJECT_NAME);
}

void centerring_messages(t_centerring *x, t_symbol *s, short argc, t_atom *argv)
{	
	
	if (s == gensym("seed"))
		x->seed = (int) atom_getfloatarg(0,argc,argv);
	
	if (s == gensym("zerophases")) 
		centerring_zerophases( x );
	
	if (s == gensym("randphases"))
		centerring_randphases( x );
}

void *centerring_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;

	t_centerring *x = (t_centerring *)pd_new(centerring_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	/* optional arguments: baseFreq, bandFreq, constFreq, seed, overlap, winfac */
	x->baseFreq = 1.0;
	x->bandFreq = 0.2;
	x->constFreq = 1.0;
	x->seed = 1977;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void centerring_init(t_centerring *x)
{
	t_fftease *fft = x->fft;
	int Nw;
	int N;
	int N2;
	short initialized= fft->initialized;
	fftease_init(fft);
	Nw = fft->Nw;
	N = fft->N;
	N2 = fft->N2;
	if(! fftease_msp_sanity_check(fft,OBJECT_NAME)){
		post("failed sanity check!");
		return;
	}

	x->frameR = (float) fft->R / (float) fft->D;

	if(!initialized){
		x->mute = 0;
		x->bufferLength = 131072;
		x->recalc = 0;
		x->ringPhases = (t_float *) calloc((N2 + 1), sizeof(t_float));
		x->ringIncrements = (t_float *) calloc((N2 + 1), sizeof(t_float));
		x->sineBuffer = (t_float *) calloc((x->bufferLength + 1), sizeof(t_float));
		fftease_makeSineBuffer(x->sineBuffer, x->bufferLength);
	} else {
		x->ringIncrements = (t_float *)realloc((void *)x->ringIncrements, (N2 + 1) * sizeof(t_float));
		x->ringPhases = (t_float *)realloc((void *)x->ringPhases, (N2 + 1) * sizeof(t_float));
	}
	centerring_adjust(x);
	centerring_zerophases(x); 
}

void centerring_free(t_centerring *x)
{

	fftease_free(x->fft);
    free(x->fft);
	free(x->ringPhases);
	free(x->ringIncrements);
	free(x->sineBuffer);
}

void centerring_adjust( t_centerring *x ) {
	
	int		i;
	t_float	*ringIncrements = x->ringIncrements;
	int N2 = x->fft->N2;
	
	if(x->frameR == 0){
		post("centerring_adjust got at 0 SR!");
		return;
	}
    for (i=0; i < N2; i++) {

		*(ringIncrements+i) = 
		fftease_frequencyToIncrement(
			x->frameR, 
			x->baseFreq * ((fftease_rrand(&(x->seed)) * x->bandFreq) + x->constFreq ),
			x->bufferLength
		);
    }
}


void centerring_zerophases( t_centerring *x ) {
	
	int	i;
	
	for (i=0; i < x->fft->N2; i++)
		*((x->ringPhases)+i) = 0.;
}


void centerring_randphases( t_centerring *x ) {
	
	int	i;
	
	for (i=0; i < x->fft->N2; i++)
		*((x->ringPhases)+i) = fftease_prand(&(x->seed)) * (float) (x->bufferLength);
	
}

void do_centerring(t_centerring *x)
{
	t_fftease *fft = x->fft;
	t_float *buffer = fft->buffer;
	t_float *channel = fft->channel;
	int i, odd, even;
	t_float a1,b1;
	int N2 = fft->N2;
	int bufferLength = x->bufferLength;
	t_float *ringPhases = x->ringPhases;
	t_float *ringIncrements = x->ringIncrements;
	t_float *sineBuffer = x->sineBuffer;
	
	/* recalculate our oscillator values if object inputs have been updated */
	
	if (x->recalc)
		centerring_adjust( x );
	
	x->recalc = 0;
	
	fftease_fold(fft);
	fftease_rdft(fft,1);
	
	/* convert to polar coordinates from complex values */ 
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		a1 = ( i == N2 ? *(buffer+1) : *(buffer + even) );
		b1 = ( i == 0 || i == N2 ? 0. : *(buffer + odd) );
		*(channel + even) = hypot( a1, b1 );
		*(channel + odd) = -atan2( b1, a1 );      
	}
	
	
	/* perform ring modulation on successive fft frames */
	
	for (i=0; i < N2; i++) {
		even = i<<1;
		
		*(channel + even) *= fftease_bufferOscil( ringPhases+i,
							  *(ringIncrements+i), sineBuffer, bufferLength );
	}
	
	/* convert from polar to cartesian */	
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(buffer + even) = *(channel + even) * cos( *(channel + odd) );
		
		if ( i != N2 )
			*(buffer + odd) = (*(channel + even)) * -sin( *(channel + odd) );
	}
	fftease_rdft(fft,-1);
	fftease_overlapadd(fft);
}

t_int *centerring_perform(t_int *w)
{

	int i, j;
	
    t_centerring *x = (t_centerring *) (w[1]);
    t_fftease *fft = x->fft;
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *vec_baseFreq = (t_float *)(w[3]);
	t_float *vec_bandFreq = (t_float *)(w[4]);
	t_float *vec_constFreq = (t_float *)(w[5]);
	t_float *MSPOutputVector = (t_float *)(w[6]);
	t_float *input = fft->input;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float *output = fft->output;
	float mult = fft->mult ;
	int MSPVectorSize = fft->MSPVectorSize;
	t_float *internalInputVector = fft->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;		
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;	

	if(x->mute){
		for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
        return w+7;
	}
    x->recalc = 1;
    x->baseFreq = *vec_baseFreq;
    x->bandFreq = *vec_bandFreq;
    x->constFreq = *vec_constFreq;

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_centerring(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_centerring(x);
			
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
            
			do_centerring(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+7;
}

void centerring_mute(t_centerring *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void centerring_bypass(t_centerring *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void centerring_overlap(t_centerring *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	centerring_init(x);
}

void centerring_winfac(t_centerring *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	centerring_init(x);
}

void centerring_fftsize(t_centerring *x, t_floatarg f)
{
	x->fft->N = (int) f;
	centerring_init(x);
}

void centerring_fftinfo( t_centerring *x )
{
	fftease_fftinfo( x->fft, OBJECT_NAME );	
}
void centerring_dsp(t_centerring *x, t_signal **sp)
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
        centerring_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(centerring_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
                sp[4]->s_vec);
    }
}


