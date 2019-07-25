/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *residency_class;

#define OBJECT_NAME "residency~"

typedef struct _residency
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_float **loveboat;
	t_float current_frame;
	long framecount;
    long last_framecount;
	//
	t_float frame_increment;
	t_float fpos;
	t_float last_fpos;
	t_float tadv;
	short acquire_stop; // flag to stop recording immediately
	float force_pos; // force to this position on receiving message
	int read_me;
	int frames_read;
	short mute;
	short playthrough;
	t_float duration;
	short lock;
	short verbose;
	short override;
	t_float *input_vec;
	t_float sync;
	short failed_init; // flag to check if init failed due to bad data from Max
    t_float size_attr;
    short interpolation_attr;
} t_residency;

void residency_dsp(t_residency *x, t_signal **sp);
t_int *residency_perform(t_int *w);
void *residency_new(t_symbol *s, int argc, t_atom *argv);
void residency_bangname(t_residency *x) ;
void residency_fftinfo(t_residency *x) ;
void residency_playthrough( t_residency *x, t_floatarg tog) ;
void residency_mute(t_residency *x, t_floatarg tog);
void residency_interpolation(t_residency *x, t_floatarg tog);
void residency_free(t_residency *x);
void residency_init(t_residency *x);
void residency_size(t_residency *x, t_floatarg newsize);
void residency_verbose(t_residency *x, t_floatarg t);
void residency_force_position(t_residency *x, t_floatarg position);
void residency_acquire_sample(t_residency *x);
void residency_meminfo( t_residency *x );
void residency_acquire_stop(t_residency *x);
void residency_transpose(t_residency *x, t_floatarg tf);
void residency_synthresh(t_residency *x, t_floatarg thresh);
void residency_oscbank(t_residency *x, t_floatarg flag);
void do_residency(t_residency *x);

void residency_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("residency~"), (t_newmethod)residency_new,
                  (t_method)residency_free,sizeof(t_residency), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_residency, x_f);
	class_addmethod(c,(t_method)residency_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)residency_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)residency_interpolation,gensym("interpolation"),A_FLOAT,0);
	class_addmethod(c,(t_method)residency_oscbank,gensym("oscbank"),A_FLOAT,0);
	class_addmethod(c,(t_method)residency_transpose,gensym("transpose"),A_FLOAT,0);
	class_addmethod(c,(t_method)residency_synthresh,gensym("synthresh"),A_FLOAT,0);
	class_addmethod(c,(t_method)residency_acquire_sample,gensym("acquire_sample"),  0);
	class_addmethod(c,(t_method)residency_acquire_stop,gensym("acquire_stop"),  0);
	class_addmethod(c,(t_method)residency_playthrough,gensym("playthrough"), A_DEFFLOAT, 0);
	class_addmethod(c,(t_method)residency_force_position,gensym("force_position"), A_FLOAT, 0);

    residency_class = c;
    fftease_announce(OBJECT_NAME);
}

void residency_force_position(t_residency *x, t_floatarg position)
{
	if( position >= 0.0 && position < 1.0 ){
		x->force_pos = position;
	}
}

void residency_meminfo( t_residency *x )
{
    t_fftease *fft = x->fft;
    post("%d frames in buffer", x->framecount);
    post("frame_duration: %f, actual time in buffer: %f", x->tadv, (float)(x->framecount) * x->tadv);
	post("main storage chunk: %.2f MB", (x->framecount * (fft->N + 2) * sizeof(t_float)) / 1000000.0 );
}

void residency_transpose(t_residency *x, t_floatarg tf)
{
	x->fft->P = tf;
}

void residency_synthresh(t_residency *x, t_floatarg thresh)
{
	x->fft->synt = thresh;
}

