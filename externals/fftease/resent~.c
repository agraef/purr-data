/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *resent_class;

#define OBJECT_NAME "resent~"


typedef struct _resent
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	t_float *frame_incr;
	t_float *store_incr;
	t_float *frame_phase;
	t_float frameloc;
	t_float **loveboat;
	t_float current_frame;
	long framecount;
    long last_framecount;
	t_float frame_increment ;
	t_float fpos;
	t_float last_fpos;
	t_float tadv;
	int read_me;
	long frames_read;
	short mute;
	void *m_clock;
	void *m_bang;
	short playthrough;
	short lock;
	t_float duration;
	t_float sync;
    long interpolation_attr;
} t_resent;

void resent_dsp(t_resent *x, t_signal **sp);
t_int *resent_perform(t_int *w);
void *resent_new(t_symbol *msg, short argc, t_atom *argv);
void resent_assist(t_resent *x, void *b, long m, long a, char *s);
void resent_acquire_sample (t_resent *x) ;
void resent_mute(t_resent *x, t_floatarg tog);
void resent_bin(t_resent *x, t_floatarg fbin, t_floatarg speed);
void resent_setphase(t_resent *x, t_floatarg phase);
void resent_addphase(t_resent *x, t_floatarg phase);
void resent_setspeed( t_resent *x,  t_floatarg speed );
void resent_addspeed( t_resent *x,  t_floatarg speed );
void resent_size( t_resent *x,  t_floatarg size_ms );
void resent_free( t_resent *x );
void resent_store_incr( t_resent *x );
void resent_setspeed_and_phase( t_resent *x,  t_floatarg speed, t_floatarg phase );
void resent_tick(t_resent *x);
void resent_fftinfo(t_resent *x);
void resent_init(t_resent *x);
void resent_linephase(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_linespeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_randphase(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_randspeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_playthrough(t_resent *x, t_floatarg state);
void resent_interpolation(t_resent *x,  t_floatarg tog);
t_float fftease_randf(t_float min, t_float max);
void resent_transpose(t_resent *x, t_floatarg tf);
void resent_synthresh(t_resent *x, t_floatarg thresh);
void resent_oscbank(t_resent *x, t_floatarg flag);

void resent_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("resent~"), (t_newmethod)resent_new,
                  (t_method)resent_free,sizeof(t_resent), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_resent, x_f);
	class_addmethod(c,(t_method)resent_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)resent_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(c,(t_method)resent_oscbank,gensym("oscbank"),A_FLOAT,0);
	class_addmethod(c,(t_method)resent_transpose,gensym("transpose"),A_FLOAT,0);
	class_addmethod(c,(t_method)resent_synthresh,gensym("synthresh"),A_FLOAT,0);
	class_addmethod(c,(t_method)resent_acquire_sample,gensym("acquire_sample"), 0);
	class_addmethod(c,(t_method)resent_linespeed, gensym("linespeed"), A_GIMME, 0);
	class_addmethod(c,(t_method)resent_linephase, gensym("linephase"), A_GIMME, 0);
	class_addmethod(c,(t_method)resent_randspeed, gensym("randspeed"), A_GIMME, 0);
	class_addmethod(c,(t_method)resent_randphase, gensym("randphase"), A_GIMME, 0);
	class_addmethod(c,(t_method)resent_bin, gensym("bin"), A_FLOAT, A_FLOAT, 0);
	class_addmethod(c,(t_method)resent_setphase, gensym("setphase"),  A_FLOAT, 0);
	class_addmethod(c,(t_method)resent_addphase, gensym("addphase"),  A_FLOAT, 0);
	class_addmethod(c,(t_method)resent_setspeed, gensym("setspeed"),  A_FLOAT, 0);
	class_addmethod(c,(t_method)resent_addspeed, gensym("addspeed"),  A_FLOAT, 0);
	class_addmethod(c,(t_method)resent_playthrough, gensym("playthrough"),  A_DEFFLOAT, 0);
	class_addmethod(c,(t_method)resent_store_incr, gensym("store_incr"),0);
	class_addmethod(c,(t_method)resent_setspeed_and_phase, gensym("setspeed_and_phase"),  A_FLOAT, A_FLOAT, 0);
    class_addmethod(c,(t_method)resent_interpolation, gensym("interpolation"), A_FLOAT, 0);
    resent_class = c;
    fftease_announce(OBJECT_NAME);
}

void resent_transpose(t_resent *x, t_floatarg tf)
{
	x->fft->P = tf;
}

void resent_synthresh(t_resent *x, t_floatarg thresh)
{
	x->fft->synt = thresh;
}

