/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *pvharm_class;

#define OBJECT_NAME "pvharm~"

typedef struct _pvharm
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_fftease *fft2;
	short mute;
	short peakflag;
	t_float hifreq;/* highest frequency to synthesize */
	t_float lofreq;/* lowest frequency to synthesize */
	t_float framethresh; /* set lower limit for synth cutoff, to avoid noise */
	int osclimit;
	int oscnt;
	t_float local_thresh;
	t_float framepeak;
	t_float *ampsort; /* sort amplitudes from highest to lowest */
	short compressor; /* flag to compress */
	t_float framestop; /* amplitude below which compressor is turned off */
} t_pvharm;

void pvharm_dsp(t_pvharm *x, t_signal **sp);
t_int *pvharm_perform(t_int *w);
void *pvharm_new(t_symbol *s, int argc, t_atom *argv);
void pvharm_mute(t_pvharm *x, t_floatarg f);
void pvharm_init(t_pvharm *x);
void pvharm_rel2peak(t_pvharm *x, t_floatarg toggle);
void pvharm_free(t_pvharm *x);
void pvharm_oscnt(t_pvharm *x);
void pvharm_osclimit(t_pvharm *x, t_floatarg f);
void pvharm_compressor(t_pvharm *x, t_floatarg state);
void pvharm_framestop(t_pvharm *x, t_floatarg state);
void pvharm_lowfreq(t_pvharm *x, t_floatarg f);
void pvharm_highfreq(t_pvharm *x, t_floatarg f);

void pvharm_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("pvharm~"), (t_newmethod)pvharm_new,
                  (t_method)pvharm_free,sizeof(t_pvharm), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_pvharm, x_f);
	class_addmethod(c,(t_method)pvharm_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)pvharm_mute,gensym("mute"),A_FLOAT,0);

    class_addmethod(c,(t_method)pvharm_osclimit,gensym("osclimit"),A_FLOAT,0);
	class_addmethod(c,(t_method)pvharm_oscnt,gensym("oscnt"),0);
	class_addmethod(c,(t_method)pvharm_compressor,gensym("compressor"),A_FLOAT,0);
	class_addmethod(c,(t_method)pvharm_framestop,gensym("framestop"),A_FLOAT,0);
    class_addmethod(c,(t_method)pvharm_highfreq,gensym("highfreq"),A_FLOAT,0);
    class_addmethod(c,(t_method)pvharm_lowfreq,gensym("lowfreq"),A_FLOAT,0);
    pvharm_class = c;
    fftease_announce(OBJECT_NAME);
}

void pvharm_lowfreq(t_pvharm *x, t_floatarg f)
{
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    if(f > x->hifreq){
        error("%s: minimum cannot exceed current maximum: %f",OBJECT_NAME,x->hifreq);
        return;
    }
    if(f < 0 ){
        f = 0;
    }
    x->lofreq = f;
    fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
    fftease_oscbank_setbins(fft2,x->lofreq, x->hifreq);
}

void pvharm_highfreq(t_pvharm *x, t_floatarg f)
{
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    int R = x->fft->R;
    if(f < x->lofreq){
        error("%s: maximum cannot go below current minimum: %f",OBJECT_NAME,x->lofreq);
        return;
    }
    if(f > R/2 ){
        f = R/2;
    }
    x->hifreq = f;
    fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
    fftease_oscbank_setbins(fft2,x->lofreq, x->hifreq);
}

void pvharm_oscnt(t_pvharm *x)
{
	post("%s: osc count: %d, local thresh: %f, frame peak: %f",OBJECT_NAME, x->oscnt, x->local_thresh, x->framepeak);
}

void pvharm_free(t_pvharm *x)
{
    if(x->fft->initialized){
        free(x->ampsort);
    }
	fftease_free(x->fft);
	fftease_free(x->fft2);
    free(x->fft);
    free(x->fft2);
}

void pvharm_rel2peak(t_pvharm *x, t_floatarg toggle)
{
	x->peakflag = (short)toggle;
}

void *pvharm_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;
	t_pvharm *x = (t_pvharm *)pd_new(pvharm_class);

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
	x->lofreq = 0;
	x->hifreq = 15000;
	fft2->N = fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = fft2->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = fft2->overlap = (int) atom_getfloatarg(1, argc, argv); }

	return x;
}

