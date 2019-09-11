/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *ether_class;

#define OBJECT_NAME "ether~"


/* Added a new inlet for the composite index */

typedef struct _ether
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_fftease *fft2;	
    int invert;
    t_float threshMult;
	short mute;
} t_ether;

void ether_dsp(t_ether *x, t_signal **sp);
t_int *ether_perform(t_int *w);
void *ether_new(t_symbol *s, int argc, t_atom *argv);
void ether_invert(t_ether *x, t_floatarg toggle);
void ether_init(t_ether *x);
void ether_free(t_ether *x);
void ether_mute(t_ether *x, t_floatarg toggle);
void ether_tilde_setup(void);

void ether_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("ether~"), (t_newmethod)ether_new,
                  (t_method)ether_free,sizeof(t_ether), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_ether, x_f);
	class_addmethod(c,(t_method)ether_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)ether_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)ether_invert,gensym("invert"), A_FLOAT, 0);
    ether_class = c;
    fftease_announce(OBJECT_NAME);
}

void ether_free(t_ether *x)
{
	fftease_free(x->fft);
	fftease_free(x->fft2);
    free(x->fft);
    free(x->fft2);
}

void *ether_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;
	t_ether *x = (t_ether *)pd_new(ether_class);

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
    if(argc > 0){ fft->N = fft2->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = fft2->overlap = (int) atom_getfloatarg(1, argc, argv); }

	return x;
}

void ether_init(t_ether *x)
{

	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	
	short initialized = fft->initialized;
	
	fftease_init(fft);
	fftease_init(fft2);

	if(!initialized){
		x->mute = 0;
		x->invert = 0;
		x->threshMult = 0.;
	} else {
        x->fft->input = (t_float *) realloc(fft->input,fft->Nw * sizeof(t_float));
        x->fft2->input = (t_float *) realloc(fft2->input,fft2->Nw * sizeof(t_float));
        x->fft->output = (t_float *) realloc(fft->output,fft->Nw * sizeof(t_float));

	}
}

void do_ether(t_ether *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int i;
	int N2 = fft->N2;
	float a1, b1, a2, b2;
	int even, odd;
	int invert = x->invert;
	t_float threshMult = x->threshMult;
	t_float *bufferOne = fft->buffer;
	t_float *bufferTwo = fft2->buffer;
	t_float *channelOne = fft->channel;
	t_float *channelTwo = fft2->channel;
	
	fftease_fold(fft);
	fftease_fold(fft2);
	fftease_rdft(fft,1);
	fftease_rdft(fft2,1);
	
	if (invert) {	
		
		
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
						
			if ( *(channelOne+even) > *(channelTwo+even) * threshMult )
				*(channelOne+even) = *(channelTwo+even);
			
			if ( *(channelOne+odd) == 0. )
				*(channelOne+odd) = *(channelTwo+odd);	 
		}
	}
	
	else {
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
			
			
			if ( *(channelOne+even) < *(channelTwo+even) * threshMult )
				*(channelOne+even) = *(channelTwo+even);
			
			if ( *(channelOne+odd) == 0. )
				*(channelOne+odd) = *(channelTwo+odd);	 
		}  
	}
	

	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	fftease_rdft(fft, -1);
	fftease_overlapadd(fft);
}

t_int *ether_perform(t_int *w)
{
	int i,j;
    t_ether *x = (t_ether *) (w[1]);
	t_float *MSPInputVector1 = (t_float *)(w[2]);
	t_float *MSPInputVector2 = (t_float *)(w[3]);
	t_float *vec_threshMult = (t_float *)(w[4]);
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
	

    x->threshMult = *vec_threshMult;

	if ( x->threshMult == 0. ){
		x->threshMult = 0.0001;
	}
	if(x->mute){
		for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+6;
	}

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
        memcpy(inputOne + (Nw - D), MSPInputVector1, D * sizeof(t_float));
        memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
        memcpy(inputTwo + (Nw - D), MSPInputVector2, D * sizeof(t_float));
        
		do_ether(x);
        
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
            
			do_ether(x);
            
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
            
            do_ether(x);
            
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}		

void ether_mute(t_ether *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void ether_invert(t_ether *x, t_floatarg toggle)
{
	x->invert = (int)toggle;
}

void ether_dsp(t_ether *x, t_signal **sp)
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
        ether_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(ether_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,sp[3]->s_vec);
    }
}

