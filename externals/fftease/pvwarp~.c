/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *pvwarp_class;

#define OBJECT_NAME "pvwarp~"

typedef struct _pvwarp
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_float hifreq; // user specified highest synthfreq
	t_float lofreq;//user speficied lowest synthfreq
	t_float topfreq;
	short *connections;
	short mute;
	short bypass;
	int pitch_connected;
	int synt_connected;
	t_float *warpfunc ;
	short please_update;
	short always_update;
	t_float cf1;
	t_float bw1;
	t_float warpfac1;
	t_float cf2;
	t_float bw2;
	t_float warpfac2;
	int funcoff;
	short verbose;
	short automate;
	long fftsize_attr;
	long overlap_attr;
} t_pvwarp;

void pvwarp_dsp(t_pvwarp *x, t_signal **sp);
t_int *pvwarp_perform(t_int *w);
void *pvwarp_new(t_symbol *s, int argc, t_atom *argv);
void pvwarp_mute(t_pvwarp *x, t_floatarg state);
void pvwarp_automate(t_pvwarp *x, t_floatarg state);
void pvwarp_autofunc(t_pvwarp *x, t_floatarg minval, t_floatarg maxval);
void pvwarp_free( t_pvwarp *x );
float closestf(float test, float *arr) ;
int freq_to_bin( float target, float fundamental );
void update_warp_function( t_pvwarp *x ) ;
void pvwarp_init(t_pvwarp *x);
void pvwarp_bottomfreq(t_pvwarp *x, t_floatarg f);
void pvwarp_topfreq(t_pvwarp *x, t_floatarg f);
void pvwarp_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("pvwarp~"), (t_newmethod)pvwarp_new,
                  (t_method)pvwarp_free,sizeof(t_pvwarp), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_pvwarp, x_f);
	class_addmethod(c,(t_method)pvwarp_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)pvwarp_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)pvwarp_bottomfreq,gensym("bottomfreq"),A_FLOAT,0);
	class_addmethod(c,(t_method)pvwarp_topfreq,gensym("topfreq"),A_FLOAT,0);
	class_addmethod(c,(t_method)pvwarp_autofunc,gensym("autofunc"),A_DEFFLOAT, A_DEFFLOAT,0);
    pvwarp_class = c;
    fftease_announce(OBJECT_NAME);
}

void pvwarp_automate(t_pvwarp *x, t_floatarg state)
{
	x->automate = (short)state;
}

void update_warp_function( t_pvwarp *x )
{
	int i,j;
	int N2 = x->fft->N2;
	t_float *warpfunc = x->warpfunc;
	t_float warpfac1 = x->warpfac1;
	t_float warpfac2 = x->warpfac2;
	t_float cf1 = x->cf1;
	t_float cf2 = x->cf2;
	t_float bw1 = x->bw1;
	t_float bw2 = x->bw2;
	t_float c_fundamental = x->fft->c_fundamental;
	t_float deviation;
	t_float diff;
	int midbin, lobin, hibin ;
	t_float hif, lof;
	int bin_extent;
	short verbose = x->verbose;
	
	for( i = 0; i < N2; i++ ){
		warpfunc[i] = 1.0;
	}
	hif = cf1 * (1. + bw1);
	lof = cf1 * (1. - bw1);
	midbin = freq_to_bin( cf1, c_fundamental );
	hibin = freq_to_bin( hif, c_fundamental );
	lobin = freq_to_bin( lof, c_fundamental );
	if( hibin >= N2 - 1 ){
		hibin = N2 - 1;
	}
	if( lobin < 0 ){
		lobin = 0;
	} 
	if( verbose )
		post("bump1: hi %d mid %d lo %d",hibin,midbin,lobin);
	
	warpfunc[midbin] = warpfac1;
	diff = warpfac1 - 1.0 ;
	bin_extent = hibin - midbin ;
	for( i = midbin, j = 0; i < hibin; i++, j++ ){
		deviation = diff * ((float)(bin_extent - j) / (float) bin_extent );
		warpfunc[ i ] += deviation ;
	}
	bin_extent = midbin - lobin ;
	for( i = midbin, j = 0; i > lobin; i--, j++ ){
		deviation = diff * ((float)(bin_extent - j) / (float) bin_extent );
		warpfunc[ i ] += deviation ;
	}
	
	// NOW DO SECOND BUMP
	hif = cf2 * (1. + bw2);
	lof = cf2 * (1. - bw2);
	midbin = freq_to_bin( cf2, c_fundamental );
	hibin = freq_to_bin( hif, c_fundamental );
	lobin = freq_to_bin( lof, c_fundamental );
	if( hibin >= N2 - 1 ){
		hibin = N2 - 1;
	}
	if( lobin < 0 ){
		lobin = 0;
	} 
	if( verbose )
		post("bump2: hi %d mid %d lo %d",hibin,midbin,lobin);
	warpfunc[midbin] = warpfac2;
	diff = warpfac2 - 1.0 ;
	bin_extent = hibin - midbin ;
	for( i = midbin, j = 0; i < hibin; i++, j++ ){
		deviation = diff * ((float)(bin_extent - j) / (float) bin_extent );
		warpfunc[ i ] += deviation ;
	}
	bin_extent = midbin - lobin ;
	for( i = midbin, j = 0; i > lobin; i--, j++ ){
		deviation = diff * ((float)(bin_extent - j) / (float) bin_extent );
		warpfunc[ i ] += deviation ;
	}
	
	x->please_update = 0;	
}