void pvharm_init(t_pvharm *x)
{
	t_fftease  *fft = x->fft;
	t_fftease  *fft2 = x->fft2;
	short initialized = fft->initialized;
	fftease_init(fft);
	fftease_init(fft2);
    
	if(!initialized){
		x->framethresh = 0.005;
		x->osclimit = fft->N2;
		fft->P = .5 ; // for testing purposes
		fft2->P = .6666666666 ; // for testing purposes
		x->mute = 0;
		x->compressor = 0;
		x->framestop = .001;
		x->ampsort = (t_float *) calloc((fft->N+1), sizeof(t_float));
	} else if(initialized == 1){
		x->ampsort = (t_float *) realloc(x->ampsort, (fft->N+1) * sizeof(t_float));
	}
	fftease_oscbank_setbins(fft, x->lofreq, x->hifreq);
	fftease_oscbank_setbins(fft2, x->lofreq, x->hifreq);
}

void do_pvharm(t_pvharm *x)
{
	t_float framethresh = x->framethresh;
	int osclimit = x->osclimit;
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int i;
	int D = fft->D;
	int freq, amp,chan;
	t_float framesum, frame_rescale;
	t_float framestop = x->framestop;
	t_float *channel = fft->channel;
	t_float *channel2 = fft2->channel;
	t_float *output = fft->output;
	t_float *output2 = fft2->output;
	int lo_bin = fft->lo_bin;
	int hi_bin = fft->hi_bin;

	fftease_fold(fft);
	fftease_rdft(fft,FFT_FORWARD);
	fftease_convert(fft);
	
	if(x->compressor){
		framesum = 0.0;
		for(chan = fft->lo_bin; chan < fft->hi_bin; chan++){
			amp = chan << 1;
			framesum += channel[amp];
		}
		if(framesum > framestop && framesum >= 0.0){
			frame_rescale = 1.0 / framesum;
			for(chan = lo_bin; chan < hi_bin; chan++){
				amp = chan << 1;
				channel[amp] *= frame_rescale;		
			}
		}	
	}
	// copy spectrum to second channel (yes it is inefficient)

	for(chan = lo_bin; chan < hi_bin; chan++){
		amp = chan << 1;
		freq = amp + 1;
		channel2[amp] = channel[amp];
		channel2[freq] = channel[freq];
	} 
	fftease_limited_oscbank(fft, osclimit, framethresh);
	fftease_limited_oscbank(fft2, osclimit, framethresh);
	for(i = 0; i < D; i++){
		output[i] += output2[i];
	}
}

t_int *pvharm_perform(t_int *w)
{
	int i,j;
    t_pvharm *x = (t_pvharm *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *in2 = (t_float *)(w[3]);
	t_float *in3 = (t_float *)(w[4]);
	t_float *in4 = (t_float *)(w[5]);
	t_float *MSPOutputVector = (t_float *)(w[6]);
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float *input = fft->input;
	t_float *output = fft->output;
	t_float *output2 = fft2->output;
	t_float mult = fft->mult;
	int MSPVectorSize = fft->MSPVectorSize;
	t_float *internalInputVector = fft->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;		
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	
	if(x->mute){
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *MSPInputVector++ * FFTEASE_BYPASS_GAIN;
		}
		return w+7;
	}

    fft->P = *in2;
    fft2->P = *in3;
    fft->synt = fft2->synt =  *in4;
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
		do_pvharm(x);
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
        memcpy(output2, output2 + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output2[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_pvharm(x);
			for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
            memcpy(output, output + D, (Nw-D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
            memcpy(output2, output2 + D, (Nw-D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output2[j] = 0.0; }
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        memcpy(internalInputVector + (operationCount * MSPVectorSize), MSPInputVector,MSPVectorSize * sizeof(t_float));
        memcpy(MSPOutputVector, internalOutputVector + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_float));
		operationCount = (operationCount + 1) % operationRepeat;
		
		if( operationCount == 0 ) {
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw - D), internalInputVector, D * sizeof(t_float));
			
			do_pvharm( x );
            
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
            memcpy(output2, output2 + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output2[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+7;
}		

void pvharm_compressor(t_pvharm *x, t_floatarg state)
{
	x->compressor = (short)state;	
}

void pvharm_framestop(t_pvharm *x, t_floatarg state)
{
	x->framestop = state;	
}

void pvharm_mute(t_pvharm *x, t_floatarg state)
{
	x->mute = (short)state;	
}

void pvharm_osclimit(t_pvharm *x, t_floatarg limit)
{
	x->osclimit = (int)limit;	
}

void pvharm_dsp(t_pvharm *x, t_signal **sp)
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
        pvharm_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(pvharm_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,sp[3]->s_vec,
                sp[4]->s_vec);
    }
}

