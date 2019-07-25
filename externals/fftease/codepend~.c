/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *codepend_class;

#define OBJECT_NAME "codepend~"

typedef struct _codepend
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_fftease *fft2; // for cross synthesis use
	t_float threshold;
	t_float exponent;
	short connected[8];
	short mute;
	short bypass;
	int invert_countdown; // delay onset of invert effect to avoid loud glitches
	int invert_nextstate;// next state for invert   
	int invert;
	t_float invert_pad;
} t_codepend;

void *codepend_new(t_symbol *s, int argc, t_atom *argv);
void codepend_dsp(t_codepend *x, t_signal **sp);
t_int *codepend_perform(t_int *w);
void codepend_invert(t_codepend *x, t_floatarg toggle);
void codepend_free(t_codepend *x);
void codepend_mute(t_codepend *x, t_floatarg toggle);
void codepend_fftinfo(t_codepend *x);
void codepend_init(t_codepend *x);
void codepend_pad(t_codepend *x, t_floatarg pad);

void codepend_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("codepend~"), (t_newmethod)codepend_new,
                  (t_method)codepend_free,sizeof(t_codepend), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_codepend, x_f);
	class_addmethod(c,(t_method)codepend_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)codepend_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)codepend_fftinfo,gensym("fftinfo"),0);
    class_addmethod(c,(t_method)codepend_invert,gensym("invert"), A_FLOAT, 0);
	class_addmethod(c,(t_method)codepend_pad,gensym("pad"), A_FLOAT, 0);
    codepend_class = c;
    fftease_announce(OBJECT_NAME);
}

void codepend_mute(t_codepend *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void codepend_fftinfo( t_codepend *x )
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void codepend_free(t_codepend *x)
{
	fftease_free(x->fft);
	fftease_free(x->fft2);
    free(x->fft);
    free(x->fft2);
}

void codepend_pad(t_codepend *x, t_floatarg pad)
{
	x->invert_pad = pad;
	codepend_invert(x,x->invert);//resubmit to invert
}

void codepend_invert(t_codepend *x, t_floatarg toggle)
{
	
	x->invert_nextstate = (short)toggle;
	x->invert_countdown = x->fft->overlap; // delay effect for "overlap" vectors
	
	if(x->invert_nextstate){ // lower gain immediately; delay going to invert
		x->fft->mult = (1. / (float) x->fft->N) * x->invert_pad;
	} else {
		x->invert = 0; //immediately turn off invert; delay raising gain
	}
	
}

void *codepend_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;
	t_codepend *x = (t_codepend *)pd_new(codepend_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	x->fft2 = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
	fft->initialized = 0;
	fft2->initialized = 0;
	
	/* optional arguments: scaling exponent, threshold (now linear), overlap, winfac */
	x->exponent = 0.25;
	x->threshold = 0.01;

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;

	fft2->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = FFTEASE_DEFAULT_WINFAC;
		
	fft2->MSPVectorSize = fft->MSPVectorSize = sys_getblksize();
    if(argc > 0){ fft->N = fft2->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = fft2->overlap = (int) atom_getfloatarg(1, argc, argv); }
	
	return x;
}

void codepend_init(t_codepend *x )
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized = fft->initialized;
	
	fftease_init(fft);
	fftease_init(fft2);
	if(!initialized){
		x->invert_pad = 0.025; // -32 dB
		x->invert_countdown = 0;
		x->mute = 0;
		x->invert = 0;
	}
    if(x->invert){
		x->fft->mult *= x->invert_pad;
    }
}

void do_codepend(t_codepend *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int i;
	int N2 = fft->N2;
	float a1, b1, a2, b2, threshold = 0.1;
	int even, odd;
	int invert = x->invert;
	t_float exponent = x->exponent;
	t_float *bufferOne = fft->buffer;
	t_float *bufferTwo = fft2->buffer;
	t_float *channelOne = fft->channel;
	//	float *channelTwo = fft2->channel;
	
	if(x->invert_countdown > 0){
		
		if(x->invert) { // we 
		} else {
		}  
		--(x->invert_countdown);
		if(! x->invert_countdown){ // countdown just ended
			if(x->invert_nextstate){ // moving to invert (gain is already down)
				x->invert = x->invert_nextstate;
			} else { // invert is already off - now reset gain
				x->fft->mult = 1. / (float) x->fft->N;
			}
		}
	}
	if ( x->threshold != 0. )
		threshold = x->threshold;		
	
	fftease_fold(fft);
	fftease_fold(fft2);
	
	fftease_rdft(fft, 1);
	fftease_rdft(fft2, 1);
	
	if (invert) {
		
		for ( i = 0; i <= N2; i++ ) {
			
			float mag_1, mag_2;
			
			odd = ( even = i<<1 ) + 1;
			
			a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
			
			a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
			
			/* complex division */	
			
			mag_1 = hypot( a1, b1 );
			mag_2 = hypot( a2, b2 );
			
			if ( mag_2 > threshold )
				*(channelOne+even) =  mag_1 / mag_2;
			
			else
				*(channelOne+even) =  mag_1 / threshold;
			
			if ( mag_1 != 0. && mag_2 != 0. )
				*(channelOne+odd) = atan2( b2, a2 ) - atan2( b1, a1 );
			
			else 
				*(channelOne+odd) = 0.;
			
			/* raise resulting magnitude to a desired power */
			
			*(channelOne+even) = pow( *(channelOne+even), exponent );
		}  
	}
	
	
	else {
		
		for ( i = 0; i <= N2; i++ ) {
			
			float f_real, f_imag;
			
			odd = ( even = i<<1 ) + 1;
			
			a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
			
			a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
			
			/* complex multiply */
			
			f_real = (a1 * a2) - (b1 * b2);
			f_imag = (a1 * b2) + (b1 * a2);	
			
			*(channelOne+even) = hypot( f_real, f_imag );
			*(channelOne+odd) = -atan2( f_imag, f_real );
			
			/* raise resulting magnitude to a desired power */
			
			*(channelOne+even) = pow( *(channelOne+even), exponent );
		}
	}
	
	/* convert back to complex form, read for the inverse fft */
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	
	fftease_rdft(fft, -1);
	fftease_overlapadd(fft);
}

t_int *codepend_perform(t_int *w)
{
	int i, j;
	/* get our inlets and outlets */
	t_codepend *x = (t_codepend *) (w[1]);
	t_float *MSPInputVector1 = (t_float *)(w[2]);
	t_float *MSPInputVector2 = (t_float *)(w[3]);
	t_float *vec_exponent = (t_float *)(w[4]);
	t_float *vec_threshold = (t_float *)(w[5]);
	t_float *MSPOutputVector = (t_float *)(w[6]);
	
    x->exponent = *vec_exponent;
    x->threshold = *vec_threshold;
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
	float mult = fft->mult;	
	
	if(x->mute){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
        return w+7;
	}

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){	
        memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
        memcpy(inputOne + (Nw - D), MSPInputVector1, D * sizeof(t_float));
        memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
        memcpy(inputTwo + (Nw - D), MSPInputVector2, D * sizeof(t_float));
        
		do_codepend(x);
        
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
            
			do_codepend(x);
            
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
            
            do_codepend(x);
            
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+7;
}		

void codepend_dsp(t_codepend *x, t_signal **sp)
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
        codepend_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(codepend_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
    }
}
