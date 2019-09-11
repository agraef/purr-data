/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *residency_buffer_class;

#define OBJECT_NAME "residency_buffer~"

typedef struct _residency_buffer
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
    long b_frames;
    long b_valid;
    t_float *b_samples;
	t_float current_frame;
	int framecount;
	//
	t_float frame_increment ;
	t_float fpos;
	t_float last_fpos;
	t_float tadv;
	long read_me;
	long frames_read;
	long MAXFRAMES;
	short mute;
	long buffer_frame_count;
	short initialized;
	short playthrough;
	t_float sync;
	short buffer_is_hosed;
    long interpolation_attr;
    t_symbol *buffername;
	void *size_outlet; // will send desired size in samples
    
} t_residency_buffer;

void residency_buffer_dsp(t_residency_buffer *x, t_signal **sp);
t_int *residency_buffer_perform(t_int *w);
void *residency_buffer_new(t_symbol *msg, short argc, t_atom *argv);
void residency_buffer_acquire_sample ( t_residency_buffer *x ) ;
void residency_buffer_meminfo( t_residency_buffer *x ) ;
void residency_buffer_mute(t_residency_buffer *x, t_floatarg toggle);
void residency_buffer_interpolation(t_residency_buffer *x, t_floatarg toggle);
void residency_buffer_calcbuf(t_residency_buffer *x, t_floatarg desired_duration);
void residency_buffer_free( t_residency_buffer *x );
void residency_buffer_playthrough(t_residency_buffer *x, t_floatarg f);
void residency_buffer_init(t_residency_buffer *x);
void residency_buffer_transpose(t_residency_buffer *x, t_floatarg tf);
void residency_buffer_synthresh(t_residency_buffer *x, t_floatarg thresh);
void residency_buffer_oscbank(t_residency_buffer *x, t_floatarg flag);
void residency_buffer_attachbuf(t_residency_buffer *x);
void residency_buffer_redraw(t_residency_buffer *x);

void residency_buffer_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("residency_buffer~"), (t_newmethod)residency_buffer_new,
                  (t_method)residency_buffer_free,sizeof(t_residency_buffer), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_residency_buffer, x_f);
	class_addmethod(c,(t_method)residency_buffer_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)residency_buffer_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)residency_buffer_interpolation,gensym("interpolation"),A_FLOAT,0);
	class_addmethod(c,(t_method)residency_buffer_oscbank,gensym("oscbank"),A_FLOAT,0);
	class_addmethod(c,(t_method)residency_buffer_transpose,gensym("transpose"),A_FLOAT,0);
	class_addmethod(c,(t_method)residency_buffer_synthresh,gensym("synthresh"),A_FLOAT,0);
	class_addmethod(c,(t_method)residency_buffer_calcbuf,gensym("calcbuf"), A_FLOAT, 0);
	class_addmethod(c,(t_method)residency_buffer_playthrough,gensym("playthrough"), A_FLOAT, 0);
	class_addmethod(c,(t_method)residency_buffer_acquire_sample,gensym("acquire_sample"), 0);
    residency_buffer_class = c;
    fftease_announce(OBJECT_NAME);
}

void residency_buffer_free( t_residency_buffer *x )
{
	fftease_free(x->fft);
    free(x->fft);
}

void residency_buffer_calcbuf(t_residency_buffer *x, t_floatarg desired_duration)
{
	t_float ms_calc;
	t_float seconds;
	t_float frames;
	t_float samples;
	t_float tadv = x->tadv;
	t_fftease *fft = x->fft;
	
	if(tadv == 0){
		post("zero tadv!");
		return;
	}
	if(fft->R == 0){
		post("zero sampling rate!");
		return;
	}
	seconds = desired_duration / 1000.0;
	frames = seconds / tadv;
	samples = frames * (t_float) (fft->N + 2);
	ms_calc = (samples / fft->R) * 1000.0;
	post("desired duration in ms: %f",desired_duration);
	post("you need %.0f samples in buffer to get %.0f frames or %f secs", 
		 samples, frames, seconds);
	outlet_float(x->size_outlet, samples);
	
}