void resent_oscbank(t_resent *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void resent_store_incr(t_resent *x)
{
	t_fftease *fft = x->fft;

	int i;
	t_float *store_incr = x->store_incr;
	t_float *frame_incr = x->frame_incr;
	
	for(i = 0; i < fft->N2; i++){
		store_incr[i] = frame_incr[i];
	}
}

void resent_free(t_resent *x){
	int i ;

    if(x->fft->initialized){
        for(i = 0; i < x->framecount; i++){
            free(x->loveboat[i]) ;
        }
        free(x->loveboat);
        free(x->frame_phase);
        free(x->frame_incr);
        free(x->store_incr);
    }
	fftease_free(x->fft);
    free(x->fft);
}

void resent_bin(t_resent *x, t_floatarg fbin, t_floatarg speed)
{
	t_fftease *fft = x->fft;

	int bin_num = (int) fbin;
	
	if(bin_num >= 0 && bin_num < fft->N2){
		x->frame_incr[bin_num] = speed ;
	} else {
		post("resent~: bin %d is out of range", bin_num);
	}
}

void resent_setphase( t_resent *x,  t_floatarg phase)
{
	t_fftease *fft = x->fft;

	t_float scaled_phase;
	int i;
	
	if( phase < 0. )
		phase = 0. ;
	if( phase > 1. )
		phase = 1.;
	scaled_phase = phase * (float) x->framecount ;
	for( i = 0; i < fft->N2; i++ ){
		x->frame_phase[i] = scaled_phase ;
	}
	
}

void resent_addphase( t_resent *x,  t_floatarg phase )
{
	t_fftease *fft = x->fft;

	t_float scaled_phase ;
	t_float *frame_phase = x->frame_phase;
	int framecount =  x->framecount;
	int i;
	
	
	if( phase < 0. )
		phase = 0. ;
	if( phase > 1. )
		phase = 1.;
	scaled_phase = phase * (float) framecount ;
	for( i = 0; i < fft->N2; i++ ){
		frame_phase[i] += scaled_phase ;
		while( frame_phase[i] < 0 )
			frame_phase[i] += framecount;
		while( frame_phase[i] > framecount - 1 )
			frame_phase[i] -= framecount ;
	}
	
	
}

void resent_setspeed( t_resent *x,  t_floatarg speed )
{
	t_fftease *fft = x->fft;
	if(! x->fft->init_status)
		return;
		
	int i;
	
	for( i = 0; i < fft->N2; i++ ){
		
		x->frame_incr[i] = speed ;
	}
	// post("speed reset to %f",speed);
}

void resent_addspeed( t_resent *x,  t_floatarg speed )
{
	t_fftease *fft = x->fft;

	int i;
	t_float *store_incr = x->store_incr;
	t_float *frame_incr = x->frame_incr;
	
	for( i = 0; i < fft->N2; i++ ){	
		frame_incr[i] = store_incr[i] + speed ;
	}
}

void resent_interpolation( t_resent *x,  t_floatarg tog )
{
    x->interpolation_attr = (int) tog;
}

void resent_setspeed_and_phase( t_resent *x,  t_floatarg speed, t_floatarg phase )
{
	t_fftease *fft = x->fft;

	t_float scaled_phase;
	int i;
	if( phase < 0. )
		phase = 0. ;
	if( phase > 1. )
		phase = 1.;
	
	scaled_phase = phase * (t_float) x->framecount ;
	for( i = 0; i < fft->N2; i++ ){
		x->frame_phase[i] = scaled_phase ;
		x->frame_incr[i] = speed ;
	}
	//  post("ssap: speed reset to %f, phase reset to %f",speed,phase);
	
}

void resent_assist (t_resent *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal/bang) Input, Sample Trigger");
				break;
		}
	} else if (msg==2) {
		switch( arg){
			case 0: 
				sprintf(dst,"(signal) Output ");
				break;
			case 1:
				sprintf(dst,"(signal) Recording Sync");
				break;
		}
		
	}
}

void resent_tick(t_resent *x) {
	outlet_bang(x->m_bang);
}