void pvwarp_autofunc(t_pvwarp *x, t_floatarg minval, t_floatarg maxval)
{
	int minpoints, maxpoints, segpoints, i;
	int pointcount = 0;
	t_float target, lastval;
	t_float m1, m2;
	int N2 = x->fft->N2;
	t_float *warpfunc = x->warpfunc;
	
	minpoints = 0.05 * (float) N2;
	maxpoints = 0.25 * (float) N2;
	if( minval > 1000.0 || minval < .001 ){
		minval = 0.5;
	}
	if( maxval < 0.01 || maxval > 1000.0 ){
		minval = 2.0;
	}
	
	lastval = fftease_randf(minval, maxval);
	// post("automate: min %d max %d",minpoints, maxpoints);
	while( pointcount < N2 ){
		target = fftease_randf(minval, maxval);
		segpoints = minpoints + (rand() % (maxpoints-minpoints));
		if( pointcount + segpoints > N2 ){
			segpoints = N2 - pointcount;
		}
		for( i = 0; i < segpoints; i++ ){
			m2 = (float)i / (float) segpoints ;
			m1 = 1.0 - m2;
			warpfunc[ pointcount + i ] = m1 * lastval + m2 * target;
		}
		lastval = target;
		pointcount += segpoints;
	}
}

void pvwarp_mute(t_pvwarp *x, t_floatarg state)
{
	x->mute = state;	
}

void pvwarp_free( t_pvwarp *x ){
    if(x->fft->initialized){
        free(x->warpfunc);
        free(x->connections);
    }
	fftease_free(x->fft);
    free(x->fft);
}

