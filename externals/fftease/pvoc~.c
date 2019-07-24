/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *pvoc_class;

#define OBJECT_NAME "pvoc~"

typedef struct _pvoc
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_float lofreq;
	t_float hifreq;
	t_float topfreq;
	short mute;
} t_pvoc;

void *pvoc_new(t_symbol *s, int argc, t_atom *argv);
void pvoc_free(t_pvoc *x);
void pvoc_mute(t_pvoc *x, t_floatarg tog);
void pvoc_init(t_pvoc *x);
void pvoc_fftinfo(t_pvoc *x);
void pvoc_lowfreq(t_pvoc *x, t_floatarg f);
void pvoc_highfreq(t_pvoc *x, t_floatarg f);
void do_pvoc(t_pvoc *x );
t_int *pvoc_perform(t_int *w);
void pvoc_dsp(t_pvoc *x, t_signal **sp);

void pvoc_tilde_setup(void)
{
    pvoc_class = class_new(gensym("pvoc~"), (t_newmethod)pvoc_new,
						   (t_method)pvoc_free,sizeof(t_pvoc), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(pvoc_class, t_pvoc, x_f);
	class_addmethod(pvoc_class,(t_method)pvoc_dsp,gensym("dsp"),0);
	class_addmethod(pvoc_class,(t_method)pvoc_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(pvoc_class,(t_method)pvoc_lowfreq,gensym("lowfreq"),A_FLOAT,0);
    class_addmethod(pvoc_class,(t_method)pvoc_highfreq,gensym("highfreq"),A_FLOAT,0);
    class_addmethod(pvoc_class,(t_method)pvoc_fftinfo,gensym("fftinfo"),0);
    fftease_announce(OBJECT_NAME);
}

void pvoc_lowfreq(t_pvoc *x, t_floatarg f)
{
    if(!x->fft->initialized)
        return;
    if(f < 0 ){
        f = 0;
    }
    x->lofreq = f;
    fftease_oscbank_setbins(x->fft,x->lofreq, x->hifreq);
}

void pvoc_highfreq(t_pvoc *x, t_floatarg f)
{
    if(!x->fft->initialized)
        return;
    if(f < 0 ){
        f = 0;
    }
    if(f < x->lofreq){
        error("%s: maximum cannot go below current minimum: %f",OBJECT_NAME,x->lofreq);
        return;
    }
    if(f > x->fft->R/2 ){
        f = x->fft->R/2;
    }
    x->hifreq = f;
    fftease_oscbank_setbins(x->fft,x->lofreq, x->hifreq);
}

void pvoc_mute(t_pvoc *x, t_floatarg tog)
{
	x->mute = (short)tog;
}

void pvoc_fftinfo(t_pvoc *x)
{
t_fftease *fft = x->fft;
	fftease_fftinfo( fft, OBJECT_NAME );	
}

void pvoc_free(t_pvoc *x ){
	fftease_free(x->fft);
    free(x->fft);
}

void pvoc_init(t_pvoc *x)
{
	float curfreq;
	t_fftease *fft = x->fft;
	
	if(fft->initialized == -1){
		return;
	}
    
	fftease_init(fft);

	if( x->hifreq < fft->c_fundamental ) {
        post("default hi frequency of 18000 Hz");
		x->hifreq = 18000.0 ;
	}
	x->fft->hi_bin = 1;  
	curfreq = 0;
	while( curfreq < x->hifreq ) {
		++(x->fft->hi_bin);
		curfreq += fft->c_fundamental ;
	}
	
	x->fft->lo_bin = 0;  
	curfreq = 0;
	while( curfreq < x->lofreq ) {
		++(x->fft->lo_bin);
		curfreq += fft->c_fundamental;
	}
}

void *pvoc_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fftease *fft;
	t_pvoc *x = (t_pvoc *)pd_new(pvoc_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	x->lofreq = 0;
	x->hifreq = 15000;
	x->mute = 0;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv);}
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv);}
    if(argc > 2){ x->lofreq = atom_getfloatarg(2, argc, argv);}
    if(argc > 3){ x->hifreq = atom_getfloatarg(3, argc, argv);}
	fft->initialized = 0;// prepare for init in DSP routine
	return x;
}

void do_pvoc(t_pvoc *x)
{
	t_fftease *fft = x->fft;
    fftease_fold(fft);
	fftease_rdft(fft, 1);
	fftease_convert(fft);
	fftease_oscbank(fft);
}

t_int *pvoc_perform(t_int *w)
{
    t_pvoc *x = (t_pvoc *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *transp = (t_float *)(w[3]);
	t_float *synth_thresh = (t_float *)(w[4]);
	t_float *MSPOutputVector = (t_float *)(w[5]);
    
	t_fftease *fft = x->fft;
	int i, j;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float mult = fft->mult;
	t_float *input = fft->input;
	t_float *output = fft->output;
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	t_float *internalInputVector = fft->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;
	
	if(x->mute){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+6;
	}

    fft->P  = *transp;
    fft->pitch_increment = fft->P*fft->L/fft->R;
    fft->synt = *synth_thresh;

	// HERE IS THE GOOD STUFF
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
		do_pvoc( x );
        
		for ( j = 0; j < D; j++ ){
			*MSPOutputVector++ = output[j] * mult;
        }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
			do_pvoc( x );
			
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

			do_pvoc( x );
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}

void pvoc_dsp(t_pvoc *x, t_signal **sp)
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
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        pvoc_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(pvoc_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
    }
}