void resent_init(t_resent *x)
{
	int i;
	short initialized = x->fft->initialized;
	t_fftease  *fft = x->fft;
	fftease_init(fft);
	if(!fftease_msp_sanity_check(fft,OBJECT_NAME)){
		return;
	}
		
	x->current_frame = x->framecount = 0;
	x->fpos = x->last_fpos = 0;
	x->tadv = (float)fft->D/(float)fft->R;
	if(x->duration < 0.1){
		x->duration = 0.1;
	}
	x->framecount =  x->duration/x->tadv ;
	x->read_me = 0;
	
	if(! initialized ){
		x->frame_increment = 1.0 ;  
		x->mute = 0;
		x->playthrough = 0;
		x->sync = 0;
		x->frames_read = 0;
		x->frame_incr = (t_float *) calloc(fft->N2, sizeof(t_float));
		x->store_incr = (t_float *) calloc(fft->N2, sizeof(t_float));
		x->frame_phase = (t_float *) calloc(fft->N2, sizeof(t_float));
		x->loveboat = (t_float **) calloc(x->framecount, sizeof(t_float *));
		for(i=0; i < x->framecount; i++){
			x->loveboat[i] = (t_float *) calloc((fft->N+2), sizeof(t_float));
			if(x->loveboat[i] == NULL){
				error("%s: Insufficient Memory!",OBJECT_NAME);
				return;
			}
		}
	} 
	else { /* this could fail or might not actually release memory - test it!! */
		x->frame_incr = (t_float *) realloc(x->frame_incr, fft->N2 * sizeof(t_float));
		x->store_incr = (t_float *) realloc(x->store_incr, fft->N2 * sizeof(t_float));
		x->frame_phase = (t_float *) realloc(x->frame_phase, fft->N2 * sizeof(t_float));

		for(i = 0; i < x->last_framecount; i++){
			free(x->loveboat[i]) ;
		}
        x->loveboat = (t_float **)realloc(x->loveboat, x->framecount * sizeof(t_float*));
		for(i=0; i < x->framecount; i++){
			x->loveboat[i] = (t_float *) calloc((fft->N+2), sizeof(t_float));
			if(x->loveboat[i] == NULL){
				error("%s: Insufficient Memory!",OBJECT_NAME);
				return;
			}
		}
	}
    x->last_framecount = x->framecount;
}

void *resent_new(t_symbol *msg, short argc, t_atom *argv)
{
t_fftease *fft;
	t_resent *x = (t_resent *)pd_new(resent_class);

    
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
	x->fft = (t_fftease *) calloc(1, sizeof(t_fftease) );
	fft = x->fft;
	fft->initialized = 0;
		
	srand(clock()); // needed ?
	x->interpolation_attr = 0;

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ x->duration = atom_getfloatarg(0, argc, argv) / 1000.0; }
    else { post("%s: must give duration argument",OBJECT_NAME); return NULL; }
    if(argc > 1){ fft->N = (int) atom_getfloatarg(1, argc, argv); }
    if(argc > 2){ fft->overlap = (int) atom_getfloatarg(2, argc, argv); }
	return x;
}

