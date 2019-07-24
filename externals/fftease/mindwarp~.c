/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *mindwarp_class;

#define OBJECT_NAME "mindwarp~"

#define MAX_WARP 16.0

typedef struct _mindwarp
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
    t_float warpFactor;
    t_float shapeWidth;
	t_float *newChannel;
    t_float *newAmplitudes;
	short mute;
} t_mindwarp;

void mindwarp_dsp(t_mindwarp *x, t_signal **sp);
t_int *mindwarp_perform(t_int *w);
void *mindwarp_new(t_symbol *s, int argc, t_atom *argv);
void mindwarp_dest(t_mindwarp *x, t_float f);
void mindwarp_init(t_mindwarp *x);
void mindwarp_free(t_mindwarp *x);
void mindwarp_mute(t_mindwarp *x, t_floatarg toggle);

void mindwarp_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("mindwarp~"), (t_newmethod)mindwarp_new,
                  (t_method)mindwarp_free,sizeof(t_mindwarp), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_mindwarp, x_f);
	class_addmethod(c,(t_method)mindwarp_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)mindwarp_mute,gensym("mute"),A_FLOAT,0);
    mindwarp_class = c;
    fftease_announce(OBJECT_NAME);
}

void *mindwarp_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_mindwarp *x = (t_mindwarp *)pd_new(mindwarp_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	
	/* args: warpfactor, shape width, overlap, window factor */
	
  	x->warpFactor = 1.0;
  	x->shapeWidth = 3.0;
	
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;

    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
	
}

void mindwarp_init(t_mindwarp *x)
{
	short initialized = x->fft->initialized;
	
	fftease_init(x->fft);
	
	if(!initialized){
		x->mute = 0;
		x->newAmplitudes = (t_float *)calloc(((x->fft->N2 + 1) * 16), sizeof(t_float));
		x->newChannel = (t_float *)calloc ((x->fft->N + 1), sizeof(t_float));
	}
    else if(initialized == 1) {
		x->newAmplitudes = (t_float *)realloc(x->newAmplitudes, ((x->fft->N2 + 1) * 16) * sizeof(float));
		x->newChannel = (t_float *)realloc(x->newChannel, (x->fft->N + 1) * sizeof(t_float));
	}
}

void mindwarp_free(t_mindwarp *x)
{
    short initialized = x->fft->initialized;
	fftease_free(x->fft);
    free(x->fft);
    if(initialized){
        free(x->newAmplitudes);
        free(x->newChannel);
    }
}

