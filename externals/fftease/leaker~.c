/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *leaker_class;

#define OBJECT_NAME "leaker~"

typedef struct _leaker
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_fftease *fft2;
	int *sieve;
	short mute;
	t_float fade_value;
} t_leaker;

void leaker_dsp(t_leaker *x, t_signal **sp);
t_int *leaker_perform(t_int *w);
static void leaker_free(t_leaker *x);
void *leaker_new(t_symbol *msg, short argc, t_atom *argv);
void leaker_upsieve(t_leaker *x) ;
void leaker_downsieve(t_leaker *x) ;
void leaker_randsieve(t_leaker *x) ;
void leaker_mute(t_leaker *x, t_floatarg state);
void leaker_init(t_leaker *x);

void leaker_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("leaker~"), (t_newmethod)leaker_new,
                  (t_method)leaker_free,sizeof(t_leaker), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_leaker, x_f);
	class_addmethod(c,(t_method)leaker_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)leaker_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)leaker_upsieve,gensym("upsieve"), 0);
	class_addmethod(c,(t_method)leaker_downsieve,gensym("downsieve"), 0);
	class_addmethod(c,(t_method)leaker_randsieve,gensym("randsieve"), 0);
    
    leaker_class = c;
    fftease_announce(OBJECT_NAME);
}

void leaker_free( t_leaker *x ){
	fftease_free(x->fft);
	fftease_free(x->fft2);
    free(x->fft);
    free(x->fft2);
	free(x->sieve);
}

void leaker_upsieve(t_leaker *x) {
	int i;
	int *sieve = x->sieve;
	for( i = 0; i < x->fft->N2; i++ ){
		sieve[i] = i + 1;
	}
}

void leaker_downsieve(t_leaker *x) {
	int i;
	int *sieve = x->sieve;
	int N2 = x->fft->N2;
	for( i = 0; i < N2; i++ ){
		sieve[i] = N2  - i;
	}
}

void leaker_randsieve(t_leaker *x) {
	int i;
	int temp;
	int pos1, pos2;
	int N2 = x->fft->N2;
	int *sieve = x->sieve;
    int maxswap = N2 - 1;
	
	for( i = 0; i < N2; i++ ){
		sieve[i] = i + 1;
	}
    while(maxswap > 0){
        pos1 = maxswap;
        pos2 = rand() % (N2 - 1);
        temp = sieve[pos1];
        sieve[pos1] = sieve[pos2];
        sieve[pos2] = temp;
        --maxswap;
    }
}

void leaker_mute(t_leaker *x, t_floatarg state)
{
	x->mute = (short)state;	
}

void *leaker_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft, *fft2;

	t_leaker *x = (t_leaker *)pd_new(leaker_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	x->fft2 = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
		
	fft->initialized = 0;
	fft2->initialized = 0;

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	
	fft2->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = FFTEASE_DEFAULT_WINFAC;
	
	return x;
}

void leaker_init(t_leaker *x)
{
	int i;
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized = fft->initialized;
	
	fftease_init(fft);
	fftease_init(fft2);

	if(!initialized) {
		x->mute = 0;
		x->fade_value = 0;
		x->sieve = (int *) calloc((fft->N2 + 1),sizeof(int));
	}
	if(initialized != 2){
		for(i = 0; i < fft->N2; i++){
			x->sieve[i] = i;
		}
	}
}

void do_leaker(t_leaker *x)
{
	int i,odd,even;
	t_float a1,a2,b1,b2;
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int N2 = fft->N2;
	t_float *buffer1 = fft->buffer;
	t_float *buffer2 = fft2->buffer;
	t_float *channel1 = fft->channel;
	int *sieve = x->sieve;
	t_float fade_value = x->fade_value;

	fftease_fold(fft);
	fftease_fold(fft2);
	fftease_rdft(fft,1);
	fftease_rdft(fft2,1);
	
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		if( fade_value <= 0 || fade_value < sieve[i]  ){
			a1 = ( i == N2 ? *(buffer1+1) : *(buffer1+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(buffer1+odd) );
			
			*(channel1+even) = hypot( a1, b1 ) ;
			*(channel1+odd) = -atan2( b1, a1 );
			*(buffer1+even) = *(channel1+even) * cos(*(channel1+odd));
			if ( i != N2 ){
				*(buffer1+odd) = -(*(channel1+even)) * sin(*(channel1+odd));
			}
		} else {
			a2 = ( i == N2 ? *(buffer2+1) : *(buffer2+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(buffer2+odd) );
			*(channel1+even) = hypot( a2, b2 ) ;
			*(channel1+odd) = -atan2( b2, a2 );
			*(buffer1+even) = *(channel1+even) * cos(*(channel1+odd) );
			if ( i != N2 ){
				*(buffer1+odd) = -(*(channel1+even)) * sin( *(channel1+odd) );
			}
		}
	}
	fftease_rdft(fft,-1);
	fftease_overlapadd(fft);
}


t_int *leaker_perform(t_int *w)
{
	int i,j;
    t_leaker *x = (t_leaker *) (w[1]);
	t_float *MSPInputVector1 = (t_float *)(w[2]);
	t_float *MSPInputVector2 = (t_float *)(w[3]);
	t_float *fade_value = (t_float *)(w[4]);
	t_float *MSPOutputVector = (t_float *)(w[5]);
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	t_float *internalInputVector1 = fft->internalInputVector;
	t_float *internalInputVector2 = fft2->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;
	t_float *inputOne = fft->input;
	t_float *inputTwo = fft2->input;
	t_float *output = fft->output;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float mult = fft->mult;	
	int N2 = fft->N2;
	
    x->fade_value = *fade_value * (float) N2;

	if(x->mute){
		for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+6;
	}

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
        memcpy(inputOne + (Nw - D), MSPInputVector1, D * sizeof(t_float));
        memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
        memcpy(inputTwo + (Nw - D), MSPInputVector2, D * sizeof(t_float));
        
		do_leaker(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
            memcpy(inputOne + (Nw-D), MSPInputVector1 + (D*i), D * sizeof(t_float));
            memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
            memcpy(inputTwo + (Nw-D), MSPInputVector2 + (D*i), D * sizeof(t_float));
            
			do_leaker(x);
            
			for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
            memcpy(output, output + D, (Nw-D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        memcpy(internalInputVector1 + (operationCount * MSPVectorSize), MSPInputVector1, MSPVectorSize * sizeof(t_float));
        memcpy(internalInputVector2 + (operationCount * MSPVectorSize), MSPInputVector2, MSPVectorSize * sizeof(t_float));
        memcpy(MSPOutputVector, internalOutputVector + (operationCount * MSPVectorSize), MSPVectorSize * sizeof(t_float));
        
		operationCount = (operationCount + 1) % operationRepeat;
        
        if( operationCount == 0 ) {
            memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
            memcpy(inputOne + (Nw - D), internalInputVector1, D * sizeof(t_float));
            memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
            memcpy(inputTwo + (Nw - D), internalInputVector2, D * sizeof(t_float));
            
            do_leaker(x);
            
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}		

void leaker_dsp(t_leaker *x, t_signal **sp)
{
    int reset_required = 0;
    int maxvectorsize = sys_getblksize();
    int samplerate = sys_getsr();
    
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
    
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
		fft2->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft2);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
        fft2->R = samplerate;
	}
    if(reset_required){
        leaker_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(leaker_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,sp[3]->s_vec);
    }
}


