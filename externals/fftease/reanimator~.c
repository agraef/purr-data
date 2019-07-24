/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

#define THRESHOLD_MIN (.000001)

static t_class *reanimator_class;

#define OBJECT_NAME "reanimator~"

typedef struct _reanimator
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_float **framebank;
	t_float *normalized_frame;
	t_float current_frame;
	int framecount;
	t_float frame_increment ;
	t_float last_frame ;
	t_float fpos;
	t_float last_fpos;
	t_float tadv;
	int readme;
	int total_frames;
	short mute;
	short initialized;
	t_float threshold;
	short inverse;
	int top_comparator_bin;
	short reanimator_mode;
	int matchframe; // current found frame
	t_float sample_len; /*duration of texture sample */
	t_float sync;
	int megs;
} t_reanimator;

void reanimator_dsp(t_reanimator *x, t_signal **sp);
t_int *reanimator_perform(t_int *w);
void *reanimator_new(t_symbol *msg, short argc, t_atom *argv);
void reanimator_analyze (t_reanimator *x);
void reanimator_mute(t_reanimator *x, t_floatarg flag);
void reanimator_inverse(t_reanimator *x, t_floatarg toggle);
void reanimator_topbin(t_reanimator *x, t_floatarg bin);
void reanimator_startframe(t_reanimator *x, t_floatarg start);
void reanimator_endframe(t_reanimator *x, t_floatarg end);
void reanimator_framerange(t_reanimator *x, t_floatarg start, t_floatarg end);
void reanimator_size(t_reanimator *x, t_floatarg size_ms);
void reanimator_freeze_and_march(t_reanimator *x, t_floatarg f);
void reanimator_resume( t_reanimator *x );
void reanimator_threshold(t_reanimator *x, t_floatarg threshold);
void reanimator_free( t_reanimator *x );
void reanimator_framecount ( t_reanimator *x );
void reanimator_init(t_reanimator *x);
void reanimator_transpose(t_reanimator *x, t_floatarg tf);
void reanimator_synthresh(t_reanimator *x, t_floatarg thresh);
void reanimator_oscbank(t_reanimator *x, t_floatarg flag);

void reanimator_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("reanimator~"), (t_newmethod)reanimator_new,
                  (t_method)reanimator_free,sizeof(t_reanimator), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_reanimator, x_f);
	class_addmethod(c,(t_method)reanimator_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)reanimator_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)reanimator_oscbank,gensym("oscbank"),A_FLOAT,0);
	class_addmethod(c,(t_method)reanimator_transpose,gensym("transpose"),A_FLOAT,0);
	class_addmethod(c,(t_method)reanimator_synthresh,gensym("synthresh"),A_FLOAT,0);
	class_addmethod(c,(t_method)reanimator_inverse,gensym("inverse"), A_FLOAT, 0);
	class_addmethod(c,(t_method)reanimator_topbin,gensym("topbin"), A_FLOAT, 0);
	class_addmethod(c,(t_method)reanimator_threshold,gensym("threshold"), A_FLOAT, 0);
	class_addmethod(c,(t_method)reanimator_analyze,gensym("analyze"), 0);
	class_addmethod(c,(t_method)reanimator_framecount,gensym("framecount"), 0);
	class_addmethod(c,(t_method)reanimator_freeze_and_march,gensym("freeze_and_march"), A_FLOAT, 0);
	class_addmethod(c,(t_method)reanimator_resume,gensym("resume"), 0);

    reanimator_class = c;
    fftease_announce(OBJECT_NAME);
}

void reanimator_transpose(t_reanimator *x, t_floatarg tf)
{
	x->fft->P = (float) tf;
}

void reanimator_synthresh(t_reanimator *x, t_floatarg thresh)
{
	x->fft->synt = (float) thresh;
}