void do_resent(t_resent *x)
{
	t_fftease *fft = x->fft;
	
	int iphase, amp, freq, i;
	int N = fft->N;
	int N2 = fft->N2;
	t_float fframe = x->current_frame ;
	t_float last_fpos = x->last_fpos ;
	int framecount = x->framecount;
	t_float *frame_incr = x->frame_incr;
	t_float *frame_phase = x->frame_phase;
	t_float *channel = fft->channel;
    t_float frak;
    long iphase1, iphase2;
    
	if(x->read_me && x->framecount > 0){
		
		fftease_fold(fft);
		fftease_rdft(fft,FFT_FORWARD);
		fftease_convert(fft);
		// use memcopy
		for(i = 0; i < N; i++){
			x->loveboat[x->frames_read][i] = channel[i];
		}
		x->frames_read++;		
		if(x->frames_read >= x->framecount){
			x->read_me = 0;
			// post("sample acquisition completed");
		} 	
		x->sync = (t_float) x->frames_read / (t_float) x->framecount;
	} 
	else {
        if(x->interpolation_attr == 1){
            for( i = 0 ; i < N2; i++ ){
                amp = i<<1;
                freq = amp + 1;
                iphase1 = floor( frame_phase[i] );
                frak = frame_phase[i] - iphase1;
                if( iphase1 < 0 )
                    iphase1 = 0;
                if( iphase1 > framecount - 1 )
                    iphase1 = framecount - 1;
                iphase2 = (iphase1 + 1) % framecount;
                channel[amp] = x->loveboat[iphase1][amp] + (frak *
                    (x->loveboat[iphase2][amp] - x->loveboat[iphase1][amp]));
                channel[freq] = x->loveboat[iphase1][freq] + (frak *
                    (x->loveboat[iphase2][freq] - x->loveboat[iphase1][freq]));
                frame_phase[i] += frame_incr[i] ;
                while( frame_phase[i] > framecount - 1)
                    frame_phase[i] -= framecount;
                while( frame_phase[i] < 0. )
                    frame_phase[i] += framecount;
            }
        }
        else {
            for( i = 0 ; i < N2; i++ ){
                amp = i<<1;
                freq = amp + 1 ;
                iphase = frame_phase[i];
                if( iphase < 0 )
                    iphase = 0;
                if( iphase > framecount - 1 )
                    iphase = framecount - 1;
                channel[amp] = x->loveboat[iphase][amp];
                channel[freq] = x->loveboat[iphase][freq];
                frame_phase[i] += frame_incr[i] ;
                while( frame_phase[i] > framecount - 1)
                    frame_phase[i] -= framecount;
                while( frame_phase[i] < 0. )
                    frame_phase[i] += framecount;
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
	x->last_fpos = last_fpos;

}

t_int *resent_perform(t_int *w)
{
	int i, j;
	//////////////////////////////////////////////
    t_resent *x = (t_resent *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *MSPOutputVector = (t_float *)(w[3]);
	t_float *sync_vec = (t_float *)(w[4]);

	/* dereference structure */	
	
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
        for(i=0; i < MSPVectorSize; i++){ sync_vec[i] = 0.0; }
		return w+5;
	}
	if( fft->obank_flag )
		mult *= FFTEASE_OSCBANK_SCALAR;
	if(x->playthrough && x->read_me){
		for (i = 0; i < MSPVectorSize; i++) {
			MSPOutputVector[i] = MSPInputVector[i] * 0.5; // scale down
		}
        for(i=0; i < MSPVectorSize; i++){ sync_vec[i] = 0.0; }
        return w+5;
	}

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_resent(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_resent(x);
			
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
            
			do_resent(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
	for ( i = 0; i < MSPVectorSize; i++ ){
		sync_vec[i] = x->sync;
	}
	return w+5;
}

void resent_acquire_sample(t_resent *x)
{
	x->read_me = 1;
	x->frames_read = 0;
	return;
}

void resent_mute(t_resent *x, t_floatarg tog)
{
	x->mute = tog;	
}

void resent_playthrough(t_resent *x, t_floatarg state)
{
	x->playthrough = state;
}

void resent_linephase(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft = x->fft;
	int bin1, bin2;
	float phase1, phase2, bindiff;
	int i;
	float m1, m2;
	
	bin1 = (int) atom_getfloatarg(0, argc, argv);
	phase1 = atom_getfloatarg(1, argc, argv) * x->framecount;
	bin2 = (int) atom_getfloatarg(2, argc, argv);
	phase2 = atom_getfloatarg(3, argc, argv) * x->framecount;
	
	if( bin1 > fft->N2 || bin2 > fft->N2 ){
		error("too high bin number");
		return;
	}
	bindiff = bin2 - bin1;
	if( bindiff < 1 ){
		error("make bin2 higher than bin 1, bye now");
		return;
	}
	for( i = bin1; i < bin2; i++ ){
		m2 = (float) i / bindiff;
		m1 = 1. - m2;
		x->frame_phase[i] = m1 * phase1 + m2 * phase2;
	}
}

void resent_randphase(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft = x->fft;
	
	float minphase, maxphase;
	int i;
	int framecount = x->framecount;
	
	minphase = atom_getfloatarg(0, argc, argv);
	maxphase = atom_getfloatarg(1, argc, argv);
	
	//  post("minphase %f maxphase %f",minphase, maxphase);
	if(minphase < 0.0)
		minphase = 0.0;
	if( maxphase > 1.0 )
		maxphase = 1.0;
  	
	for( i = 0; i < fft->N2; i++ ){
		x->frame_phase[i] = (int) (fftease_randf( minphase, maxphase ) * (float) (framecount - 1) ) ;	
	} 
}

void resent_randspeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft = x->fft;
	
	float minspeed, maxspeed;
	int i;
	
	
	minspeed = atom_getfloatarg(0, argc, argv);
	maxspeed = atom_getfloatarg(1, argc, argv);
	
	for( i = 0; i < fft->N2; i++ ){
		x->frame_incr[i] = fftease_randf(minspeed, maxspeed);
	} 
}

void resent_linespeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft = x->fft;
	int bin1, bin2;
	float speed1, speed2, bindiff;
	int i;
	float m1, m2;
	
	bin1 = (int) atom_getfloatarg(0, argc, argv);
	speed1 = atom_getfloatarg(1, argc, argv);
	bin2 = (int) atom_getfloatarg(2, argc, argv);
	speed2 = atom_getfloatarg(3, argc, argv);
	
	if( bin1 > fft->N2 || bin2 > fft->N2 ){
		error("too high bin number");
		return;
	}
	bindiff = bin2 - bin1;
	if( bindiff < 1 ){
		error("make bin2 higher than bin 1, bye now");
		return;
	}
	for( i = bin1; i < bin2; i++ ){
		m2 = (float) i / bindiff;
		m1 = 1. - m2;
		x->frame_incr[i] = m1 * speed1 + m2 * speed2;
	}
}

void resent_dsp(t_resent *x, t_signal **sp)
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
        resent_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(resent_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
    }
}
