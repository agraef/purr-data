/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *multyq_class;

#define OBJECT_NAME "multyq~"

typedef struct _multyq
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_float cf1;
	t_float gainfac1;
	t_float bw1;
	t_float cf2;
	t_float gainfac2;
	t_float bw2;
	t_float cf3;
	t_float gainfac3;
	t_float bw3;
	t_float cf4;
	t_float gainfac4;
	t_float bw4;
	t_float *rcos;
	t_float *filt;
	t_float *freqs;
	int rcoslen;
	short please_update;
	short always_update;
	short mute;
} t_multyq;

void multyq_dsp(t_multyq *x, t_signal **sp);
t_int *multyq_perform(t_int *w);
void *multyq_new(t_symbol *s, int argc, t_atom *argv);
void multyq_mute(t_multyq *x, t_floatarg state);
void update_filter_function(t_multyq *x);
void filtyQ( float *S, float *C, float *filtfunc, int N2 );
void multyq_init(t_multyq *x);
void multyq_free(t_multyq *x);
void multyq_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("multyq~"), (t_newmethod)multyq_new,
                  (t_method)multyq_free,sizeof(t_multyq), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_multyq, x_f);
	class_addmethod(c,(t_method)multyq_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)multyq_mute,gensym("mute"),A_FLOAT,0);
    multyq_class = c;
    fftease_announce(OBJECT_NAME);
}

void multyq_free(t_multyq *x)
{
    if(x->fft->initialized){
        free(x->rcos);
        free(x->freqs);
        free(x->filt);
    }
	fftease_free(x->fft);
    free(x->fft);
}

void *multyq_new(t_symbol *s, int argc, t_atom *argv)
{
    int i;
	t_fftease *fft;
	t_multyq *x = (t_multyq *)pd_new(multyq_class);
    for(i = 0; i < 12; i++){
        inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    }
    outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();	
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;		
	fft->initialized = 0;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void multyq_init(t_multyq *x)
{
	int i;
	t_float funda, base;
	
	t_fftease  *fft = x->fft;
	short initialized = fft->initialized;
	
	fftease_init(fft);

	if(!initialized){
		x->please_update = 0;
		x->always_update = 0;
		x->rcoslen = 8192 ;	
		x->rcos = (t_float *) calloc(x->rcoslen, sizeof( t_float ));
		x->freqs = (t_float *) calloc(fft->N2, sizeof( t_float ));
		x->filt = (t_float *) calloc((fft->N2 + 1), sizeof( t_float ));
		
		x->cf1  = 200.;
		x->gainfac1  = 0.0;
		x->bw1 = .15;
		x->cf2  = 700.;
		x->gainfac2  = 0.0;
		x->bw2  = .1; 
		x->cf3  = 3000.;
		x->gainfac3  = 0.0;
		x->bw3  = .15;
		x->cf4  = 12000.;
		x->gainfac4 = 0.0;
		x->bw4 = .15;
		x->mute = 0;
		for (i = 0; i < x->rcoslen; i++){
			x->rcos[i] =  .5 - .5 * cos(((float)i/(float)x->rcoslen) * TWOPI);
		}	
	} else {
		x->freqs = (t_float *) realloc(x->freqs, fft->N2 * sizeof( t_float ));
		x->filt = (t_float *) realloc(x->filt, (fft->N2 + 1) * sizeof( t_float ));
	}
	x->fft->input = (t_float *)realloc(fft->input, fft->Nw * sizeof(t_float));
    x->fft->output = (t_float *)realloc(fft->output, fft->Nw * sizeof(t_float));
	
	funda = base = (t_float)fft->R /(t_float)fft->N ;
	for(i = 0; i < fft->N2; i++){
		x->freqs[i] = base;
		base += funda;
	}
	update_filter_function(x);
}

void do_multyq(t_multyq *x)
{
	int real, imag, amp, phase;
	t_float a, b;
	int i;
	t_fftease *fft = x->fft;
	t_float *S = fft->buffer;
	t_float *C = fft->channel;
	t_float *filtfunc = x->filt;
	int N2 = fft->N2;
	fftease_fold(fft);
	fftease_rdft(fft,1);
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		a = ( i == N2 ? S[1] : S[real] );
		b = ( i == 0 || i == N2 ? 0. : S[imag] );
		C[amp] = hypot( a, b );
		C[amp] *= filtfunc[ i ];
		C[phase] = -atan2( b, a );
	}
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		S[real] = *(C+amp) * cos( *(C+phase) );
		if ( i != N2 )
			S[imag] = -*(C+amp) * sin( *(C+phase) );
	}
	fftease_rdft(fft,-1);
	fftease_overlapadd(fft);
}