void reanimator_oscbank(t_reanimator *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void reanimator_framecount ( t_reanimator *x )
{	
	post("%d frames stored", x->total_frames);
}

void reanimator_freeze_and_march(t_reanimator *x, t_floatarg f)
{	
	x->frame_increment = f;
	x->reanimator_mode = 1;
}

void reanimator_resume( t_reanimator *x )
{
	x->reanimator_mode = 0;
}

void reanimator_free( t_reanimator *x ){
	int i;
    if(x->fft->initialized){
        fftease_free(x->fft);
        for(i = 0; i < x->framecount; i++){
            free(x->framebank[i]) ;
        }
        free((char**)x->framebank);
        free(x->normalized_frame);
    }
}

void *reanimator_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft;
	t_reanimator *x = (t_reanimator *)pd_new(reanimator_class);

	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
    fft->initialized = 0;
	x->sample_len = 1000.0;
    if(argc > 0){ x->sample_len = atom_getfloatarg(0, argc, argv); }
	else { post("%s: must include duration argument",OBJECT_NAME); return NULL; }
	x->sample_len *= .001; /* convert to seconds */
	
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 1){ fft->N = (int) atom_getfloatarg(1, argc, argv); }
    if(argc > 2){ fft->overlap = (int) atom_getfloatarg(2, argc, argv); }
	return x;
}

void reanimator_init(t_reanimator *x )
{
	t_fftease  *fft = x->fft;
	t_float **framebank = x->framebank;
	int framecount = x->framecount;
	short initialized = fft->initialized;
	
	fftease_init(fft);
	if(!fftease_msp_sanity_check(fft,OBJECT_NAME)){
		return;
	}
	// sanity check here	
	x->tadv = (float)fft->D/(float)fft->R;
	x->current_frame = framecount = 0;
	x->fpos = x->last_fpos = 0;
	x->total_frames =  x->sample_len / x->tadv;	

	if(!initialized){
		x->sync = 0.0;
		x->inverse = 0;
		x->initialized = 0; // for perform
		x->threshold = .0001;
		x->top_comparator_bin = 10;
		x->reanimator_mode = 0;
		x->frame_increment = 1.0;
		x->mute = 0;
		x->readme = 0;
		x->total_frames =  x->sample_len / x->tadv;
		x->framebank = (t_float **) calloc(x->total_frames, sizeof(t_float *));
		
		while(framecount < x->total_frames ){
			x->framebank[framecount] = (t_float *) calloc((fft->N+2),sizeof(t_float));
			++framecount;
		}
		
	}
    else if(initialized == 1){
	// danger: could be more frames this time!!!
		while(framecount < x->total_frames ){
			x->framebank[framecount] = (t_float *) realloc(framebank[framecount], (fft->N+2) * sizeof(t_float));
			++framecount;
		}
	}

	x->framecount = framecount;
	x->megs = sizeof(t_float) * x->framecount * (fft->N+2);
}

