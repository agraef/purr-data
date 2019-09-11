/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *pvwarpb_class;

#define OBJECT_NAME "pvwarpb~"

typedef struct _pvwarpb
{
    t_object x_obj;
    t_float x_f;
	t_symbol *buffername;
	t_float lofreq;/* user speficied lowest synthfreq */
	t_float hifreq;/* user specified highest synthfreq */
	t_float topfreq;
	t_fftease *fft;
	short mute;
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
    t_float *warpfunc; // workspace to create a new function
	short initialized; // state for object
    int b_frames;
    t_float *b_samples;
    int b_valid;
} t_pvwarpb;

void pvwarpb_dsp(t_pvwarpb *x, t_signal **sp);
t_int *pvwarpb_perform(t_int *w);
void *pvwarpb_new(t_symbol *s, int argc, t_atom *argv);
void pvwarpb_mute(t_pvwarpb *x, t_floatarg state);
void pvwarpb_automate(t_pvwarpb *x, t_floatarg state);
void pvwarpb_autofunc(t_pvwarpb *x, t_floatarg minval, t_floatarg maxval);
void pvwarpb_free( t_pvwarpb *x );
t_float fftease_randf( t_float min, t_float max );
t_float closestf(t_float test, t_float *arr) ;
int freq_to_bin(t_float target, t_float fundamental);
void update_warp_function( t_pvwarpb *x ) ;
void pvwarpb_init(t_pvwarpb *x);
void pvwarpb_bottomfreq(t_pvwarpb *x, t_floatarg f);
void pvwarpb_topfreq(t_pvwarpb *x, t_floatarg f);
void pvwarpb_attachbuf(t_pvwarpb *x);
void pvwarpb_setbuf(t_pvwarpb *x, t_symbol *wavename);
void pvwarpb_redraw(t_pvwarpb *x);

void pvwarpb_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("pvwarpb~"), (t_newmethod)pvwarpb_new,
                  (t_method)pvwarpb_free,sizeof(t_pvwarpb), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_pvwarpb, x_f);
	class_addmethod(c,(t_method)pvwarpb_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)pvwarpb_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)pvwarpb_bottomfreq,gensym("bottomfreq"),A_FLOAT,0);
	class_addmethod(c,(t_method)pvwarpb_topfreq,gensym("topfreq"),A_FLOAT,0);
	class_addmethod(c,(t_method)pvwarpb_autofunc,gensym("autofunc"),A_DEFFLOAT, A_DEFFLOAT,0);
    pvwarpb_class = c;
    fftease_announce(OBJECT_NAME);
}

void pvwarpb_automate(t_pvwarpb *x, t_floatarg state)
{
	x->automate = state;
}

void update_warp_function( t_pvwarpb *x )
{
	int i,j;
	int N2 = x->fft->N2;
	t_float warpfac1 = x->warpfac1;
	t_float warpfac2 = x->warpfac2;
    long b_frames;
    t_float *warpfunc = x->warpfunc;
    float *b_samples;
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
    
    pvwarpb_attachbuf(x);

    b_frames = x->b_frames;
    if(b_frames < N2){
        post("%s: table too small",OBJECT_NAME);
        return;
    }
    
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
/*	if( verbose )
		post("bump1: hi %d mid %d lo %d",hibin,midbin,lobin); */
	
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
	/* if( verbose )
		post("bump2: hi %d mid %d lo %d",hibin,midbin,lobin); */
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
    // buffer stuffer
	b_samples = x->b_samples;
    for(i = 0; i < N2; i++){
        b_samples[i] = warpfunc[i];
    }
	x->please_update = 0;
    pvwarpb_redraw(x);
}

void pvwarpb_redraw(t_pvwarpb *x)
{
    t_garray *a;
    if (!(a = (t_garray *)pd_findbyclass(x->buffername, garray_class))) {
        if (*x->buffername->s_name) pd_error(x, "function~: %s: no such array", x->buffername->s_name);
    }
    else  {
        garray_redraw(a);
    }
}

void pvwarpb_verbose(t_pvwarpb *x, t_floatarg state)
{
	x->verbose = state;	
}

void pvwarpb_autofunc(t_pvwarpb *x, t_floatarg minval, t_floatarg maxval)
{
	int minpoints, maxpoints, segpoints, i;
	int pointcount = 0;
	t_float target, lastval;
	t_float m1, m2;
	int N2 = x->fft->N2;
    long b_frames;
    t_float *warpfunc = x->warpfunc;
    float *b_samples;
    
    pvwarpb_attachbuf(x);

    b_frames = x->b_frames;
    if(b_frames < N2){
        post("%s: table too small or not mono",OBJECT_NAME);
        return;
    }

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
    // buffer stuffer
	b_samples = x->b_samples;
    for(i = 0; i < N2; i++){
        b_samples[i] = warpfunc[i];
    }
    pvwarpb_redraw(x);
}

void pvwarpb_mute(t_pvwarpb *x, t_floatarg state)
{
	x->mute = state;	
}