void *residency_buffer_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_residency_buffer *x = (t_residency_buffer *)pd_new(residency_buffer_class);
	t_fftease *fft;

	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->size_outlet = outlet_new(&x->x_obj, gensym("float"));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ x->buffername = atom_getsymbolarg(0, argc, argv); }
    else { post("%s: Must specify array name", OBJECT_NAME); return NULL; }
    if(argc > 1){ fft->N = (int) atom_getfloatarg(1, argc, argv); }
    if(argc > 2){ fft->overlap = (int) atom_getfloatarg(2, argc, argv); }
	return x;
}

void residency_buffer_init(t_residency_buffer *x)
{

	t_fftease *fft = x->fft;
	short initialized = x->fft->initialized;
	if( fft->R <= 0 ){
		post("bad SR");
		return;
	}
	if( fft->MSPVectorSize <= 0 ){
		post("bad vectorsize");
		return;
	}
	fftease_init(fft);	
	x->tadv = (t_float)fft->D / (t_float)fft->R;
	if(!initialized){
		x->mute = 0;
		x->sync = 0;
		x->initialized = 1;
		x->current_frame = x->framecount = 0;
		x->frame_increment = 1.0 ;
		x->fpos = x->last_fpos = 0;
	}
}

void do_residency_buffer(t_residency_buffer *x)
{
	t_fftease *fft = x->fft;
	int N = x->fft->N;
	int i,j,k;
	t_float fframe = x->current_frame ;
	t_float fincr = x->frame_increment;
	t_float fpos = x->fpos;
	t_float last_fpos = x->last_fpos ;
	t_float *channel = fft->channel;
	float *b_samples;
	long b_frames = x->b_frames;
    long b_valid = x->b_valid;
	int frames_read = x->frames_read;
	long index_offset;
	long buffer_frame_count = x->buffer_frame_count;
    long index1, index2;
    t_float frak;
    
    residency_buffer_attachbuf(x);
    b_samples = x->b_samples;
    b_frames = x->b_frames;

    buffer_frame_count = (int)((t_float) b_frames / (t_float)(x->fft->N + 2));
    if(b_frames < 1 || ! b_valid){
        post("%s: table too small or not valid",OBJECT_NAME);
        return;
    }
	if( x->read_me ) {
		fftease_fold(fft);
		fftease_rdft(fft, FFT_FORWARD);
		fftease_convert(fft);
		
		index_offset = (N+2) * frames_read;
		
		for(i = index_offset, j = 0; i < index_offset + N + 2; i++, j++){
            if(i >= b_frames){
                
                post("hit end of buffer on frame %d", frames_read);
                goto escape;
            }
			b_samples[i] = channel[j];
		}
		
		++frames_read;
		x->sync = (t_float)frames_read/(t_float)(buffer_frame_count);
		
		if( frames_read >= buffer_frame_count){
			x->read_me = 0;
            fpos = 0.0;
            residency_buffer_redraw(x);
		}
        
	} 
	else {
		if( fpos < 0 )
			fpos = 0;
		if( fpos > 1 )
			fpos = 1;
		if( fpos != last_fpos ){
			fframe =  fpos * (t_float) buffer_frame_count;
			last_fpos = fpos;
		}
		
		fframe += fincr;
        // post("fframe %f framecount %d", fframe, buffer_frame_count);
        
		while(fframe >= buffer_frame_count) {
			fframe -= buffer_frame_count;
		} 
		while( fframe < 0. ) {
			fframe += buffer_frame_count;
		}
		// goto escape;
		
		if(x->interpolation_attr == 1){
            long iframe = floor(fframe);
            index1 = (N+2) * iframe;
            index2 = (N+2) * ((iframe + 1) % buffer_frame_count);
            frak = fframe - iframe;
            for( i = index1, j = index2, k = 0; i < index1 + N + 2; i++, j++, k++){
                if(i >= b_frames || j >= b_frames){
                    post("hit end of buffer on frame %d, index %d %d", index1,i,j);
                    goto escape;
                }
                channel[k] = b_samples[i] + frak * (b_samples[j] - b_samples[i]);
            }
        }
        else {
            index_offset = (N+2) * (long) fframe;
            for( i = index_offset, j = 0; i < index_offset + N + 2; i++, j++ ){
                if(i >= b_frames){
                    post("hit end of buffer on frame %d, index %d", index_offset,i);
                    goto escape;
                }
                channel[j] = b_samples[i];
            }
        }
		x->sync = fframe / (t_float) buffer_frame_count;
		// REPLACE loveboat with buffer
		if(fft->obank_flag){
			fftease_oscbank(fft);
		}
        else {
			fftease_unconvert(fft);
			fftease_rdft(fft, FFT_INVERSE);
			fftease_overlapadd(fft);
		}
        
	}
escape:
    ;
	/* restore state variables */
	x->current_frame = fframe;
	x->frame_increment = fincr;
	x->fpos = fpos;
	x->last_fpos = last_fpos;
	x->frames_read = frames_read;
}