void do_mindwarp(t_mindwarp *x)
{
	t_float *newChannel = x->newChannel;
	
	int		
	i,j,
	bindex,
	N,
	N2,
	Nw,
	shapeWidth = (int) x->shapeWidth,
	remainingWidth,
	newLength;
float
	cutoff,
	filterMult,
	interpIncr,
	interpPhase;
	t_float warpFactor;
	t_fftease *fft = x->fft;
	t_float *newAmplitudes = x->newAmplitudes;
	t_float *channelOne = fft->channel;
	
	N = fft->N;
	N2 = fft->N2;
	Nw = fft->Nw;
	warpFactor = x->warpFactor;
	cutoff = (t_float) N2 * .9;
	filterMult = .00001;
	
	fftease_fold(fft);
	fftease_rdft(fft,FFT_FORWARD);
	fftease_leanconvert(fft);
	
	if(warpFactor <= 0){
		error("bad warp, resetting");
		warpFactor = 1.0;
	}
	
	newLength = (int) ((t_float) N2 / warpFactor);
	
	if(newLength <= 0){
		error("bad length: resetting");
		newLength = 1.0;
	}
	
	interpIncr = (t_float) N2 / (t_float) newLength;
	
	interpPhase = 0.;
	
	
	// do simple linear interpolation on magnitudes
	
	for ( bindex=0; bindex < newLength; bindex++ ) {
		
		int		localbindex = ((int) interpPhase) << 1;
		
		t_float	lower = *(channelOne + localbindex),
		upper = *(channelOne + localbindex + 2),
		diff = interpPhase - ( (t_float) ( (int) interpPhase ) );
		
		*(newAmplitudes+bindex) = lower + ( ( upper - lower ) * diff );
		
		interpPhase += interpIncr;
	}
	
	
	
	// replace magnitudes with warped values 
	
	if (warpFactor > 1.) {
		
		int	until = (int) ( cutoff / warpFactor );
		
		for ( bindex=0; bindex < until; bindex++ ) {
			register int	amp = bindex<<1;
			
			*(newChannel+amp) = *(newAmplitudes+bindex);
		}
		
		
		// filter remaining spectrum as spectral envelope has shrunk 
		
		for ( bindex=until; bindex < N2; bindex++ ) {
			register int	amp = bindex<<1;
			
			*(newChannel+amp) *= filterMult;
		}
	}
	
	
	//OK
	
	// spectral envelope has enlarged, no post filtering is necessary
	
	else {
		
		for ( bindex=0; bindex <= N2; bindex++ ) {
			register int	amp = bindex<<1;
			
			*(newChannel+amp) = *(newAmplitudes+bindex);
		}
	}
	
	
	
	// constrain our shapeWidth value
	
	if ( shapeWidth > N2 )
		shapeWidth = N2;
  	
	if ( shapeWidth < 1 )
		shapeWidth = 1;
	
	// lets just shape the entire signal by the shape width 
	
	
	for ( i=0; i < N; i += shapeWidth << 1 ) {
		
		t_float       amplSum = 0.,
		freqSum = 0.,
		factor = 1.0;
		
		for ( j = 0; j < shapeWidth << 1; j += 2 ) {
			
			amplSum += *(newChannel+i+j);
			freqSum += *(channelOne+i+j);
		}
		
		if (amplSum < 0.000000001)
			factor = 0.000000001;	
		
		// this can happen, crashing external; now fixed.
		
		if( freqSum <= 0 ){
			//		error("bad freq sum, resetting");
			freqSum = 1.0;
		}
		else	
			factor = amplSum / freqSum;
		
		for ( j = 0; j < shapeWidth << 1; j += 2 )
			*(channelOne+i+j) *= factor;
	}
	
	// copy remaining magnitudes (fixed shadowed variable warning by renaming bindex)
	
	if ( (remainingWidth = N2 % shapeWidth) ) {
		
		int			lbindex = (N2 - remainingWidth) << 1;
		
		
		t_float       amplSum = 0.,
		freqSum = 0.,
		factor;
		
		for ( j = 0; j < remainingWidth << 1; j += 2 ) {
			
			amplSum += *(newChannel+lbindex+j);
			freqSum += *(channelOne+lbindex+j);
		}
		
		if (amplSum < 0.000000001)
			factor = 0.000000001;	
		
		else	
			factor = amplSum / freqSum;
		
		for ( j = 0; j < remainingWidth << 1; j += 2 )
			*(channelOne+bindex+j) *= factor;
	}

	fftease_leanunconvert(fft);

	fftease_rdft(fft,FFT_INVERSE);
	fftease_overlapadd(fft);
	
}


t_int *mindwarp_perform(t_int *w)
{
    t_mindwarp *x = (t_mindwarp *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *vec_warpFactor = (t_float *)(w[3]);
	t_float *vec_shapeWidth = (t_float *)(w[4]);
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

	x->warpFactor = *vec_warpFactor;
	x->shapeWidth =  *vec_shapeWidth;
	
	if(x->warpFactor <= 0.0){
		x->warpFactor = 0.1;
		error("%s: zero warp factor is illegal",OBJECT_NAME);
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_mindwarp(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_mindwarp(x);
			
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
            
			do_mindwarp(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}	

void mindwarp_mute(t_mindwarp *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void mindwarp_dsp(t_mindwarp *x, t_signal **sp)
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
        mindwarp_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(mindwarp_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
    }
}