void *pvwarp_new(t_symbol *s, int argc, t_atom *argv)
{
    int i;
	t_fftease *fft;
	t_pvwarp *x = (t_pvwarp *)pd_new(pvwarp_class);
    for(i = 0; i < 9; i++){
        inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    }
	outlet_new(&x->x_obj, gensym("signal"));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	x->lofreq = 0.0;
	x->hifreq = 18000.0;

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void pvwarp_init(t_pvwarp *x)
{
	
	t_fftease  *fft = x->fft;
	
	fftease_init(fft);
		srand(clock());
		x->please_update = 0;
		x->verbose = 0;
		x->bypass = 0;
		x->mute = 0;
		x->always_update = 0;
		x->automate = 0;
		x->warpfac1 = 1.0;
		x->warpfac2 = 1.0;
		x->funcoff = 0;
		x->cf1 = 500.;
		x->cf2 = 3000.;
		x->bw1 = 0.2;
		x->bw2 = 0.2;
		x->connections = (short *) calloc(16, sizeof(short));
	x->warpfunc = (t_float *) calloc(fft->N2, sizeof(t_float));
	fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
	update_warp_function(x);
}

void pvwarp_bottomfreq(t_pvwarp *x, t_floatarg f)
{
	
	if( f < 0 || f > x->fft->R / 2.0 ){
		error("%s: frequency %f out of range", OBJECT_NAME, f);
		return;
	}
	x->lofreq = f;
	fftease_oscbank_setbins(x->fft, x->lofreq, x->hifreq);	
}

void pvwarp_topfreq(t_pvwarp *x, t_floatarg f)
{
	if( f < x->lofreq || f > x->fft->R / 2.0 ){
		error("%s: frequency %f out of range", OBJECT_NAME, f);
		return;
	}
	x->hifreq = f;
	fftease_oscbank_setbins(x->fft, x->lofreq, x->hifreq);		
}

void do_pvwarp(t_pvwarp *x)
{
	t_fftease *fft = x->fft;
	int lo_bin = fft->lo_bin;
	int hi_bin = fft->hi_bin;
	int chan, freq;
	int funcoff = x->funcoff;
	int N2 = fft->N2;
	t_float *channel = fft->channel;
	t_float *warpfunc = x->warpfunc;
	
	fftease_fold(fft);
	fftease_rdft(fft,FFT_FORWARD);
	fftease_convert(fft);

	for ( chan = lo_bin; chan < hi_bin; chan++ ) {
		freq = (chan << 1) + 1;
		channel[freq] *= warpfunc[(chan + funcoff) % N2];
	}
	fftease_oscbank(fft);
}

t_int *pvwarp_perform(t_int *w)
{
	int 	i,j;
	
	t_float f;
	t_pvwarp *x = (t_pvwarp *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *in1 = (t_float *)(w[3]);
	t_float *in2 = (t_float *)(w[4]);
	t_float *in3 = (t_float *)(w[5]);
	t_float *in4 = (t_float *)(w[6]);
	t_float *in5 = (t_float *)(w[7]);
	t_float *in6 = (t_float *)(w[8]);
	t_float *in7 = (t_float *)(w[9]);
	t_float *in8 = (t_float *)(w[10]);
	t_float *in9 = (t_float *)(w[11]);
	t_float *MSPOutputVector = (t_float *)(w[12]);
	
	t_fftease *fft = x->fft;
	int D = fft->D;
	int Nw = fft->Nw;
	int N2 = fft->N2;
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
		return w+13;
	}

	if(!x->automate) {
        x->cf1  = *in1;
        x->bw1 = *in2 ;
        x->warpfac1 = *in3;
        x->cf2  = *in4;
        x->bw2 = *in5;
        x->warpfac2 = *in6;
	}
	

    f  = *in7 ;
    if( f < 0 ) {
        f = 0.0;
    } else if (f > 1.0 ){
        f = 1.0;
    }
    x->funcoff = (int) (f * (float) (N2 - 1));
    fft->P = *in8 ;
    fft->synt = *in9 ;

	if( (x->please_update || x->always_update)  && ! x->automate){
		update_warp_function(x);
	}

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_pvwarp(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_pvwarp(x);
			
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
            
			do_pvwarp(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+13;
}		


int freq_to_bin( float target, float fundamental ){
	float lastf = 0.0;
	float testf = 0.0;
	int thebin = 0;
	while( testf < target ){
		++thebin;
		lastf = testf;
		testf += fundamental;
	}
	
	if(fabs(target - testf) < fabs(target - lastf) ){
		return thebin;
	} else {
		return (thebin - 1);
	}
}

void pvwarp_dsp(t_pvwarp *x, t_signal **sp)
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
        pvwarp_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(pvwarp_perform, 12, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
                sp[4]->s_vec, sp[5]->s_vec, sp[6]->s_vec, sp[7]->s_vec,
                sp[8]->s_vec, sp[9]->s_vec, sp[10]->s_vec);
    }
}
