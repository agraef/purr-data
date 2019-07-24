/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *schmear_class;

#define OBJECT_NAME "schmear~"
#define MAXSCHMEAR (129)

typedef struct _schmear
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_float schmearmult;
	short mute;
	t_float *spreader;
	t_float *newamps;
	int spreadlen;
	t_float threshold;
	short freakmode;
	int shift;
} t_schmear;

void schmear_dsp(t_schmear *x, t_signal **sp);
t_int *schmear_perform(t_int *w);
void *schmear_new(t_symbol *s, int argc, t_atom *argv);
void schmear_mute(t_schmear *x, t_floatarg toggle);
void schmear_rel2peak(t_schmear *x, t_floatarg toggle);
void schmear_free(t_schmear *x);
void schmear_init(t_schmear *x);
void schmear_threshold(t_schmear *x, t_floatarg f);
void schmear_schmimp(t_schmear *x, t_symbol *msg, short argc, t_atom *argv);
void schmear_shift(t_schmear *x, t_floatarg f);
void schmear_oscbank(t_schmear *x, t_floatarg flag);

void schmear_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("schmear~"), (t_newmethod)schmear_new,
                  (t_method)schmear_free,sizeof(t_schmear), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_schmear, x_f);
	class_addmethod(c,(t_method)schmear_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)schmear_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)schmear_threshold,gensym("threshold"),A_FLOAT,0);
	class_addmethod(c,(t_method)schmear_shift,gensym("shift"),A_FLOAT,0);
	class_addmethod(c,(t_method)schmear_schmimp,gensym("schmimp"),A_GIMME,0);
    
    schmear_class = c;
    fftease_announce(OBJECT_NAME);
}

void schmear_oscbank(t_schmear *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void schmear_mute(t_schmear *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void schmear_shift(t_schmear *x, t_floatarg f)
{
	x->shift = (int)f;
}

void schmear_schmimp(t_schmear *x, t_symbol *msg, short argc, t_atom *argv)
{
	int i;
	if(argc > MAXSCHMEAR){
		post("%d is too long for schmear", argc);
		return;
	}
	if(! (argc % 2) ){
		post("%s: impulse length %d must be odd",OBJECT_NAME, argc);
		return;
	}
	for( i = 0; i < argc; i++ ){
		x->spreader[i] = atom_getfloatarg(i,argc,argv);
	}
	x->spreadlen = argc;
}

void schmear_threshold(t_schmear *x, t_floatarg t)
{
	x->threshold = (float)t;
}

void *schmear_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_schmear *x = (t_schmear *)pd_new(schmear_class);
    outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	x->threshold = 0.1;
	x->schmearmult = 0.1;
	x->mute = 0;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void schmear_init(t_schmear *x)
{

	t_fftease  *fft = x->fft;
	fftease_init(fft);
	x->newamps = (t_float *) calloc((fft->N2+1), sizeof(t_float));
	x->spreadlen = 7;
	x->spreader = (t_float *) calloc(MAXSCHMEAR, sizeof(t_float));
	x->spreader[0] = 0.6;
	x->spreader[1] = 0.3;
	x->spreader[2] = 0.15;
	x->spreader[3] = 0.0;
	x->spreader[4] = 0.15;
	x->spreader[5] = 0.3;
	x->spreader[6] = 0.6;
	x->shift = 0;
}

void schmear_free(t_schmear *x)
{
    if(x->fft->initialized){
        free(x->newamps);
        free(x->spreader);
    }
	fftease_free(x->fft);
    free(x->fft);
}

void do_schmear(t_schmear *x)
{
	int i, j;
	t_fftease *fft = x->fft;
	t_float *channel = fft->channel;
	t_float frame_peak = 0.0, local_thresh;
	t_float threshold = x->threshold;
	int shift = x->shift;
	int N = fft->N;
	int N2 = fft->N2;
	t_float *newamps = x->newamps;
	t_float *spreader = x->spreader;
	t_float curamp;
	int spreadlen = x->spreadlen;
	int spread_center = (spreadlen - 1) / 2;
	int thisbin;
	
	fftease_fold(fft);
	fftease_rdft(fft,1);
	fftease_convert(fft);

	for(i = 0; i < N; i += 2){	
		if(frame_peak < channel[i])
			frame_peak = channel[i];
	}
	local_thresh = frame_peak * threshold;
	for(i = 0; i < N2; i++){
		newamps[i] = 0.0;
	}
	/*
	if( freakmode ){ // weird mistake version
		for(i = 0; i < N2; i++){	
			if(channel[i * 2] > local_thresh){
				curamp = channel[i * 2];
				for(j = i - spread_center; j <= i + spread_center; j++){
					if(j >= 0 && j < N2){
						newamps[j] += curamp * spreader[j + spread_center];
					}
				}
			}
		}  
	} 
	*/
	// no spread for now
	
		for(i = 0; i < N2; i++){	
			curamp = channel[i * 2];
			if(curamp > local_thresh){

				for(j = 0; j < spreadlen; j++){
					thisbin = i + j - spread_center;
					if(thisbin >= 0 && thisbin < N2){
						newamps[thisbin] += curamp * spreader[j];
					}
				}
			
			} else {
				newamps[i] = curamp;
			}
		}

    if( shift > 0 ){
        for( i = 0; i < N2; i++){
            channel[i * 2] = newamps[i];
        }
        for( i = 0; i < N2; i++){
            newamps[(i + shift) % N2] = channel[i * 2];
        }
    }
	// move amps back where they belong
	for(i = 0; i < N2; i++){
		channel[i * 2] = newamps[i];
	}
	fftease_unconvert(fft);
	fftease_rdft(fft,-1);
	fftease_overlapadd(fft);

}

t_int *schmear_perform(t_int *w)
{
	int	i,j;
    t_schmear *x = (t_schmear *) (w[1]);
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
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *MSPInputVector++ * FFTEASE_BYPASS_GAIN;
		}
		return w+4;
	}

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_schmear(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_schmear(x);
			
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
            
			do_schmear(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+4;
}		

void schmear_dsp(t_schmear *x, t_signal **sp)
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
        schmear_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(schmear_perform, 3, x, sp[0]->s_vec, sp[1]->s_vec);
    }
}