void do_reanimator(t_reanimator *x)
{
	t_float ampsum, new_ampsum, rescale;
	t_float min_difsum, difsum;
	int	
	i,j;
	t_fftease *fft = x->fft;
	
	int framecount = x->framecount;
	int total_frames = x->total_frames;
	
	float threshold = x->threshold;
	int top_comparator_bin = x->top_comparator_bin ;
	
	t_float **framebank = x->framebank;
	// for reanimator mode
	t_float fframe = x->current_frame ;
	t_float last_fpos = x->last_fpos ;
	t_float fincr = x->frame_increment;
	t_float fpos = x->fpos ;
	t_float sync = x->sync;
	t_float *channel = fft->channel;
	t_float *output = fft->output;
	int matchframe = x->matchframe;
	int N = fft->N;
	int D = fft->D;
	float rescale_inv;
	/***********************************/

	if(total_frames <= 0)
		return;
	/* SAMPLE MODE */
	if( x->readme ) {
			
		
		if( framecount >= total_frames ){
			sync = 1.0;
			x->readme = 0;
			post("reanimator~: data acquisition completed");
			x->initialized = 1;
			// clear input buffer
			for( i = 0; i < fft->Nw; i++ ){
				fft->input[i] = 0.0;
			}
		} else {
			fftease_fold(fft);
			fftease_rdft(fft,FFT_FORWARD);
			fftease_convert(fft);
			sync = (float) framecount / (float) total_frames;
			
			new_ampsum = ampsum = 0;
			for(i = 0; i < N; i += 2 ){
				ampsum += channel[i];
			}
			
			if( ampsum > .000001 ){
				rescale = 1.0 / ampsum ;
				
				// use more efficient memcpy
				for(i = 0; i < N; i++){
					framebank[framecount][i] = channel[i];
				}
				for( i = 0; i < N; i += 2 ){
					framebank[framecount][i] *= rescale;
				} 
				++framecount;

			} else {
				post("amplitude for frame %d is too low\n", framecount);
			}
		}		
		
	} 	/* reanimator RESYNTHESIS */
	else if(x->reanimator_mode) {   
		if( fpos < 0 )
			fpos = 0;
		if( fpos > 1 )
			fpos = 1;
		if( fpos != last_fpos ){
			fframe =  fpos * (float) framecount ;
			last_fpos = fpos;
		}
		
		
		fframe += fincr;
		while( fframe >= framecount ) {
			fframe -= framecount;
		} 
		while( fframe < 0. ) {
			fframe += framecount ;
		}
		matchframe = (int) fframe;
		
		// use memcopy
		for(i = 0; i < N; i++){
			channel[i] = framebank[matchframe][i];
		}
		if(fft->obank_flag){
			fftease_oscbank(fft);
		} else {
			fftease_unconvert(fft);
			fftease_rdft(fft,FFT_INVERSE);
			fftease_overlapadd(fft);
		}
		
		
	}
	/* REANIMATION HERE */
	else {
		fftease_fold(fft);
		fftease_rdft(fft,FFT_FORWARD);
		fftease_convert(fft);
		ampsum = 0;
		// NORMALIZE INPUT FRAME
		for( i = 0; i < N; i += 2 ){
			ampsum += channel[i];
		}
		
		if( ampsum > threshold ){
			rescale = 1.0 / ampsum;
			for( i = 0; i < N; i += 2 ){
				channel[i] *= rescale;
			}
		} 
		else {
			// AMPLITUDE OF INPUT WAS TOO LOW - OUTPUT SILENCE AND RETURN
			for (i = 0; i < D; i++ ){
				output[i] = 0.0;
			}
			matchframe = 0;		
			x->current_frame = fframe;
			x->frame_increment = fincr;
			x->fpos = fpos;
			x->sync = sync;
			x->framecount = framecount;
			x->matchframe = matchframe;
			return;
			
		}
		// NOW COMPARE TO STORED FRAMES
		if( x->inverse ){ // INVERSE CASE
			min_difsum = 0.0 ;
			
			for( j = 0; j < framecount; j++ ){
				difsum = 0;
				for( i = 0; i < top_comparator_bin * 2; i += 2 ){
					difsum += fabs( channel[i] - framebank[j][i] ); 
				}
				//      fprintf(stderr,"bin 20: in %f compare %f\n", channel[40], frames[j][40]);
				if( difsum > min_difsum ){
					matchframe = j;
					min_difsum = difsum;
				}
			}
		} else { // NORMAL CASE
			min_difsum = 1000000.0 ;
			
			for( j = 0; j < framecount; j++ ){
				difsum = 0;
				for( i = 0; i < top_comparator_bin * 2; i += 2 ){
					difsum += fabs( channel[i] - framebank[j][i] ); 
				}
				//      fprintf(stderr,"bin 20: in %f compare %f\n", channel[40], frames[j][40]);
				if( difsum < min_difsum ){
					matchframe = j;
					min_difsum = difsum;
				}
			}
		}
		// use memcopy
		for(i = 0; i < N; i++){
			channel[i] = framebank[matchframe][i];
		}		
		if(fft->obank_flag){
			fftease_oscbank(fft);
		} else {
			fftease_unconvert(fft);
			fftease_rdft(fft,FFT_INVERSE);
			fftease_overlapadd(fft);
		}
		
		// scale back to match
		rescale_inv = 1.0 / rescale;
		for (i = 0; i < D; i++){
			output[i] *= rescale_inv;
		}
	}
	
	/* restore state variables */
	x->current_frame = fframe;
	x->frame_increment = fincr;
	x->fpos = fpos;
	x->sync = sync;
	x->framecount = framecount;
	x->matchframe = matchframe;
}


