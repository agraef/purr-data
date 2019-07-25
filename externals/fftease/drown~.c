/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *drown_class;

#define OBJECT_NAME "drown~"

typedef struct _drown
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_float drownmult;
	short mute;
	short bypass;
	t_float threshold;
	short peakflag;
} t_drown;

t_int *bthresher_perform(t_int *w);
void drown_dsp(t_drown *x, t_signal **sp);
void *drown_new(t_symbol *s, int argc, t_atom *argv);
void drown_mute(t_drown *x, t_floatarg toggle);
void drown_adaptive(t_drown *x, t_floatarg toggle);
void drown_float(t_drown *x, t_float f);
void drown_overlap(t_drown *x, t_floatarg o);
void drown_free(t_drown *x);
void drown_init(t_drown *x);
void drown_fftinfo(t_drown *x);

void drown_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("drown~"), (t_newmethod)drown_new,
                  (t_method)drown_free,sizeof(t_drown), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_drown, x_f);
	class_addmethod(c,(t_method)drown_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)drown_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)drown_fftinfo,gensym("fftinfo"),0);
    class_addmethod(c,(t_method)drown_adaptive,gensym("adaptive"),A_FLOAT,0);
    drown_class = c;
    fftease_announce(OBJECT_NAME);
}

void drown_fftsize(t_drown *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	drown_init(x);
}

void drown_overlap(t_drown *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	drown_init(x);
}

void drown_winfac(t_drown *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	drown_init(x);
}

void drown_fftinfo(t_drown *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void drown_adaptive(t_drown *x, t_floatarg toggle)
{
	x->peakflag = (short)toggle;
}

void drown_mute(t_drown *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void drown_bypass(t_drown *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void *drown_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fftease *fft;
	t_drown *x = (t_drown *)pd_new(drown_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
    fft = x->fft;
	x->fft->initialized = 0;
	x->threshold = 0.001;
	x->drownmult = 0.1;
	x->mute = 0;
	x->peakflag = 1;
	x->fft->N = FFTEASE_DEFAULT_FFTSIZE;
	x->fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	x->fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	
	return x;
}

void drown_init(t_drown *x)
{
	fftease_init(x->fft);
}

void drown_free(t_drown *x)
{
	fftease_free(x->fft);
    free(x->fft);
}

void do_drown(t_drown *x)
{
	int i;
	t_fftease *fft = x->fft;
	t_float *channel = fft->channel;
	t_float threshold = x->threshold;
	t_float drownmult = x->drownmult;
	t_float frame_peak = 0.0, local_thresh;
	int N = fft->N;
	
	fftease_fold(fft);
	fftease_rdft(fft,1);
	fftease_leanconvert(fft);
	if(x->peakflag){
		for(i = 0; i < N; i += 2){	
			if(frame_peak < channel[i])
				frame_peak = channel[i];
		}
		local_thresh = frame_peak * threshold;
	} else {
		local_thresh = threshold;
	}
	for(i = 0; i < N; i += 2){	
		if(channel[i] < local_thresh)
			channel[i]  *= drownmult;
	}  	
	fftease_leanunconvert(fft);
	fftease_rdft(fft,-1);
	fftease_overlapadd(fft);
}

t_int *drown_perform(t_int *w)
{
	int	i,j;
    t_drown *x = (t_drown *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *threshold = (t_float *)(w[3]);
	t_float *drownmult = (t_float *)(w[4]);
	t_float *MSPOutputVector = (t_float *)(w[5]);
	t_fftease *fft = x->fft;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float *input = fft->input;
	t_float *output = fft->output;
	t_float mult = fft->mult;
	int MSPVectorSize = fft->MSPVectorSize;
	t_float *internalInputVector = fft->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;		
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	
	if(x->mute){
		for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+6;
	}	

    x->threshold = *threshold;
    x->drownmult = *drownmult;

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_drown(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_drown(x);
			
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
            
			do_drown(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}

void drown_dsp(t_drown *x, t_signal **sp)
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
        drown_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(drown_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
    }
}
