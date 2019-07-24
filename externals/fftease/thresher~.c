/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *thresher_class;

#define OBJECT_NAME "thresher~"
#define DEFAULT_HOLD (40.0)

typedef struct _thresher
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_float move_threshold;
	t_float *composite_frame;
	int *frames_left;
	int max_hold_frames;
	t_float max_hold_time;
	int first_frame;
	t_float damping_factor;
	short thresh_connected;
	short damping_connected;
	short mute;
	t_float tadv;
} t_thresher;

void thresher_dsp(t_thresher *x, t_signal **sp);
t_int *thresher_perform(t_int *w);
void *thresher_new(t_symbol *s, int argc, t_atom *argv);
void thresher_mute(t_thresher *x, t_floatarg f);
void thresher_free( t_thresher *x );
void thresher_init(t_thresher *x);
void thresher_transpose(t_thresher *x, t_floatarg tf);
void thresher_synthresh(t_thresher *x, t_floatarg thresh);
void thresher_oscbank(t_thresher *x, t_floatarg flag);

void thresher_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("thresher~"), (t_newmethod)thresher_new,
                  (t_method)thresher_free,sizeof(t_thresher), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_thresher, x_f);
	class_addmethod(c,(t_method)thresher_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)thresher_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)thresher_oscbank,gensym("oscbank"),A_FLOAT,0);
	class_addmethod(c,(t_method)thresher_transpose,gensym("transpose"),A_FLOAT,0);
	class_addmethod(c,(t_method)thresher_synthresh,gensym("synthresh"),A_FLOAT,0);
    thresher_class = c;
    fftease_announce(OBJECT_NAME);
}

void thresher_transpose(t_thresher *x, t_floatarg tf)
{
	x->fft->P = tf;
}

void thresher_synthresh(t_thresher *x, t_floatarg thresh)
{
	x->fft->synt = thresh;
}

void thresher_oscbank(t_thresher *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void thresher_free(t_thresher *x){
    if(x->fft->initialized){
        free(x->composite_frame);
    }
	fftease_free(x->fft);
    free(x->fft);
}

void thresher_mute(t_thresher *x, t_floatarg f){
	x->mute = (short)f;
}

void *thresher_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_thresher *x = (t_thresher *)pd_new(thresher_class);
    
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease) );
	fft = x->fft;
	fft->initialized = 0;
	x->move_threshold = 0.001;
	x->damping_factor = 0.99;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;	
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void thresher_init(t_thresher *x)
{
	t_fftease  *fft = x->fft;
	short initialized = fft->initialized;

	fftease_init(fft);
	x->tadv = (t_float) fft->D / (t_float) fft->R ;

	if(!initialized){
		x->mute = 0;
		if(!x->damping_factor){
			x->damping_factor = .95;
		}
		x->first_frame = 1;
		x->move_threshold = .00001 ;
		x->max_hold_time = DEFAULT_HOLD ;
		x->max_hold_frames = x->max_hold_time / x->tadv;
		x->composite_frame = (t_float *) calloc( (fft->N+2), sizeof(t_float));
		x->frames_left = (int *) calloc( (fft->N+2), sizeof(int) );
		
	} else if(initialized == 1){
		x->composite_frame = (t_float *) realloc(x->composite_frame, (fft->N+2) * sizeof(t_float) );
		x->frames_left = (int *) realloc(x->frames_left, (fft->N+2) * sizeof(int) );
	}
}

void do_thresher(t_thresher *x)
{
	int i;
	
	t_fftease *fft = x->fft;
	t_float *channel = fft->channel;
	t_float damping_factor = x->damping_factor;
	int max_hold_frames = x->max_hold_frames;
	int *frames_left = x->frames_left;
	t_float *composite_frame = x->composite_frame;
	int N = fft->N;
	t_float move_threshold = x->move_threshold;

	fftease_fold(fft);
	fftease_rdft(fft,FFT_FORWARD);
	fftease_convert(fft);
	
	if( x->first_frame ){
		for ( i = 0; i < N+2; i++ ){
			composite_frame[i] = channel[i];
			frames_left[i] = max_hold_frames;
		}
		x->first_frame = 0;
	} else {
		for( i = 0; i < N+2; i += 2 ){
			if(fabs( composite_frame[i] - channel[i] ) > move_threshold || frames_left[i] <= 0 ){
				composite_frame[i] = channel[i];
				composite_frame[i+1] = channel[i+1];
				frames_left[i] = max_hold_frames;
			} else {
				--(frames_left[i]);
				composite_frame[i] *= damping_factor;
			}
		}
	}
	// try memcpy here
	for ( i = 0; i < N+2; i++ ){
		channel[i] = composite_frame[i];
	}
	if(fft->obank_flag){
		fftease_oscbank(fft);
	} else {
		fftease_unconvert(fft);
		fftease_rdft(fft,FFT_INVERSE);
		fftease_overlapadd(fft);
	}	
}

t_int *thresher_perform(t_int *w)
{
	int		    i,j;
    t_thresher *x = (t_thresher *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *inthresh = (t_float *)(w[3]);
	t_float *damping = (t_float *)(w[4]);
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


    x->move_threshold = *inthresh;
    x->damping_factor = *damping;

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_thresher(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_thresher(x);
			
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
            
			do_thresher(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}

void thresher_dsp(t_thresher *x, t_signal **sp)
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
        thresher_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(thresher_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
    }
}