void residency_oscbank(t_residency *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void residency_verbose(t_residency *x, t_floatarg t)
{
	x->verbose = t;
}

void residency_size(t_residency *x, t_floatarg newsize)
{
	if(newsize > 0.0){//could be horrendous size, but that's the user's problem
		x->duration = newsize/1000.0;
		residency_init(x);
	}
}

void residency_playthrough (t_residency *x, t_floatarg tog)
{
	x->playthrough = tog;
}

void residency_acquire_stop(t_residency *x)
{
	x->acquire_stop = 1;
	x->read_me = 0;
}

void residency_free(t_residency *x){
	int i;
    if(x->fft->initialized){
        for(i = 0; i < x->framecount; i++){
            free(x->loveboat[i]) ;
        }
        free(x->loveboat);
    }
	fftease_free(x->fft);
    free(x->fft);
}

void *residency_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fftease *fft;
	t_residency *x = (t_residency *)pd_new(residency_class);

	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;

	x->fft->N = FFTEASE_DEFAULT_FFTSIZE;
	x->fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	x->fft->winfac = FFTEASE_DEFAULT_WINFAC;
    x->last_framecount = x->framecount = 0;
    
    if(argc > 0){ x->duration = atom_getfloatarg(0, argc, argv) / 1000.0; }
    else { post("%s: must give duration argument",OBJECT_NAME); return NULL; }
    if(argc > 1){ fft->N = (int) atom_getfloatarg(1, argc, argv); }
    if(argc > 2){ fft->overlap = (int) atom_getfloatarg(2, argc, argv); }
	return x;
}

void residency_init(t_residency *x)
{
	int i;
	t_fftease *fft = x->fft;
    if(!fft->R){
        return;
    }
	if(fft->initialized == -1){
		return;
	}
    
    fftease_init(x->fft);
	
	x->tadv = (t_float)fft->D/(t_float)fft->R;
	if( x->duration <= 0 ){
		x->duration = 1.0;
	}
    if(!x->tadv){
        return;
    }
	x->framecount =  x->duration / x->tadv;
	x->read_me = 0;
	x->acquire_stop = 0;
	if(x->verbose){
		post("%s: will allocate %d frames",OBJECT_NAME, x->framecount);
	}
	// could probably improve memory management here
    if(x->framecount <= 0){
      //  post("bad framecount:%s",x->framecount);
        return;
    }
	if(fft->initialized == 0){
        // x->virgin = 1;
        x->force_pos = -1.0;
        x->current_frame = 0;
        x->fpos = x->last_fpos = 0;
        
		x->sync = 0;
		x->mute = 0;
		x->playthrough = 0;
		x->frame_increment = 0.0; // frozen by default
		x->verbose = 0;
        x->loveboat = (t_float **) calloc(x->framecount, sizeof(t_float *));
		for(i=0;i < x->framecount; i++){
			x->loveboat[i] = (t_float *) calloc((fft->N + 2), sizeof(t_float));
			if(x->loveboat[i] == NULL){
				error("%s: memory error",OBJECT_NAME);
				return;
			}
		}
	}
    else if((x->framecount == x->last_framecount) && (fft->initialized != 0)){
        return;
    }
    else if(fft->initialized == 1) {
        if(x->framecount != x->last_framecount) {
            // free individual oldies
            for(i = 0; i < x->last_framecount; i++){
                free(x->loveboat[i]);
            }
            x->loveboat = (t_float**)realloc(x->loveboat, x->framecount * sizeof(t_float *));
            
            for(i=0;i < x->framecount; i++){
                x->loveboat[i] = (t_float *) calloc((fft->N + 2), sizeof(t_float));
                if(x->loveboat[i] == NULL){
                    error("%s: memory error",OBJECT_NAME);
                    return;
                }
            }
        }
	}
	
	if(! fftease_msp_sanity_check(fft, OBJECT_NAME)){
		// return 0;
		post("residency~ failed sanity test in Init");
		x->failed_init = 1;
	} else {
		x->failed_init = 0;
	}
	if (fft->D <= 0.0 || fft->R <= 0.0){
		error("%s: bad decimation size or bad sampling rate - cannot proceed",OBJECT_NAME);
		post("D: %d R: %d",fft->D, fft->R);
		return;
	}
    x->last_framecount = x->framecount;
}

