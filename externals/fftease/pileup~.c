/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *pileup_class;

#define OBJECT_NAME "pileup~"


typedef struct _pileup
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	/* pileup vars */
	t_float move_threshold;
	t_float *last_frame ;
	int *frames_left;
	t_float inverse_compensation_gain; // gain up inverse
	t_float persistence; // decay factor
	int mode;
	t_float tadv;
	short mute;
	t_float hi_freq;
	t_float lo_freq;
} t_pileup;

void pileup_dsp(t_pileup *x, t_signal **sp);
t_int *pileup_perform(t_int *w);
void *pileup_new(t_symbol *s, int argc, t_atom *argv);
void pileup_mute(t_pileup *x, t_floatarg f);
void pileup_free( t_pileup *x );
void pileup_clear( t_pileup *x );
void pileup_init(t_pileup *x);
void pileup_mode(t_pileup *x, t_floatarg mode);
void pileup_inverse_gain(t_pileup *x, t_floatarg gain);
void pileup_persistence(t_pileup *x, t_floatarg persistence);
void pileup_transpose(t_pileup *x, t_floatarg tf);
void pileup_synthresh(t_pileup *x, t_floatarg thresh);
void pileup_oscbank(t_pileup *x, t_floatarg flag);
void pileup_highfreq(t_pileup *x, t_floatarg f);
void pileup_lowfreq(t_pileup *x, t_floatarg f);

void pileup_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("pileup~"), (t_newmethod)pileup_new,
                  (t_method)pileup_free,sizeof(t_pileup), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_pileup, x_f);
	class_addmethod(c,(t_method)pileup_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)pileup_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)pileup_oscbank,gensym("oscbank"),A_FLOAT,0);
	class_addmethod(c,(t_method)pileup_transpose,gensym("transpose"),A_FLOAT,0);
	class_addmethod(c,(t_method)pileup_synthresh,gensym("synthresh"),A_FLOAT,0);
	class_addmethod(c,(t_method)pileup_clear,gensym("clear"), 0);
	class_addmethod(c,(t_method)pileup_mode,gensym("mode"), A_FLOAT, 0);
	class_addmethod(c,(t_method)pileup_inverse_gain,gensym("inverse_gain"), A_FLOAT, 0);
	class_addmethod(c,(t_method)pileup_persistence,gensym("persistence"), A_FLOAT, 0);
	class_addmethod(c,(t_method)pileup_lowfreq,gensym("lowfreq"),A_FLOAT,0);
	class_addmethod(c,(t_method)pileup_highfreq,gensym("highfreq"),A_FLOAT,0);
    
    pileup_class = c;
    fftease_announce(OBJECT_NAME);
}

void pileup_highfreq(t_pileup *x, t_floatarg f)
{
	t_float curfreq;
	t_fftease *fft = x->fft;
	
	if(f < x->lo_freq){
		error("current minimum is %f",x->lo_freq);
		return;
	}
	if(f > fft->R/2 ){
		f = fft->R/2;
	}	
	x->hi_freq = f;
	fft->hi_bin = 1;  
	curfreq = 0;
	while(curfreq < x->hi_freq) {
		++(fft->hi_bin);
		curfreq += fft->c_fundamental;
	}
}

void pileup_lowfreq(t_pileup *x, t_floatarg f)
{
	t_float curfreq;
	t_fftease *fft = x->fft;
	
	if(f > x->hi_freq){
		error("current maximum is %f",x->lo_freq);
		return;
	}
	if(f < 0 ){
		f = 0;
	}	
	x->lo_freq = f;
	fft->lo_bin = 0;  
	curfreq = 0;
	while( curfreq < x->lo_freq ) {
		++(fft->lo_bin);
		curfreq += fft->c_fundamental ;
	}
}


void pileup_transpose(t_pileup *x, t_floatarg tf)
{
	x->fft->P = tf;
}

void pileup_synthresh(t_pileup *x, t_floatarg thresh)
{
	x->fft->synt = thresh;
}

