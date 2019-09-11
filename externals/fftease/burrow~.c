/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *burrow_class;

#define OBJECT_NAME "burrow~"

typedef struct _burrow
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_fftease *fft2; // for cross synthesis use
    int invert;
    t_float threshold;
    t_float multiplier;
	short mute;
 	short bypass;
} t_burrow;

void *burrow_new(t_symbol *s, int argc, t_atom *argv);
void burrow_dsp(t_burrow *x, t_signal **sp);
t_int *burrow_perform(t_int *w);
t_int *offset_perform(t_int *w);
t_int *burrow_perform(t_int *w);
void burrow_assist(t_burrow *x, void *b, long m, long a, char *s);
void burrow_float(t_burrow *x, t_floatarg myFloat);
void burrow_init(t_burrow *x);
void burrow_free(t_burrow *x);
void burrow_invert(t_burrow *x, t_floatarg toggle);
void burrow_mute(t_burrow *x, t_floatarg toggle);
void burrow_fftinfo(t_burrow *x);
void burrow_tilde_setup(void);
void burrow_winfac(t_burrow *x, t_floatarg f);
void do_burrow(t_burrow *x);
void burrow_bypass(t_burrow *x, t_floatarg toggle);
void burrow_perform64(t_burrow *x, t_object *dsp64, t_float **ins,
                      long numins, t_float **outs,long numouts, long vectorsize,
                      long flags, void *userparam);
void burrow_dsp64(t_burrow *x, t_object *dsp64, short *count, t_float samplerate, long maxvectorsize, long flags);

void burrow_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("burrow~"), (t_newmethod)burrow_new,
                  (t_method)burrow_free,sizeof(t_burrow), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_burrow, x_f);
	class_addmethod(c,(t_method)burrow_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)burrow_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)burrow_fftinfo,gensym("fftinfo"),0);
    class_addmethod(c,(t_method)burrow_invert,gensym("invert"), A_FLOAT, 0);
    burrow_class = c;
    fftease_announce(OBJECT_NAME);
}

void burrow_free(t_burrow *x)
{
	fftease_free(x->fft);
    fftease_free(x->fft2);
}

void burrow_invert(t_burrow *x, t_floatarg toggle)
{
	x->invert = (short)toggle;
}

void burrow_mute(t_burrow *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void burrow_bypass(t_burrow *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void burrow_winfac(t_burrow *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	burrow_init(x);
}


void burrow_fftinfo( t_burrow *x )
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void burrow_init(t_burrow *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized = fft->initialized;
	
	fftease_init(fft);
	fftease_init(fft2);

	if(!initialized){
		x->mute = 0;
		x->invert = 0;
	}
}

void *burrow_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;
	t_burrow 	*x = (t_burrow *) pd_new(burrow_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1, sizeof(t_fftease));
	x->fft2 = (t_fftease *) calloc(1, sizeof(t_fftease));

	fft = x->fft;
	fft2 = x->fft2;	
	fft->initialized = fft2->initialized = 0;
	x->threshold = 0.0;
	x->multiplier = 0.01;

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

t_int *burrow_perform(t_int *w)
{
	/* get our inlets and outlets */
	t_burrow *x = (t_burrow *) (w[1]);
	t_float *MSPInputVector1 = (t_float *)(w[2]);
	t_float *MSPInputVector2 = (t_float *)(w[3]);
	t_float *flt_threshold = (t_float *)(w[4]);
	t_float *flt_multiplier = (t_float *)(w[5]);
	t_float *MSPOutputVector = (t_float *)(w[6]);
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	t_float *internalInputVector1 = fft->internalInputVector;
	t_float *internalInputVector2 = fft2->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float *output = fft->output;
	t_float mult = fft->mult;
	
	if( fft->obank_flag )
		mult *= FFTEASE_OSCBANK_SCALAR;
		
	int
	i,j,
	invert = 0,
	threshold = 1.,
	multiplier = 1.;
	t_float
	*inputOne,
	*inputTwo,
	*bufferOne,
	*bufferTwo,
	*channelOne,
	*channelTwo;
    
	/* dereference structure  */
	
	inputOne = fft->input;
	inputTwo = fft2->input;
	bufferOne = fft->buffer;
	bufferTwo = fft2->buffer;
	channelOne = fft->channel;
	channelTwo = fft2->channel;
	multiplier = x->multiplier;
	threshold = x->threshold;
	invert = x->invert;
    
	mult = fft->mult;
    x->threshold = *flt_threshold;
    x->multiplier = *flt_multiplier;
    
	/* save some CPUs if muted */
	if(x->mute){
        for(i=0; i < MSPVectorSize; i++){
            MSPOutputVector[i] = 0.0;
        }
        return w+7;
	}
    
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
        memcpy(inputOne + (Nw - D), MSPInputVector1, D * sizeof(t_float));
        memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
        memcpy(inputTwo + (Nw - D), MSPInputVector2, D * sizeof(t_float));
        
		do_burrow(x);
        
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
            
			do_burrow(x);
            
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
            
            do_burrow(x);
 
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+7;
}

void do_burrow(t_burrow *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int i;
	int N2 = fft->N2;
	float a1, b1,
	a2, b2;
	int even, odd;
	
	/* dereference structure  */	
	
	t_float *bufferOne = fft->buffer;
	t_float *bufferTwo = fft2->buffer;
	
	t_float *channelOne = fft->channel;
	t_float *channelTwo = fft2->channel;
	t_float multiplier = x->multiplier;	
	t_float threshold = x->threshold;
	
	int invert = x->invert;


	/* apply hamming window and fold our window buffer into the fft buffer */ 
	
	fftease_fold(fft);
	fftease_fold(fft2);
	
	
	/* do an fft */ 
	
	fftease_rdft(fft, 1);
	fftease_rdft(fft2, 1);
	
	if (invert) {
		
		/* convert to polar coordinates from complex values */
		
		for ( i = 0; i <= N2; i++ ) {
			
			odd = ( even = i<<1 ) + 1;
			
			a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
			
			a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
			
			*(channelOne+even) = hypot( a1, b1 );
			*(channelOne+odd) = -atan2( b1, a1 );
			
			*(channelTwo+even) = hypot( a2, b2 );
			
			/* use simple threshold from second signal to trigger filtering */
			
			if ( *(channelTwo+even) < threshold )
				*(channelOne+even) *= multiplier;
						
		}  
	}
	
	else {
		
		/* convert to polar coordinates from complex values */
		
		for ( i = 0; i <= N2; i++ ) {
			
			odd = ( even = i<<1 ) + 1;
			
			a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
			
			a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
			
			*(channelOne+even) = hypot( a1, b1 );
			*(channelOne+odd) = -atan2( b1, a1 );
			
			*(channelTwo+even) = hypot( a2, b2 );
			
			/* use simple threshold from second signal to trigger filtering */
			
			if ( *(channelTwo+even) > threshold )
				*(channelOne+even) *= multiplier;						
		}  
	}
	
	/* convert back to complex form, read for the inverse fft */
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	
	
	/* do an inverse fft */
	
	fftease_rdft(fft, -1);
	
	/* dewindow our result */
	
	fftease_overlapadd(fft);
	
}

void burrow_dsp(t_burrow *x, t_signal **sp)
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
        burrow_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(burrow_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
    }
}