void pvwarpb_free( t_pvwarpb *x ){
    if(x->fft->initialized){
        free(x->warpfunc);
    }
    fftease_free(x->fft);
    free(x->fft);
}

void *pvwarpb_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_pvwarpb *x = (t_pvwarpb *)pd_new(pvwarpb_class);
    int i;
    for(i=0;i<3;i++){
        inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    }
	outlet_new(&x->x_obj, gensym("signal"));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0; // for FFTease package
	x->initialized = 0; // for object
	x->lofreq = 0.0;
	x->hifreq = 10000.0;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    x->warpfunc = (t_float *) calloc(8192, sizeof(t_float));
    if(argc > 0){ x->buffername = atom_getsymbolarg(0, argc, argv); }
    else { post("%s: Must specify array name", OBJECT_NAME); return NULL; }
    if(argc > 1){ fft->N = (int) atom_getfloatarg(1, argc, argv); }
    if(argc > 2){ fft->overlap = (int) atom_getfloatarg(2, argc, argv); }
	return x;
}

void pvwarpb_init(t_pvwarpb *x)
{
	t_fftease  *fft = x->fft;

	fftease_init(fft);
	
	if(!x->initialized){
		srand(clock());
		x->please_update = 0;
		x->verbose = 0;
		x->mute = 0;
		x->topfreq = 3000. ;
		x->always_update = 0;
		x->automate = 0;
		x->warpfac1 = 1.0;
		x->warpfac2 = 1.0;
		x->funcoff = 0;
		x->cf1 = 500.;
		x->cf2 = 3000.;
		x->bw1 = 0.2;
		x->bw2 = 0.2;
		x->initialized = 1;
	}
    if(fft->N2 > 8192){
        x->warpfunc = (t_float *) realloc(x->warpfunc, fft->N2);
    }
	fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
}

void pvwarpb_bottomfreq(t_pvwarpb *x, t_floatarg f)
{
	if( f < 0 || f > x->fft->R / 2.0 ){
		error("%s: frequency %f out of range", OBJECT_NAME, f);
		return;
	}
	x->lofreq = f;
	fftease_oscbank_setbins(x->fft, x->lofreq, x->hifreq);	
}

void pvwarpb_topfreq(t_pvwarpb *x, t_floatarg f)
{
	if( f < x->lofreq || f > x->fft->R / 2.0 ){
		error("%s: frequency %f out of range", OBJECT_NAME, f);
		return;
	}
	x->hifreq = f;
	fftease_oscbank_setbins(x->fft, x->lofreq, x->hifreq);	
}

void do_pvwarpb(t_pvwarpb *x)
{
	t_fftease *fft = x->fft;
	int lo_bin = fft->lo_bin;
	int hi_bin = fft->hi_bin;
	int chan, freq;
	int funcoff = x->funcoff;
	int N2 = fft->N2;
	t_float *channel = fft->channel;
    long b_frames;
    float *b_samples;

	fftease_fold(fft);
	fftease_rdft(fft,FFT_FORWARD);
	fftease_convert(fft);

    b_samples = x->b_samples;
    if(! b_samples){
        goto panic2;
    }

    b_frames = x->b_frames;
    if(b_frames < N2){
        post("%s: table too small",OBJECT_NAME);
        goto panic1;
    }

	for ( chan = lo_bin; chan < hi_bin; chan++ ) {
		freq = (chan << 1) + 1;
		channel[freq] *= b_samples[(chan + funcoff) % N2];
	}
panic1: ;
panic2: ;
	fftease_oscbank(fft);
}

t_int *pvwarpb_perform(t_int *w)
{
	int 	i,j;
	
	t_float f;
	t_pvwarpb *x = (t_pvwarpb *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *in7 = (t_float *)(w[3]);
	t_float *in8 = (t_float *)(w[4]);
	t_float *in9 = (t_float *)(w[5]);
	t_float *MSPOutputVector = (t_float *)(w[6]);
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

    pvwarpb_attachbuf(x);
	if(x->mute || !x->b_valid || x->b_frames < N2){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+7;
	}

    f  = *in7 ;
    if( f < 0 ) {
        f = 0.0;
    } else if (f > 1.0 ){
        f = 1.0;
    }
    x->funcoff = (int) (f * (t_float) (N2 - 1));
    fft->P = *in8 ;
    fft->synt = *in9;

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_pvwarpb(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_pvwarpb(x);
			
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
            
			do_pvwarpb(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+7;
}		


int freq_to_bin( t_float target, t_float fundamental ){
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

void pvwarpb_attachbuf(t_pvwarpb *x)
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

void pvwarpb_setbuf(t_pvwarpb *x, t_symbol *wavename)
{
    x->buffername = wavename;
}

void pvwarpb_dsp(t_pvwarpb *x, t_signal **sp)
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
        pvwarpb_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(pvwarpb_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
                sp[4]->s_vec);
    }
}