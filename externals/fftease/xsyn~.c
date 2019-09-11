/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *xsyn_class;

#define OBJECT_NAME "xsyn~"

typedef struct _xsyn
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_fftease *fft2;
	short mute;
} t_xsyn;

void xsyn_dsp(t_xsyn *x, t_signal **sp);
t_int *xsyn_perform(t_int *w);
void *xsyn_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
void xsyn_free( t_xsyn *x );
void xsyn_init(t_xsyn *x);
void xsyn_mute(t_xsyn *x, t_floatarg toggle);

void xsyn_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("xsyn~"), (t_newmethod)xsyn_new,
                  (t_method)xsyn_free,sizeof(t_xsyn), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_xsyn, x_f);
	class_addmethod(c,(t_method)xsyn_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)xsyn_mute,gensym("mute"),A_FLOAT,0);
    xsyn_class = c;
    fftease_announce(OBJECT_NAME);
}

void xsyn_free( t_xsyn *x )
{
    
	fftease_free(x->fft);
    free(x->fft);
    fftease_free(x->fft2);
    free(x->fft2);
}

void xsyn_mute(t_xsyn *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void *xsyn_new(t_symbol *s, int argc, t_atom *argv)
{
	t_xsyn *x = (t_xsyn *)pd_new(xsyn_class);
    t_fftease *fft, *fft2;
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
	x->mute = 0;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void xsyn_init(t_xsyn *x)
{
	fftease_init(x->fft);
	fftease_init(x->fft2);
}

void do_xsyn(t_xsyn *x)
{
    
    int i;
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    t_float *channel1 = fft->channel;
    t_float *channel2 = fft2->channel;
    int N = fft->N;
    t_float maxamp;
    
	fftease_fold(fft);
	fftease_fold(fft2);
    
	fftease_rdft(fft,FFT_FORWARD);
	fftease_rdft(fft2,FFT_FORWARD);
    
	fftease_leanconvert(fft);
	fftease_leanconvert(fft2);
	
	maxamp = 0;
    
	for( i = 0; i < N; i+= 2 ) {
		if( channel2[i] > maxamp ) {
			maxamp = channel2[i];
		}
	}
    
	if( maxamp >  0.000001 ){
		for( i = 0; i < N; i+= 2 ) {
			channel1[i] *= (channel2[i] / maxamp );
		}
	}
    
	fftease_leanunconvert(fft);
    
	fftease_rdft(fft,FFT_INVERSE);
    
	fftease_overlapadd(fft);
}

t_int *xsyn_perform(t_int *w)
{
    int i,j;
	t_xsyn *x = (t_xsyn *) (w[1]);
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
	
	if(x->mute){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+5;
	}
    
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
        memcpy(inputOne + (Nw - D), MSPInputVector1, D * sizeof(t_float));
        memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
        memcpy(inputTwo + (Nw - D), MSPInputVector2, D * sizeof(t_float));
        
		do_xsyn(x);
        
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
            
			do_xsyn(x);
            
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
            
            do_xsyn(x);
            
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+5;
}

void xsyn_dsp(t_xsyn *x, t_signal **sp)
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
        xsyn_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(xsyn_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
    }
}
