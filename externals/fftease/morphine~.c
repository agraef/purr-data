/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"
/*
 This external links to qsortE, so unlike others in this collection, morphine~ is covered under the GNU GPL.
 */
static t_class *morphine_class;

#define OBJECT_NAME "morphine~"

typedef struct _pickme {
	
	int		bin;
	float		value;
	
} pickme;


typedef struct _morphine
{

    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_fftease *fft2;
    pickme *picks;
    pickme *mirror;
    t_float morphIndex;
    t_float exponScale;	
	short mute;
} t_morphine;

void morphine_dsp(t_morphine *x, t_signal **sp);
t_int *morphine_perform(t_int *w);
void *morphine_new(t_symbol *s, int argc, t_atom *argv);
int sortIncreasing( const void *a, const void *b );
int qsortE (char *base_ptr, int total_elems, int size, int (*cmp)(const void *a, const void *b));
void morphine_transition(t_morphine *x, t_floatarg f);
void morphine_free(t_morphine *x);
void morphine_mute(t_morphine *x, t_floatarg toggle);
void morphine_tilde_setup(void);
void morphine_init(t_morphine *x);

int sortIncreasing( const void *a, const void *b )
{
	
	if ( ((pickme *) a)->value > ((pickme *) b)->value )
		return 1;
	
	if ( ((pickme *) a)->value < ((pickme *) b)->value )
		return -1;
	
	return 0;
}

void morphine_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("morphine~"), (t_newmethod)morphine_new,
                  (t_method)morphine_free,sizeof(t_morphine), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_morphine, x_f);
	class_addmethod(c,(t_method)morphine_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)morphine_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)morphine_transition,gensym("transition"), A_FLOAT, 0);
    morphine_class = c;
    fftease_announce(OBJECT_NAME);
}

void morphine_transition(t_morphine *x, t_floatarg f)
{	
	x->exponScale = f;
}

void *morphine_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;	
	t_morphine *x = (t_morphine *)pd_new(morphine_class);

	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
    
	x->fft = (t_fftease *)  calloc(1,sizeof(t_fftease));
	x->fft2 = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
	fft->initialized = 0;
	fft2->initialized = 0;
	x->exponScale = -5.0;	

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

void morphine_init(t_morphine *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized = fft->initialized;
    
	fftease_init(fft);
	fftease_init(fft2);

	if(!initialized){
		x->morphIndex = 0.;
		x->mute = 0;
		x->picks = (pickme *) calloc((fft->N2+1), sizeof(pickme));
		x->mirror = (pickme *) calloc((fft->N2+1), sizeof(pickme));
	} else if(x->fft->initialized == 1) {
		x->picks = (pickme *) realloc(x->picks, (fft->N2+1) * sizeof(pickme));
		x->mirror = (pickme *) realloc(x->mirror, (fft->N2+1) * sizeof(pickme));
	}
}

void do_morphine(t_morphine *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int	i;
	int lookupIndex,even, odd;
	t_float mult,
	morphIndex,
	exponScale,
	a1, b1,
	a2, b2;

	t_float *bufferOne = fft->buffer;
	t_float *bufferTwo = fft2->buffer;
	t_float *channelOne = fft->channel;
	t_float *channelTwo = fft2->channel;
	int N2 = fft->N2;
	pickme	*picks = x->picks;
	pickme *mirror = x->mirror;
	mult = fft->mult;	
	morphIndex = x->morphIndex;
	exponScale = x->exponScale;

	fftease_fold(fft);
	fftease_fold(fft2);
	
	/* do an fft */ 
	
	fftease_rdft(fft,1);
	fftease_rdft(fft2,1);
	
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
		*(channelTwo+odd) = -atan2( b2, a2 );
		
		
		/* find amplitude differences between home and visitors */
		
		(picks+i)->value = fabs( *(channelOne+even) - 
								*(channelTwo+even) );
		(picks+i)->bin = i;  
    }
	
	/* sort our differences in ascending order */
	
	
	qsortE( (char *) picks, (int) N2+1, (int) sizeof(pickme),
		   sortIncreasing );
	
	/* now we create an effective mirror of the sorted distribution.
	 we will assure that the initial transition will be made from
	 small spectral differences (when the sort behavior is increasing)
	 and the ending transition will also be made from small spectral
	 differences */
	
	for ( i=0; i <= N2; i += 2 ) {
        (mirror+(i/2))->bin = (picks+i)->bin;
        (mirror+(i/2))->value = (picks+i)->value;
	}
	
	for ( i=1; i <= N2; i += 2 ) {
        (mirror+(N2-(i/2)))->bin = (picks+i)->bin;
        (mirror+(N2-(i/2)))->value = (picks+i)->value;
	}
	
	
	/* calculate our morphIndex from an exponential function based on exponScale */
	
	if (exponScale == 0.) 
		lookupIndex = (int) (( (float) N2 ) * morphIndex);
	
	else {
		
     	if ( morphIndex < .5 ) {
			
			lookupIndex = (int) ( ((float) N2) * ((
												   (1. - exp( exponScale * morphIndex * 2. )) /
												   (1. - exp( exponScale )) ) * .5) );
    	}
		
    	else {
			
			lookupIndex = (int) ( ((float) N2) * ( .5 +  
												  (( (1. - exp( -exponScale * (morphIndex - .5) * 2. )) /
													(1. - exp( -exponScale )) ) * .5) ) );
    	}			 
		
	}
	
	
	//      post("%d", lookupIndex);
	
	/* choose the bins that are least different first */
	
    for ( i=0; i <= lookupIndex; i++ ) {
		
		even = ((mirror+i)->bin)<<1,
		odd = (((mirror+i)->bin)<<1) + 1;	
		
		*(channelOne+even) = *(channelTwo+even);
		*(channelOne+odd) = *(channelTwo+odd);
    }
	
	/* convert back to complex form, read for the inverse fft */
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	fftease_rdft(fft,-1);
	fftease_overlapadd(fft);
}


t_int *morphine_perform(t_int *w)
{

	int i, j;
    t_morphine *x = (t_morphine *) (w[1]);
	t_float *MSPInputVector1 = (t_float *)(w[2]);
	t_float *MSPInputVector2 = (t_float *)(w[3]);
	t_float *vec_morphIndex = (t_float *)(w[4]);
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
	t_float morphIndex = x->morphIndex;
	
	if(x->mute){
		for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+6;
	}		

	morphIndex = *vec_morphIndex;
	if ( morphIndex < 0 )
		morphIndex = 0.;
	else {
		if ( morphIndex > 1. )
			morphIndex = 1.;
	}
	x->morphIndex = morphIndex;
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
        memcpy(inputOne + (Nw - D), MSPInputVector1, D * sizeof(t_float));
        memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
        memcpy(inputTwo + (Nw - D), MSPInputVector2, D * sizeof(t_float));
        
		do_morphine(x);
        
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
            
			do_morphine(x);
            
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
            
            do_morphine(x);
            
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}

void morphine_free(t_morphine *x)
{
    if(x->fft->initialized){
        free(x->picks);
        free(x->mirror);
    }
	fftease_free(x->fft);
	fftease_free(x->fft2);
    free(x->fft);
    free(x->fft2);
}

void morphine_mute(t_morphine *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void morphine_dsp(t_morphine *x, t_signal **sp)
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
        morphine_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(morphine_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,sp[3]->s_vec);
    }
}