void do_residency(t_residency *x)
{
	int i;
	t_float fframe = x->current_frame ;
	t_float last_fpos = x->last_fpos ;
	int framecount = x->framecount;
	t_float fincr = x->frame_increment;
	t_float fpos = x->fpos;
	t_float force_pos = x->force_pos;
    t_float frak;
    long index1, index2;
    
	t_fftease *fft = x->fft;
    
	if(x->acquire_stop){
		x->acquire_stop = 0;
		fpos = (t_float) x->frames_read / (t_float) framecount;
		last_fpos = fpos;
		fframe = x->frames_read;
		if(x->verbose){
			post("residency: data acquisition stopped");
		}
	}
	else if(x->read_me) { // state for sampling to buffer
		if(x->frames_read >= framecount){ // termination condition
			x->read_me = 0;
			if(x->verbose){
				post("residency: data acquisition completed");
			}
		}
		else { // convert and store in one frame
			fftease_fold(fft);
			fftease_rdft(fft,1);
			fftease_convert(fft);
			for(i= 0; i < fft->N + 2; i++){
				x->loveboat[x->frames_read][i] = fft->channel[i];
			}
			++(x->frames_read);
            
		}
	}
	else { // a sample is now in the buffer
		if(fpos < 0)
			fpos = 0;
		if(fpos > 1)
			fpos = 1;
		
		if(force_pos >= 0.0 && force_pos < 1.0){
			//	post("forcing frame to %f", force_pos);
			fframe =  force_pos * (float) framecount;
			last_fpos = fpos = force_pos;
			x->force_pos = -1.0;
		}
		else if(fpos != last_fpos){
			fframe =  fpos * (float) framecount;
			last_fpos = fpos;
		}
		fframe += fincr;
        
		while(fframe >= framecount) {
			fframe -= framecount;
		}
		while( fframe < 0. ) {
			fframe += framecount;
		}
		if(x->framecount > 0) {
			x->sync = fframe/(float)x->framecount;
		}
        if(x->interpolation_attr == 1){
            index1 = floor(fframe);
            index2 = (index1+1) % x->framecount;
            frak = fframe - (t_float)index1;
            for(i= 0; i < fft->N + 2; i++){
                fft->channel[i] =
                x->loveboat[index1][i] + frak * (x->loveboat[index2][i] - x->loveboat[index1][i]);
            }
        }
        else {
            for(i= 0; i < fft->N + 2; i++){
                fft->channel[i]= x->loveboat[(int)fframe][i];
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
	/* restore state variables */
	
	x->current_frame = fframe;
	x->frame_increment = fincr;
	x->fpos = fpos;
	x->last_fpos = last_fpos;
}

t_int *residency_perform(t_int *w)
{
	int i, j;
	
	//////////////////////////////////////////////
    t_residency *x = (t_residency *) (w[1]);
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
	t_float mult = fft->mult;
	
    x->frame_increment = *increment;
    x->fpos = *position;
	if( fft->obank_flag )
		mult *= FFTEASE_OSCBANK_SCALAR;
	
	if(x->mute){
		for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
        for(i=0; i < MSPVectorSize; i++){ vec_sync[i] = 0.0; }
		return w+7;
	}
	
	if(x->acquire_stop){
		// will reset flag inside do_residency
        for(i=0; i < MSPVectorSize; i++){ output[i] = 0.0; }
		x->read_me = 0;
	}
	for ( i = 0; i < MSPVectorSize; i++ ){
		vec_sync[i] = x->sync;
	}
	if(x->framecount > 0 && x->read_me )
		x->sync = (t_float)x->frames_read/(t_float)x->framecount;
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_residency(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_residency(x);
			
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
            
			do_residency(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
return w+7;
}

void residency_acquire_sample(t_residency *x)
{
	x->read_me = 1;
	x->frames_read = 0;
	if(x->verbose)
		post("beginning spectral data acquisition");
	return;
}

void residency_mute(t_residency *x, t_floatarg tog)
{
	x->mute = (short) tog;
}

void residency_interpolation(t_residency *x, t_floatarg tog)
{
	x->interpolation_attr = (short) tog;
}

void residency_dsp(t_residency *x, t_signal **sp)
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
        residency_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(residency_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
                sp[4]->s_vec);
    }
}