t_int *multyq_perform(t_int *w)
{
	int i, j;
    t_multyq *x = (t_multyq *) (w[1]);
	t_float	*MSPInputVector = (t_float *)(w[2]);
	t_float	*in2 = (t_float *)(w[3]);
	t_float	*in3 = (t_float *)(w[4]);
	t_float	*in4 = (t_float *)(w[5]);
	t_float	*in5 = (t_float *)(w[6]);
	t_float	*in6 = (t_float *)(w[7]);
	t_float	*in7 = (t_float *)(w[8]);
	t_float	*in8 = (t_float *)(w[9]);
	t_float	*in9 = (t_float *)(w[10]);
	t_float	*in10 = (t_float *)(w[11]);
	t_float	*in11 = (t_float *)(w[12]);
	t_float	*in12 = (t_float *)(w[13]);
	t_float	*in13 = (t_float *)(w[14]);
	t_float	*MSPOutputVector = (t_float *)(w[15]);
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

    x->cf1 = *in2;
    x->bw1 = *in3;
    x->gainfac1 = *in4;
    x->cf2 = *in5;
    x->bw2 = *in6;
    x->gainfac2 = *in7;
    x->cf3 = *in8;
    x->bw3 = *in9;
    x->gainfac3 = *in10;
    x->cf4 = *in11;
    x->bw4 = *in12;
    x->gainfac4 = *in13;

	if(x->mute){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+16;
	}	
		
    update_filter_function(x);

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_multyq(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_multyq(x);
			
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
            
			do_multyq(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+16;
}		

void multyq_mute(t_multyq *x, t_floatarg state)
{
	x->mute = (short)state;	
}

void update_filter_function(t_multyq *x)
{
	t_float lo, hi ;
	t_float ploc, gainer;
	int i;
	t_float nyquist = (float)x->fft->R / 2.0;
	t_float *filt = x->filt;
	t_float *rcos = x->rcos;
	t_float *freqs = x->freqs;
	int rcoslen = x->rcoslen;
	int N2 = x->fft->N2;
	
	// sanity
	if( x->cf1 < 0 ){
		x->cf1 = 0;
	} 
	else if( x->cf1 > nyquist){
		x->cf1 = nyquist ;
	} 
	if( x->bw1 <= .05 ){
		x->bw1 = .05;
	}
	else if( x->bw1 > 1. ){
		x->bw1 = 1.;
	}
	if( x->gainfac1 < -1. ){
		x->gainfac1 = -1;
	}
	if( x->cf2 < 0 ){
		x->cf2 = 0;
	} 
	else if( x->cf2> nyquist){
		x->cf2 = nyquist ;
	} 
	if( x->bw2 <= .05 ){
		x->bw2 = .05;
	}
	else if( x->bw2 > 1. ){
		x->bw2 = 1.;
	}
	if( x->gainfac2 < -1. ){
		x->gainfac2 = -1;
	}
	if( x->cf3 < 0 ){
		x->cf3 = 0;
	} 
	else if( x->cf3 > nyquist){
		x->cf3 = nyquist ;
	} 
	if( x->bw3 <= .05 ){
		x->bw3 = .05;
	}
	else if( x->bw3 > 1. ){
		x->bw3 = 1.;
	}
	if( x->gainfac3 < -1. ){
		x->gainfac3 = -1;
	}
	if( x->cf4 < 0 ){
		x->cf4 = 0;
	} 
	else if( x->cf4 > nyquist){
		x->cf4 = nyquist ;
	} 
	if( x->bw4 <= .05 ){
		x->bw4 = .05;
	}
	else if( x->bw4 > 1. ){
		x->bw4 = 1.;
	}
	if( x->gainfac4 < -1. ){
		x->gainfac4 = -1;
	}
	for( i = 0; i < N2; i++ ) {
		x->filt[i] = 1.0 ;
	}
	// filt 1
	lo = x->cf1 * (1.0 - x->bw1 );
	hi = x->cf1 * (1.0 + x->bw1 );
	for( i = 0; i < N2; i++ ) {
		if(freqs[i] >= lo && freqs[i] <= hi){
			ploc = (freqs[i] - lo) / (hi - lo);
			gainer = 1 + x->gainfac1 * rcos[ (int) (ploc * rcoslen) ] ;
			if( gainer < 0 ){
				gainer = 0;
			}
			filt[i] *= gainer ;
			
		}
	}
	// filt 2
	lo = x->cf2 * (1.0 - x->bw2 );
	hi = x->cf2 * (1.0 + x->bw2 );
	for( i = 0; i < N2; i++ ) {
		if( freqs[i] >= lo && freqs[i] <= hi){
			ploc = (freqs[i] - lo) / (hi - lo);
			gainer = 1 + x->gainfac2 * rcos[ (int) (ploc * rcoslen) ] ;
			if( gainer < 0 ){
				gainer = 0;
			}
			filt[i] *= gainer ;
			
		}
	}
	// filt 3
	lo = x->cf3 * (1.0 - x->bw3 );
	hi = x->cf3 * (1.0 + x->bw3 );
	for( i = 0; i < N2; i++ ) {
		if(freqs[i] >= lo && freqs[i] <= hi){
			ploc = (freqs[i] - lo) / (hi - lo);
			gainer = 1 + x->gainfac3 * rcos[ (int) (ploc * rcoslen) ] ;
			if( gainer < 0 ){
				gainer = 0;
			}
			filt[i] *= gainer ;
			
		}
	}
	// filt 4
	lo = x->cf4 * (1.0 - x->bw4 );
	hi = x->cf4 * (1.0 + x->bw4 );
	for( i = 0; i < N2; i++ ) {
		if(freqs[i] >= lo && freqs[i] <= hi){
			ploc = (freqs[i] - lo) / (hi - lo);
			gainer = 1 + x->gainfac4 * rcos[ (int) (ploc * rcoslen) ] ;
			if( gainer < 0 ){
				gainer = 0;
			}
			filt[i] *= gainer ;
		}
	}
}

void filtyQ( float *S, float *C, float *filtfunc, int N2 )
{
	int real, imag, amp, phase;
	t_float a, b;
	int i;
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		a = ( i == N2 ? S[1] : S[real] );
		b = ( i == 0 || i == N2 ? 0. : S[imag] );
		C[amp] = hypot( a, b );
		C[amp] *= filtfunc[ i ];
		C[phase] = -atan2( b, a );
	}
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		S[real] = *(C+amp) * cos( *(C+phase) );
		if ( i != N2 )
			S[imag] = -*(C+amp) * sin( *(C+phase) );
	}
}

void multyq_dsp(t_multyq *x, t_signal **sp)
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
        multyq_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(multyq_perform, 15, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
                sp[4]->s_vec, sp[5]->s_vec, sp[6]->s_vec, sp[7]->s_vec,
                sp[8]->s_vec, sp[9]->s_vec, sp[10]->s_vec, sp[11]->s_vec,
                sp[12]->s_vec, sp[13]->s_vec);
    }
}