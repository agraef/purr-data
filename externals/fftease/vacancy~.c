/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *vacancy_class;

#define OBJECT_NAME "vacancy~"

typedef struct _vacancy
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_fftease *fft2;
    int invert;
    int useRms;
    int swapPhase;
	short mute;
	t_float threshold;
} t_vacancy;


/* msp function prototypes */

void vacancy_dsp(t_vacancy *x, t_signal **sp);
t_int *vacancy_perform(t_int *w);
void *vacancy_new(t_symbol *s, int argc, t_atom *argv);
void vacancy_rms(t_vacancy *x, t_floatarg f);
void vacancy_invert(t_vacancy *x, t_floatarg f);
void vacancy_swapphase(t_vacancy *x, t_floatarg f);
void vacancy_free(t_vacancy *x);
void vacancy_mute(t_vacancy *x, t_floatarg toggle);
void vacancy_tilde_setup(void);
void vacancy_init(t_vacancy *x);

void vacancy_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("vacancy~"), (t_newmethod)vacancy_new,
                  (t_method)vacancy_free,sizeof(t_vacancy), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_vacancy, x_f);
	class_addmethod(c,(t_method)vacancy_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)vacancy_mute,gensym("mute"),A_FLOAT,0);
    
    class_addmethod(c,(t_method)vacancy_rms,gensym("rms"), A_FLOAT, 0);
    class_addmethod(c,(t_method)vacancy_invert,gensym("invert"), A_FLOAT, 0);
	class_addmethod(c,(t_method)vacancy_swapphase,gensym("swapphase"), A_FLOAT, 0);
    
    vacancy_class = c;
    fftease_announce(OBJECT_NAME);
}

void vacancy_rms(t_vacancy *x, t_floatarg f)
{
	x->useRms = (int) f;
}

void vacancy_invert(t_vacancy *x, t_floatarg f)
{
	x->invert = (int) f;
}

void vacancy_swapphase(t_vacancy *x, t_floatarg f)
{
	x->swapPhase = (int) f;
}

void vacancy_mute(t_vacancy *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void *vacancy_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;	

	t_vacancy *x = (t_vacancy *)pd_new(vacancy_class);

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

void vacancy_init(t_vacancy *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized = x->fft->initialized;

	fftease_init(fft);
	fftease_init(fft2);
	
	if(!initialized){
		x->mute = 0;
		x->invert = 0;
		x->threshold = 0.01;
		x->useRms = 1;
		x->swapPhase = 0;
	}
}

void vacancy_free(t_vacancy *x)
{
	fftease_free(x->fft);
    fftease_free(x->fft2);
    free(x->fft);
    free(x->fft2);
}

void do_vacancy(t_vacancy *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int	
	i,

	even, odd;
	float
	useme,
	rms = 0.,
	a1, b1,
	a2, b2;

	
	/* dereference structure  */	
	
	t_float *bufferOne = fft->buffer;
	t_float *bufferTwo = fft2->buffer;
	int N2 = fft->N2;
	int Nw = fft->Nw;
	t_float *channelOne = fft->channel;
	t_float *channelTwo = fft2->channel;
	t_float *inputOne = fft->input;
	int invert = x->invert;
	int useRms = x->useRms;
	int swapPhase = x->swapPhase;
	

	
	if (useRms) {
		
		rms = 0.;
		
		for ( i=0; i < Nw; i++ )
			rms += *(inputOne+i) * *(inputOne+i);
		
		rms = sqrt( rms / Nw );
		
		useme = rms * x->threshold;
	}
	
	else
		useme = x->threshold;
	
	fftease_fold(fft);
	fftease_fold(fft2);
	
	fftease_rdft(fft,FFT_FORWARD);
	fftease_rdft(fft2,FFT_FORWARD);
	

	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
		b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
		
		a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
		b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
		
		*(channelOne+even) = hypot( a1, b1 );
		*(channelOne+odd) = -atan2( b1, a1 );
		
		*(channelTwo+even) = hypot( a2, b2 );
		*(channelTwo+odd) = -atan2( b2, a2 ); 
	}
	
	


	if (invert) {
		
		if (swapPhase) {
			
			for ( i=0; i < N2; i+=2 ) {
				if ( *(channelOne+i) > useme && *(channelTwo+i) < *(channelOne+i) ) {
					*(channelOne+i) = *(channelTwo+i);
					*(channelOne+i+1) = *(channelTwo+i+1);
				}
			}
		}
		
		else {
			
			for ( i=0; i < N2; i+=2 ) {
				if ( *(channelOne+i) > useme && *(channelTwo+i) < *(channelOne+i) ) {
					*(channelOne+i) = *(channelTwo+i);
					
					if ( *(channelOne+i+1) == 0. )
						*(channelOne+i+1) = *(channelTwo+i+1);
				}
			}
		}
	}
	
	else {
		
		if (swapPhase) {
			
			for ( i=0; i < N2; i+=2 ) {
				if ( *(channelOne+i) < useme && *(channelTwo+i) > *(channelOne+i) ) {
					*(channelOne+i) = *(channelTwo+i);
					*(channelOne+i+1) = *(channelTwo+i+1);
				}
			}
		}
		
		else {
			
			for ( i=0; i < N2; i+=2 ) {
				
				if ( *(channelOne+i) < useme && *(channelTwo+i) > *(channelOne+i) ) {
					*(channelOne+i) = *(channelTwo+i);
					
					if ( *(channelOne+i+1) == 0. )
						*(channelOne+i+1) = *(channelTwo+i+1);
				}
			}
		}
	}
	

	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	

	
	fftease_rdft(fft,FFT_INVERSE);
	fftease_overlapadd(fft);
	
}

t_int *vacancy_perform(t_int *w)
{
	
	int	
	i,j;
	
	/* get our inlets and outlets */
	t_vacancy *x = (t_vacancy *) (w[1]);
	t_float *MSPInputVector1 = (t_float *)(w[2]);
	t_float *MSPInputVector2 = (t_float *)(w[3]);
	t_float *vec_threshold = (t_float *)(w[4]);
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
	t_float  *inputTwo = fft2->input;
	t_float  *output = fft->output;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float mult = fft->mult;		
	
	if(x->mute){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+6;
	}

	x->threshold = *vec_threshold;
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
        memcpy(inputOne + (Nw - D), MSPInputVector1, D * sizeof(t_float));
        memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
        memcpy(inputTwo + (Nw - D), MSPInputVector2, D * sizeof(t_float));
        
		do_vacancy(x);
        
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
            
			do_vacancy(x);
            
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
            
            do_vacancy(x);
            
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}		

void vacancy_dsp(t_vacancy *x, t_signal **sp)
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
        vacancy_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(vacancy_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,sp[3]->s_vec);
    }
}