t_int *reanimator_perform(t_int *w)
{
	int		i,j;
	
	//////////////////////////////////////////////
    t_reanimator *x = (t_reanimator *) (w[1]);
	t_float *driver = (t_float *)(w[2]); // was driver
	t_float *texture = (t_float *)(w[3]);
	t_float *MSPOutputVector = (t_float *)(w[4]); // was soundout
	t_float *matchout = (t_float *)(w[5]);
	t_float *sync_vec = (t_float *)(w[6]);
	
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

	/***********************************/
	if(x->mute || ! x->initialized){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+7;
	}	

	if( fft->obank_flag )
		mult *= FFTEASE_OSCBANK_SCALAR;
		
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
		if(x->readme){
            memcpy(input + (Nw - D), texture, D * sizeof(t_float));
		} else {
            memcpy(input + (Nw - D), driver, D * sizeof(t_float));
		}
		do_reanimator(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}	
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
			if(x->readme){
				memcpy(input + (Nw - D), texture + (D * i), D * sizeof(float));
			} else {
				memcpy(input + (Nw - D), driver + (D * i), D * sizeof(float));
			}
			do_reanimator(x);
			for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
            memcpy(output, output + D, (Nw-D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
		if(x->readme){
			memcpy(internalInputVector + (operationCount * MSPVectorSize), texture, MSPVectorSize * sizeof(float));

		} else {

			memcpy(internalInputVector + (operationCount * MSPVectorSize), driver, MSPVectorSize * sizeof(float));
		}
		memcpy(MSPOutputVector, internalOutputVector + (operationCount * MSPVectorSize), MSPVectorSize * sizeof(float));

		operationCount = (operationCount + 1) % operationRepeat;

		if( operationCount == 0 ) {
            memcpy(input, input + D,  (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw - D), internalInputVector,  D * sizeof(t_float));
			do_reanimator( x );

			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}	
	// now load other output buffers
	for(i = 0; i < MSPVectorSize; i++){
		matchout[i] = x->matchframe;
		sync_vec[i] = x->sync;
	}
    return w+7;
}

void reanimator_analyze ( t_reanimator *x )
{
	x->readme = 1;
	x->initialized = 1;
	x->framecount = 0;
	post("reanimator: beginning spectral data acquisition");
	return;
	
}

void reanimator_mute(t_reanimator *x, t_floatarg flag)
{
	x->mute = (short)flag;	
}
void reanimator_topbin(t_reanimator *x, t_floatarg bin)
{
	if( bin > 1 && bin < x->fft->N2 )
		x->top_comparator_bin = bin;
}


void reanimator_inverse(t_reanimator *x, t_floatarg toggle)
{
	x->inverse = (short)toggle;	
}

void reanimator_threshold(t_reanimator *x, t_floatarg threshold)
{
	if( threshold > THRESHOLD_MIN )
		x->threshold = threshold;
	else
		x->threshold = THRESHOLD_MIN;	
}

void reanimator_dsp(t_reanimator *x, t_signal **sp)
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
        reanimator_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(reanimator_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,sp[4]->s_vec);
    }
}
