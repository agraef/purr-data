/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *scrape_class;

#define OBJECT_NAME "scrape~"

typedef struct _scrape
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_float knee;
	t_float cutoff;
	t_float scrape_mult;
	t_float thresh1;
	t_float thresh2;
	t_float *threshfunc;
	short mute;
} t_scrape;

void scrape_dsp(t_scrape *x, t_signal **sp);
t_int *scrape_perform(t_int *w);
void *scrape_new(t_symbol *msg, short argc, t_atom *argv);
void update_thresh_function( t_scrape *x );
void scrape_frowned( float *S, float *C, float *threshfunc, float fmult, int N2 );
void scrape_mute(t_scrape *x, t_floatarg toggle);
void scrape_free( t_scrape *x );
void update_thresh_function( t_scrape *x );
void scrape_init(t_scrape *x);

void scrape_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("scrape~"), (t_newmethod)scrape_new,
                  (t_method)scrape_free,sizeof(t_scrape), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_scrape, x_f);
	class_addmethod(c,(t_method)scrape_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)scrape_mute,gensym("mute"),A_FLOAT,0);

    scrape_class = c;
    fftease_announce(OBJECT_NAME);
}

void scrape_free( t_scrape *x )
{
    if(x->fft->initialized){
        free(x->threshfunc);
    }
    fftease_free(x->fft);
    free(x->fft);
}

void *scrape_new(t_symbol *msg, short argc, t_atom *argv)
{
    t_fftease *fft;
	t_scrape *x = (t_scrape *)pd_new(scrape_class);

	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
    
	x->fft = (t_fftease *) calloc(1, sizeof(t_fftease) );
	fft = x->fft;
	fft->initialized = 0;
	x->knee = 1000.0;
	x->cutoff = 4000.0;
	x->thresh1 = 0.001;
	x->thresh2 = 0.09;
	x->scrape_mult = 0.1;

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;

    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void scrape_init(t_scrape *x)
{
	t_fftease  *fft = x->fft;
	short initialized = fft->initialized;
	
	fftease_init(fft);
		
	if(!initialized){
		x->mute = 0;
		x->threshfunc = (t_float *) calloc(fft->N2, sizeof(t_float));
		update_thresh_function(x);
	} else if(initialized == 1){
		x->threshfunc = (t_float *) realloc(x->threshfunc, fft->N2 * sizeof(t_float));
		update_thresh_function(x);
	}
}

void update_thresh_function( t_scrape *x )
{
	t_float funda, curfreq, m1, m2;
	int i;
	int R = x->fft->R;
	int N = x->fft->N;
	int N2 = x->fft->N2;
	
	funda = (t_float)  R / ((t_float)N);
	curfreq = funda ;
	for( i = 0; i < N2; i++ ) {
		if( curfreq  < x->knee ){
			x->threshfunc[i] = 0.0 ;
		} else if( curfreq >= x->knee && curfreq < x->cutoff ) {
			m2 = (x->knee - curfreq) / (x->cutoff - x->knee) ;
			m1 = 1.0 - m2 ;
			x->threshfunc[i] = m1 * x->thresh1 + m2 * x->thresh2 ;
		} else {
			x->threshfunc[i] = x->thresh2;
		}
		curfreq += funda ;
	}
}

void scrape_mute(t_scrape *x, t_floatarg toggle)
{
	x->mute = (short)toggle;	
}

void do_scrape(t_scrape *x)
{
	int real, imag, amp, phase;
	t_float a, b;
	int i;
	t_float maxamp = 0.0;
	t_fftease *fft = x->fft;
	int N2 = fft->N2;
	t_float scrape_mult = x->scrape_mult;
	t_float *channel = fft->channel;
	t_float *buffer = fft->buffer;
	t_float *threshfunc = x->threshfunc;
	
	fftease_fold(fft);
	fftease_rdft(fft,FFT_FORWARD);
	
	for( i = 0; i <= N2; i++ ){
		amp = i<<1;
		if( maxamp < channel[amp] ){
			maxamp = channel[amp];
		}
	}
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		a = ( i == N2 ? buffer[1] : buffer[real] );
		b = ( i == 0 || i == N2 ? 0. : buffer[imag] );
		channel[amp] = hypot( a, b );
		
		if ( (channel[amp]) < threshfunc[i] * maxamp ){
			channel[amp] *= scrape_mult;
		}
		channel[phase] = -atan2( b, a );
	}
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		buffer[real] = *(channel+amp) * cos( *(channel+phase) );
		if ( i != N2 )
			buffer[imag] = -*(channel+amp) * sin( *(channel+phase) );
	}

	fftease_rdft(fft, FFT_INVERSE);
	fftease_overlapadd(fft);
}

t_int *scrape_perform(t_int *w)
{
	int	i,j;
	t_float tmp ;
    t_scrape *x = (t_scrape *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *knee_freq = (t_float *)(w[3]);
	t_float *cut_freq = (t_float *)(w[4]);
	t_float *thresh1 = (t_float *)(w[5]);
	t_float *thresh2 = (t_float *)(w[6]);
	t_float *scrape_mult = (t_float *)(w[7]);
	t_float *MSPOutputVector = (t_float *)(w[8]);
	
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
		return w+9;
	}
    tmp = *knee_freq++;
    if( tmp > 50 && tmp < 20000 ){
        x->knee = tmp;
    }
    tmp = *cut_freq++;
    if( tmp > x->knee && tmp < 20000 ){
        x->cutoff = *cut_freq++;
    }
    x->thresh1 = *thresh1;
    x->thresh2 = *thresh2;
    x->scrape_mult = *scrape_mult;
    update_thresh_function( x );

    // 
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_scrape(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_scrape(x);
			
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
            
			do_scrape(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+9;
}		

void scrape_dsp(t_scrape *x, t_signal **sp)
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
        scrape_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(scrape_perform, 8, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
                sp[4]->s_vec, sp[5]->s_vec, sp[6]->s_vec);
    }
}
