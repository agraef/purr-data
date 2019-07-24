/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *swinger_class;

#define OBJECT_NAME "swinger~"

typedef struct _swinger
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_fftease *fft2;
	short mute;
} t_swinger;

void swinger_dsp(t_swinger *x, t_signal **sp);
t_int *swinger_perform(t_int *w);
void *swinger_new(t_symbol *s, int argc, t_atom *argv);
void swinger_mute(t_swinger *x, t_floatarg state);
void swinger_init(t_swinger *x);
void swinger_free(t_swinger *x);

void swinger_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("swinger~"), (t_newmethod)swinger_new,
                  (t_method)swinger_free,sizeof(t_swinger), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_swinger, x_f);
	class_addmethod(c,(t_method)swinger_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)swinger_mute,gensym("mute"),A_FLOAT,0);
    swinger_class = c;
    fftease_announce(OBJECT_NAME);
}

void swinger_mute(t_swinger *x, t_floatarg state)
{
	x->mute = state;	
}

void *swinger_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;
	t_swinger *x = (t_swinger *)pd_new(swinger_class);

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
	
    if(argc > 0){ fft->N = fft2->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = fft2->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void swinger_init(t_swinger *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	
	if(!fft->initialized){
		x->mute = 0;
	}	
	fftease_init(fft);
	fftease_init(fft2);

}

void do_swinger(t_swinger *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int		
	i,
	
	R,
	N,
	N2,
	D,
	Nw,
	
	even, odd;
	
	t_float
	a1, b1,
	a2, b2,
	*bufferOne,
	*bufferTwo,
	*channelOne,
	*channelTwo;
	bufferOne = fft->buffer;
	bufferTwo = fft2->buffer;
	R = fft->R;
	N = fft->N;
	N2 = fft->N2;
	D = fft->D;
	Nw = fft->Nw;
	channelOne = fft->channel;
	channelTwo = fft2->channel;
	
	
	/* apply hamming window and fold our window buffer into the fft buffer */ 
	
	fftease_fold(fft);
	fftease_fold(fft2);
	
	/* do an fft */ 
	
	fftease_rdft(fft,FFT_FORWARD);
	fftease_rdft(fft2,FFT_FORWARD);
	
	/* use redundant coding for speed, even though moving the invert variable
	 comparison outside of the for loop will give us only a minimal performance
	 increase (hypot and atan2 are the most intensive portions of this code).
	 consider adding a table lookup for atan2 instead.
	 */
	
	/* convert to polar coordinates from complex values */
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
		b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
		
		a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
		b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
		
		/* replace signal one's phases with those of signal two */
		
		*(channelOne+even) = hypot( a1, b1 );
		*(channelOne+odd) = -atan2( b2, a2 );        
	}
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	
	
	/* do an inverse fft */
	
	fftease_rdft(fft,FFT_INVERSE);
	
	
	
	/* dewindow our result */
	
	fftease_overlapadd(fft);
	
	/* set our output and adjust our retaining output buffer */
	
	
}

t_int *swinger_perform(t_int *w)
{
	int i,j;
    t_swinger *x = (t_swinger *) (w[1]);
	t_float *MSPInputVector1 = (t_float *)(w[2]);
	t_float *MSPInputVector2 = (t_float *)(w[3]);
	t_float *MSPOutputVector = (t_float *)(w[4]);
	
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
	
	/* no computation if muted */
	
	if(x->mute){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+5;
	}
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
        memcpy(inputOne + (Nw - D), MSPInputVector1, D * sizeof(t_float));
        memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
        memcpy(inputTwo + (Nw - D), MSPInputVector2, D * sizeof(t_float));
        
		do_swinger(x);
        
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
            
			do_swinger(x);
            
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
            
            do_swinger(x);
            
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+5;
}	

void swinger_free( t_swinger *x )
{
	fftease_free(x->fft);
	fftease_free(x->fft2);
    free(x->fft);
    free(x->fft2);
}

void swinger_dsp(t_swinger *x, t_signal **sp)
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
        swinger_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(swinger_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
    }
}