void pileup_oscbank(t_pileup *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void pileup_persistence(t_pileup *x, t_floatarg persistence)
{
	x->persistence = persistence; 
}

void pileup_clear(t_pileup *x)
{
    x->last_frame = (t_float*)realloc(x->last_frame,(x->fft->N+2) * sizeof(t_float));
}

void pileup_mode(t_pileup *x, t_floatarg mode)
{
	if( mode >= 0 && mode <= 3)
		x->mode = (int) mode;
}

void pileup_inverse_gain(t_pileup *x, t_floatarg gain)
{
	x->inverse_compensation_gain = gain;
}


void pileup_free(t_pileup *x){
    if(x->fft->initialized){
        free(x->last_frame);
        free(x->frames_left);
    }
	fftease_free(x->fft);
    free(x->fft);
}

void pileup_mute(t_pileup *x, t_floatarg f){
	x->mute = (short)f;
}

void *pileup_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_pileup *x = (t_pileup *)pd_new(pileup_class);
	outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
		
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void pileup_init(t_pileup *x)
{
	t_fftease  *fft = x->fft;
	short initialized = fft->initialized;

	fftease_init(fft);
	
	if(!initialized){
		x->mode = 0;
		x->inverse_compensation_gain = 4.0;
		x->mute = 0;
		x->move_threshold = .00001 ;

		x->last_frame = (t_float *) calloc((fft->N+2), sizeof(t_float));
		x->frames_left = (int *) calloc((fft->N+2), sizeof(int));
	} else {
		x->last_frame = (t_float *) realloc(x->last_frame,(fft->N+2)*sizeof(t_float));
		x->frames_left = (int *) realloc(x->frames_left, (fft->N+2) * sizeof(int));
        x->fft->input = (t_float*) realloc(fft->input, fft->Nw * sizeof(t_float));
        x->fft->output = (t_float*) realloc(fft->output, fft->Nw * sizeof(t_float));
        x->fft->c_lastphase_in = (t_float*)realloc(fft->c_lastphase_in, (fft->N2+1) * sizeof(t_float));
        x->fft->c_lastphase_out = (t_float*)realloc(fft->c_lastphase_out, (fft->N2+1) * sizeof(t_float));
	}
	x->tadv = (t_float) fft->D / (t_float)fft->R ;
}

void do_pileup(t_pileup *x)
{
	int i;
	t_fftease *fft = x->fft;
	t_float *last_frame = x->last_frame;
	t_float persistence = x->persistence; // decay factor
	int N = fft->N;
	t_float *channel = fft->channel;

	fftease_fold(fft);
	fftease_rdft(fft,FFT_FORWARD);
	fftease_convert(fft);
	
	if( x->mode == 0 ){
		for( i = 0; i < N; i += 2 ){
			if( fabs( channel[i] ) < last_frame[i]  ){ // fabs?
				channel[i] = last_frame[i];
				channel[i + 1] = last_frame[i + 1];
			} else {
				last_frame[i] = fabs( channel[i] );
				last_frame[i + 1] = channel[i + 1];
			}
		}
	}
	else if( x->mode == 1) {
		for( i = 0; i < N; i += 2 ){
			if( fabs( channel[i] ) < last_frame[i]  ){ // fabs?
				channel[i] = last_frame[i];
			} else {
				last_frame[i] = fabs( channel[i] );
			}
		}
	}
	else if( x->mode == 2 ){
		for( i = 0; i < N; i += 2 ){
			if( fabs( channel[i] ) > last_frame[i]  ){ // fabs?
				channel[i] = last_frame[i] * x->inverse_compensation_gain;
				channel[i + 1] = last_frame[i + 1];
			} else {
				last_frame[i] = fabs( channel[i] );
				last_frame[i + 1] = channel[i + 1];
			}
		}
	}	
	if( persistence < 1.0){
		for( i = 0; i < N; i += 2 ){
			last_frame[i] *= persistence;
		}
	}

	if(fft->obank_flag){
		fftease_oscbank(fft);
	} else {
	    fftease_unconvert(fft);
		fftease_rdft(fft,FFT_INVERSE);
		fftease_overlapadd(fft);
	}
}
t_int *pileup_perform(t_int *w)

{
	int		    i,j;
    t_pileup *x = (t_pileup *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *MSPOutputVector = (t_float *)(w[3]);
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
		return w+4;
	}	
	
	if( fft->obank_flag )
		mult *= FFTEASE_OSCBANK_SCALAR;
				
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_pileup(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_pileup(x);
			
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
            
			do_pileup(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+4;
}

void pileup_dsp(t_pileup *x, t_signal **sp)
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
        pileup_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(pileup_perform, 3, x, sp[0]->s_vec, sp[1]->s_vec);
    }
}