void residency_buffer_redraw(t_residency_buffer *x)
{
    t_garray *a;
    if (!(a = (t_garray *)pd_findbyclass(x->buffername, garray_class))) {
        if (*x->buffername->s_name) pd_error(x, "function~: %s: no such array", x->buffername->s_name);
    }
    else  {
        garray_redraw(a);
    }
}

void residency_buffer_attachbuf(t_residency_buffer *x)
{
  	int frames;
    t_symbol *buffername = x->buffername;
	t_garray *a;
    
	x->b_frames = 0;
	x->b_valid = 0;
	if (!(a = (t_garray *)pd_findbyclass(buffername, garray_class)))
    {
		if (*buffername->s_name) pd_error(x, "player~: %s: no such array",
                                          buffername->s_name);
    }
	else if (!garray_getfloatarray(a, &frames, &x->b_samples))
    {
		pd_error(x, "%s: bad template for player~", buffername->s_name);
    }
	else  {
		x->b_frames = frames;
		x->b_valid = 1;
		garray_usedindsp(a);
	}
}

t_int *residency_buffer_perform(t_int *w)
{
	int i, j;
	
	//////////////////////////////////////////////
    t_residency_buffer *x = (t_residency_buffer *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *increment = (t_float *)(w[3]);
	t_float *position = (t_float *)(w[4]);
	t_float *MSPOutputVector = (t_float *)(w[5]);
	t_float *vec_sync = (t_float *)(w[6]);
	t_fftease *fft = x->fft;
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	t_float *internalInputVector = fft->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;
	
	int D = fft->D;
	int Nw = fft->Nw;
	t_float *input = fft->input;
	t_float *output = fft->output;
	float mult = fft->mult;

	
	if( fft->obank_flag )
		mult *= FFTEASE_OSCBANK_SCALAR;

	residency_buffer_attachbuf(x);
	/* quit before doing anything unless we're good to go */
    
	if( x->mute || ! x->b_valid) {
		for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
        for(i=0; i < MSPVectorSize; i++){ vec_sync[i] = 0.0; }
		return w+7;
	}

    x->frame_increment = *increment;
    x->fpos = *position;

	for ( i = 0; i < MSPVectorSize; i++ ){
		vec_sync[i] = x->sync;
	}
	if(x->framecount > 0 && x->read_me ){
		x->sync = (t_float)x->frames_read/(t_float)x->framecount;
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_residency_buffer(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_residency_buffer(x);
			
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
            
			do_residency_buffer(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+7;
}	

void residency_buffer_acquire_sample ( t_residency_buffer *x )
{
    residency_buffer_attachbuf(x);
	x->read_me = 1;
	x->frames_read = 0;
    x->buffer_frame_count = (int)((t_float) (x->b_frames) / (t_float)(x->fft->N + 2));
	post("storing %d FFT frames", x->buffer_frame_count);
	post("%s: beginning spectral data acquisition",OBJECT_NAME);
	return;
	
}

void residency_buffer_mute(t_residency_buffer *x, t_floatarg toggle)
{
	x->mute = (short)toggle;	
}

void residency_buffer_interpolation(t_residency_buffer *x, t_floatarg toggle)
{
	x->interpolation_attr = (short)toggle;
}

void residency_buffer_playthrough(t_residency_buffer *x, t_floatarg toggle)
{
	x->playthrough = (short)toggle;	
}


void residency_buffer_transpose(t_residency_buffer *x, t_floatarg tf)
{
	x->fft->P = (float) tf;
}

void residency_buffer_synthresh(t_residency_buffer *x, t_floatarg thresh)
{
	x->fft->synt = (float) thresh;
}

void residency_buffer_oscbank(t_residency_buffer *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void residency_buffer_dsp(t_residency_buffer *x, t_signal **sp)
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
        residency_buffer_init(x);
    }

    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(residency_buffer_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
                sp[4]->s_vec);
    }
}
