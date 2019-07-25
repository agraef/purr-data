/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *shapee_class;

#define OBJECT_NAME "shapee~"

typedef struct _shapee
{
	t_object x_obj;
    float x_f;
	t_fftease *fft,*fft2;
    int widthConnected;    
    t_float shapeWidth;
	short mute;
} t_shapee;


/* msp function prototypes */

void *shapee_new(t_symbol *s, int argc, t_atom *argv);
void shapee_init(t_shapee *x);
void shapee_mute(t_shapee *x, t_floatarg state);
void shapee_free(t_shapee *x);
void shapee_dsp(t_shapee *x, t_signal **sp);
t_int *shapee_perform(t_int *w);
void shapee_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("shapee~"), (t_newmethod)shapee_new,
                  (t_method)shapee_free,sizeof(t_shapee), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_shapee, x_f);
	class_addmethod(c,(t_method)shapee_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)shapee_mute,gensym("mute"),A_FLOAT,0);
    shapee_class = c;
    fftease_announce(OBJECT_NAME);
}

void *shapee_new(t_symbol *s, int argc, t_atom *argv)
{

	t_fftease *fft, *fft2;		
	t_shapee *x = (t_shapee *)pd_new(shapee_class);

	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	x->fft2 = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
	fft->initialized = 0;
	fft2->initialized = 0;
	/* INITIALIZATIONS */
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	fft2->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = FFTEASE_DEFAULT_WINFAC;
	
	fft2->R = fft->R = sys_getsr();
	fft2->MSPVectorSize = fft->MSPVectorSize = sys_getblksize();
	x->shapeWidth = 2.0;
    if(argc > 0){ fft->N = fft2->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = fft2->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void shapee_init(t_shapee *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized = fft->initialized;
	
	fftease_init(fft);
	fftease_init(fft2);
	if(!initialized){
		x->mute = 0;
	}
}

void shapee_mute(t_shapee *x, t_floatarg state)
{
	x->mute = (short)state;
}

void do_shapee(t_shapee *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int		
	i,j,

	R,
	N,
	N2,
	D,
	Nw,
	remainingWidth,
	even, odd;

	
	t_float
	a1, b1,
	a2, b2,
	*bufferOne,
	*bufferTwo,
	*channelOne,
	*channelTwo;
	

int	shapeWidth = (int) x->shapeWidth;

	bufferOne = fft->buffer;
	bufferTwo = fft2->buffer;
	R = fft->R;
	N = fft->N;
	N2 = fft->N2;
	D = fft->D;
	Nw = fft->Nw;

	channelOne = fft->channel;
	channelTwo = fft2->channel;

    
	
	if(shapeWidth < 1 || shapeWidth > N2)
		shapeWidth = 1;

	/* apply hamming window and fold our window buffer into the fft buffer */ 
	
	fftease_fold(fft);
	fftease_fold(fft2);
	
	/* do an fft */ 
	
	fftease_rdft(fft,FFT_FORWARD);
	fftease_rdft(fft2,FFT_FORWARD);
	
	/* convert to polar coordinates from complex values */ 
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
		b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
		
		a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
		b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
		
		/* replace signal one's phases with those of signal two */
		
		*(channelOne+even) = hypot( a1, b1 );
		*(channelOne+odd) = -atan2( b1, a1 );
		
		*(channelTwo+even) = hypot( a2, b2 );
		*(channelTwo+odd) = -atan2( b2, a2 );       
	}
	
	/* constrain our shapeWidth value */
	
	if ( shapeWidth > N2 )
		shapeWidth = N2;
  	
	if ( shapeWidth < 1 )
		shapeWidth = 1;
	
	
	/* lets just shape the entire signal by the shape width */
	
	for ( i=0; i < N; i += shapeWidth << 1 ) {
		
		float       amplSum = 0.,
		freqSum = 0.,
		factor;
		
		for ( j = 0; j < shapeWidth << 1; j += 2 ) {
			
			amplSum += *(channelTwo+i+j);
			freqSum += *(channelOne+i+j);
		}
		if(freqSum <= 0.001){
			freqSum = 1.0;
		}
		if (amplSum < 0.000000001)
			factor = 0.000000001;	
		
		else	
			factor = amplSum / freqSum;
		
		for ( j = 0; j < shapeWidth * 2; j += 2 )
			*(channelOne+i+j) *= factor;
	}
	
	/* copy remaining magnitudes */
	
	if ( (remainingWidth = N2 % shapeWidth) ) {
		
		int			bindex = (N2 - remainingWidth) << 1;
		
		
		float       amplSum = 0.,
		freqSum = 0.,
		factor;
		
		for ( j = 0; j < remainingWidth * 2; j += 2 ) {
			
	  		amplSum += *(channelTwo+bindex+j);
	  		freqSum += *(channelOne+bindex+j);
		}
		if(freqSum <= 0.00001){
			freqSum = 1.0;
		}
		if (amplSum < 0.000000001)
			factor = 0.000000001;	
		
		else	
			factor = amplSum / freqSum;
		
		for ( j = 0; j < remainingWidth * 2; j += 2 )
	  		*(channelOne+bindex+j) *= factor;
	}
	
	
	/* convert from polar to cartesian */	
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = (*(channelOne+even)) * -sin( *(channelOne+odd) );
	}

	fftease_rdft(fft,FFT_INVERSE);
	fftease_overlapadd(fft);
}


t_int *shapee_perform(t_int *w)
{
	int i,j;
	/* get our inlets and outlets */
	t_shapee *x = (t_shapee *) (w[1]);
	t_float *MSPInputVector1 = (t_float *)(w[2]);
	t_float *MSPInputVector2 = (t_float *)(w[3]);
	t_float *inShape = (t_float *)(w[4]);
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
	
	if(x->mute){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+6;
	}

	x->shapeWidth =   *inShape;

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
        memcpy(inputOne + (Nw - D), MSPInputVector1, D * sizeof(t_float));
        memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
        memcpy(inputTwo + (Nw - D), MSPInputVector2, D * sizeof(t_float));
        
		do_shapee(x);
        
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
            
			do_shapee(x);
            
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
            
            do_shapee(x);
            
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}	
	
void shapee_free( t_shapee *x )
{
	fftease_free(x->fft);
	fftease_free(x->fft2);
    free(x->fft);
    free(x->fft2);
}

void shapee_dsp(t_shapee *x, t_signal **sp)
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
        shapee_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(shapee_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,sp[3]->s_vec);
    }
}
