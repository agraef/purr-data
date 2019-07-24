/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *pvcompand_class;

#define OBJECT_NAME "pvcompand~"

typedef struct _pvcompand
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_float rescale;
	t_float *curthresh;
	t_float *atten;
	t_float *thresh;
	int count;
	t_float thresh_interval;
	t_float max_atten; 
	t_float atten_interval;
	t_float tstep;
	t_float gstep;
	t_float last_max_atten;
	short norml;
	short mute;
} t_pvcompand;

void pvcompand_dsp(t_pvcompand *x, t_signal **sp);
t_int *pvcompand_perform(t_int *w);
void *pvcompand_new(t_symbol *s, int argc, t_atom *argv);
void update_thresholds(t_pvcompand *x);
void pvcompand_normalize(t_pvcompand *x, t_floatarg val);
void pvcompand_float(t_pvcompand *x, t_float f);
void pvcompand_free(t_pvcompand *x);
float pvcompand_ampdb(float db);
void pvcompand_init(t_pvcompand *x);
void pvcompand_mute(t_pvcompand *x, t_floatarg f);

void pvcompand_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("pvcompand~"), (t_newmethod)pvcompand_new,
                  (t_method)pvcompand_free,sizeof(t_pvcompand), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_pvcompand, x_f);
	class_addmethod(c,(t_method)pvcompand_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)pvcompand_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)pvcompand_normalize,gensym("normalize"), A_FLOAT, 0);
    pvcompand_class = c;
    fftease_announce(OBJECT_NAME);
}

void pvcompand_mute(t_pvcompand *x, t_floatarg f)
{
	x->mute = (short)f;
}


void pvcompand_free( t_pvcompand *x ){
    if(x->fft->initialized){
        free(x->curthresh);
        free(x->atten);
        free(x->thresh);
    }
	fftease_free(x->fft);
    free(x->fft);
}

void *pvcompand_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_pvcompand *x = (t_pvcompand *)pd_new(pvcompand_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	x->max_atten = -6.0;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void pvcompand_init(t_pvcompand *x)
{
	t_fftease  *fft = x->fft;
	short initialized = x->fft->initialized;

	fftease_init(fft);
	
	if(!initialized){
		x->norml = 0;
		x->mute = 0;
		x->thresh_interval = 1.0;
		x->last_max_atten =  x->max_atten; 
		x->atten_interval = 2.0 ; 
		x->tstep = 1.0 ;
		x->gstep = 2.0 ;
		x->thresh = (t_float *) calloc((fft->N), sizeof(t_float));
		x->atten = (t_float *) calloc(fft->N, sizeof(t_float) );
		x->curthresh = (t_float *) calloc(fft->N, sizeof(t_float) );
	} else if(initialized == 1) {
		x->thresh = (t_float *) realloc(x->thresh, fft->N * sizeof(t_float));
		x->atten = (t_float *) realloc(x->atten, fft->N * sizeof(t_float));
		x->curthresh = (t_float *) realloc(x->curthresh, fft->N * sizeof(t_float));
	}
	update_thresholds(x);
}

void update_thresholds( t_pvcompand *x ) {
	int i;
	t_float nowamp = x->max_atten ;
	t_float nowthresh = 0.0 ;
	int N = x->fft->N;
	x->count = 0;
	if( nowamp < 0.0 ){
		while( nowamp < 0.0 ){
			x->atten[x->count] = pvcompand_ampdb( nowamp );
			nowamp += x->gstep ;
			++(x->count);
			if(x->count >= N){
				error("count exceeds %d",N);
				x->count = N - 1;
				break;
			}
		}
    }
	else if( nowamp > 0.0 ){
		while( nowamp > 0.0 ){
			x->atten[x->count] = pvcompand_ampdb( nowamp );
			nowamp -= x->gstep ;
			++(x->count);
			if(x->count >= N){
				error("count exceeds %d",N);
				x->count = N - 1;
				break;
			}
		}
	}
	for( i = 0; i < x->count; i++){
		x->thresh[i] = pvcompand_ampdb( nowthresh );
		nowthresh -= x->tstep ;
	}
}

void pvcompand_normalize(t_pvcompand *x, t_floatarg val) 
{
	x->norml = (short)val;
}

void do_pvcompand(t_pvcompand *x)
{
	t_fftease *fft = x->fft;
	t_float *channel = fft->channel;
	int N = fft->N;
	t_float *curthresh = x->curthresh;
	t_float *thresh = x->thresh;
	t_float *atten = x->atten;
	int count = x->count;
	t_float max_atten = x->max_atten;
	int i,j;	
	t_float maxamp ;	
	t_float cutoff;
	t_float avr, new_avr, rescale;
	
	fftease_fold(fft);
	fftease_rdft(fft,FFT_FORWARD);
	fftease_leanconvert(fft);

	maxamp = 0.;
	avr = 0;
	for( i = 0; i < N; i+= 2 ){
		avr += channel[i];
		if( maxamp < channel[i] ){
			maxamp = channel[i] ;
		}
	}
	if(count <= 1){
		//	post("count too low!"); 
		count = 1;
	}
	for( i = 0; i < count; i++ ){
		curthresh[i] = thresh[i]*maxamp ;
	}
	cutoff = curthresh[count-1];
	new_avr = 0;
	for( i = 0; i < N; i += 2){
		if( channel[i] > cutoff ){
			j = count-1;
			while( channel[i] > curthresh[j] ){
				j--;
				if( j < 0 ){
					j = 0;
					break;
				}
			}
			channel[i] *= atten[j];
		}
		new_avr += channel[i] ;
	}
	
	if( x->norml ) {
		if( new_avr <= 0 ){
			new_avr = .0001;
		}
		rescale =  avr / new_avr ;
		
	} else {
		rescale = pvcompand_ampdb( max_atten * -.5); 
	}
	for( i = 0; i < N; i += 2){
		channel[i] *= rescale;
	} 
	
	fftease_leanunconvert(fft);
	fftease_rdft(fft, FFT_INVERSE);
	fftease_overlapadd(fft);
}

t_int *pvcompand_perform(t_int *w)
{
	int i,j;
    t_pvcompand *x = (t_pvcompand *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *in2 = (t_float *)(w[3]);
	t_float *MSPOutputVector = (t_float *)(w[4]);
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
		return w+5;
	}

    x->max_atten = *in2;
    if(x->max_atten != x->last_max_atten) {
        x->last_max_atten = x->max_atten;
        update_thresholds(x);
    }
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_pvcompand(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_pvcompand(x);
			
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
            
			do_pvcompand(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+5;
}	

float pvcompand_ampdb(float db) 
{
	float amp;
	amp = pow((t_float)10.0, (t_float)(db/20.0)) ;
	return(amp);
}

void pvcompand_dsp(t_pvcompand *x, t_signal **sp)
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
        pvcompand_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(pvcompand_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
    }
}


