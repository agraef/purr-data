/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *pvtuner_class;

#define OBJECT_NAME "pvtuner~"

#define MAXTONES (8192)
#define BASE_FREQ (27.5)	/* low A  */
#define DIATONIC 0
#define EASTERN 1
#define MINOR 2
#define EQ12 3
#define PENTATONIC 4
#define MAJOR_ADDED_SIXTH 5
#define MINOR_ADDED_SIXTH 6
#define ADDED_SIXTH_MAJOR 5
#define ADDED_SIXTH_MINOR 6
#define MAJOR_SEVENTH_CHORD 7
#define MINOR_SEVENTH_CHORD 8
#define DOMINANT_SEVENTH_CHORD 9
#define EQ8 10
#define PENTACLUST 11
#define QUARTERCLUST 12
#define EQ5 13
#define SLENDRO 14
#define PELOG 15
#define IMPORTED_SCALE 16
#define EQN 17

typedef struct {
	t_float *pitchgrid;
	int scale_steps; // total number of members
	short current_scale;
} t_pvtuner_scale;

typedef struct _pvtuner
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	int lo_bin;
	int hi_bin;
	int hi_tune_bin;
	t_float topfreq;
	t_float curfreq;
	// TUNING
	float *pitchgrid;
	float pbase;
	int scale_steps;
	short current_scale;
	short mute;
	t_float lofreq;
	t_float hifreq;
	t_float tabscale;
	int scale_len;
	short verbose;
	long fftsize_attr;
	long overlap_attr;
	long scale_interpolation; // interpolation flag, set to zero by default
	t_float interpolation_dur; // set to 1 second
	long interpolation_frames; // == duration / (D/R)
	long interpolation_countdown; // count frames for interpolation
	t_float ip; // interpolation point
	t_pvtuner_scale *this_scale;
	t_pvtuner_scale *last_scale;
} t_pvtuner;


void pvtuner_dsp(t_pvtuner *x, t_signal **sp);
t_int *pvtuner_perform(t_int *w);
t_float closestf(t_float test, t_float *arr) ;
void pvtuner_diatonic( t_pvtuner *x );
void pvtuner_eastern( t_pvtuner *x );
void pvtuner_minor( t_pvtuner *x );
void pvtuner_eq12( t_pvtuner *x );
void pvtuner_pentatonic( t_pvtuner *x );
void pvtuner_major_added_sixth( t_pvtuner *x );
void pvtuner_minor_added_sixth( t_pvtuner *x );
void pvtuner_major_seventh_chord( t_pvtuner *x );
void pvtuner_minor_seventh_chord( t_pvtuner *x );
void pvtuner_dominant_seventh_chord( t_pvtuner *x );
void pvtuner_eq8( t_pvtuner *x );
void pvtuner_pentaclust( t_pvtuner *x );
void pvtuner_quarterclust( t_pvtuner *x );
void pvtuner_eq5( t_pvtuner *x );
void pvtuner_slendro( t_pvtuner *x );
void pvtuner_pelog( t_pvtuner *x );
void pvtuner_update_imported( t_pvtuner *x );
void pvtuner_init(t_pvtuner *x);
void *pvtuner_new(t_symbol *s, int argc, t_atom *argv);
void pvtuner_import_scale(t_pvtuner *x, t_symbol *filename);
void pvtuner_list (t_pvtuner *x, t_symbol *msg, short argc, t_atom *argv);
void pvtuner_toptune( t_pvtuner *x, t_floatarg f );
void pvtuner_frequency_range(t_pvtuner *x, t_floatarg lo, t_floatarg hi);
void pvtuner_basefreq( t_pvtuner *x, t_floatarg bassfreq);
void pvtuner_free(t_pvtuner *x);
void pvtuner_mute(t_pvtuner *x, t_floatarg state);
void pvtuner_list (t_pvtuner *x, t_symbol *msg, short argc, t_atom *argv);
void pvtuner_binfo(t_pvtuner *x);
void pvtuner_eqn(t_pvtuner *x, t_floatarg steps);
void pvtuner_interpolation(t_pvtuner *x, t_floatarg state);

void pvtuner_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("pvtuner~"), (t_newmethod)pvtuner_new,
                  (t_method)pvtuner_free,sizeof(t_pvtuner), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_pvtuner, x_f);
	class_addmethod(c,(t_method)pvtuner_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)pvtuner_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)pvtuner_basefreq,gensym("basefreq"),A_DEFFLOAT,0);
	class_addmethod(c,(t_method)pvtuner_diatonic,gensym("diatonic"),0);
	class_addmethod(c,(t_method)pvtuner_eastern,gensym("eastern"),0);
	class_addmethod(c,(t_method)pvtuner_minor,gensym("minor"),0);
	class_addmethod(c,(t_method)pvtuner_eq12,gensym("eq12"),0);
	class_addmethod(c,(t_method)pvtuner_pentatonic,gensym("pentatonic"),0);
	class_addmethod(c,(t_method)pvtuner_major_added_sixth,gensym("added_sixth_major"),0);
	class_addmethod(c,(t_method)pvtuner_minor_added_sixth,gensym("added_sixth_minor"),0);
	class_addmethod(c,(t_method)pvtuner_major_seventh_chord,gensym("major_seventh_chord"),0);
	class_addmethod(c,(t_method)pvtuner_minor_seventh_chord,gensym("minor_seventh_chord"),0);
	class_addmethod(c,(t_method)pvtuner_dominant_seventh_chord,gensym("dominant_seventh_chord"),0);
	class_addmethod(c,(t_method)pvtuner_eq8,gensym("eq8"),0);
	class_addmethod(c,(t_method)pvtuner_pentaclust,gensym("pentaclust"),0);
	class_addmethod(c,(t_method)pvtuner_quarterclust,gensym("quarterclust"),0);
	class_addmethod(c,(t_method)pvtuner_eq5,gensym("eq5"),0);
	class_addmethod(c,(t_method)pvtuner_eqn,gensym("eqn"),A_FLOAT, 0);
    class_addmethod(c,(t_method)pvtuner_interpolation,gensym("interpolation"),A_FLOAT, 0);
	class_addmethod(c,(t_method)pvtuner_slendro,gensym("slendro"),0);
	class_addmethod(c,(t_method)pvtuner_pelog,gensym("pelog"),0);
	class_addmethod(c,(t_method)pvtuner_list,gensym("list"),A_GIMME,0);
	class_addmethod(c,(t_method)pvtuner_frequency_range,gensym("frequency_range"),A_FLOAT,A_FLOAT, 0);
    pvtuner_class = c;
    fftease_announce(OBJECT_NAME);
}

void *pvtuner_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fftease *fft;
	t_pvtuner *x = (t_pvtuner *)pd_new(pvtuner_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	x->verbose = 0;
	x->lofreq = 0;
	x->hifreq = 18000;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;	
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}
/* Copies current scale (this_scale) to next scale (next_scale)
 */
void pvtuner_copy_scale(t_pvtuner *x)
{
	int i;
	t_pvtuner_scale *this = x->this_scale;
	t_pvtuner_scale *last = x->last_scale;
	last->scale_steps = this->scale_steps;
	last->current_scale = this->current_scale;
	for(i = 0; i < last->scale_steps; i++){
		last->pitchgrid[i] = this->pitchgrid[i];
	}
}

void pvtuner_init(t_pvtuner *x)
{
	int mem;
	
	t_fftease *fft = x->fft;
	if( fft->R <= 0.0 ){
		post("%s: zero sample rate reported - initialization deferred", OBJECT_NAME);
		x->fft->initialized = 0; // failed initialization
		return;
	}

	if(!x->fft->initialized){
		x->mute = 0;
		mem = (MAXTONES+1)*sizeof(float);
		x->pitchgrid = (float *) calloc(mem,1);
		x->pbase = BASE_FREQ;
		x->this_scale = (t_pvtuner_scale *) calloc(1,sizeof(t_pvtuner_scale));
		x->last_scale = (t_pvtuner_scale *) calloc(1,sizeof(t_pvtuner_scale));
		x->this_scale->pitchgrid = (t_float *) calloc(1,mem);
		x->last_scale->pitchgrid = (t_float *) calloc(1,mem);
		x->this_scale->scale_steps = 0;
		x->last_scale->scale_steps = 0;
		pvtuner_diatonic(x);// default scale (rebuilt for new scale structure)
		pvtuner_copy_scale(x); // now both this and next scale are diatonic
	} 

	fftease_init(fft);
	if( x->fft->D && x->fft->R ){
		/*
		x->interpolation_frames = x->interpolation_dur / ((float)x->fft->D / (float)x->fft->R);
		post("interpolation frames: %d", x->interpolation_frames);
		*/
	} else {
		post("pvtuner~: dire warning!");
		return;
		// dire warning
	}
	fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
	x->hi_tune_bin = fft->hi_bin;
}

void pvtuner_toptune(t_pvtuner *x, t_floatarg f)
{
	int tbin;
	t_float curfreq;
	t_float fundamental = x->fft->c_fundamental;
	t_fftease *fft = x->fft;
	tbin = 1;  
	curfreq = 0;
	
	if( f < 0 || f > x->fft->R / 2.0 ){
		error("frequency %f out of range", f);
		return;
	}
	while( curfreq < f ) {
		++tbin;
		curfreq += fundamental ;
	}
	if( tbin > fft->lo_bin && tbin <= fft->hi_bin ){
		x->hi_tune_bin = tbin;
	} else {
		error("pvtuner~: bin %d out of range", tbin);
	}
	
}


void pvtuner_list (t_pvtuner *x, t_symbol *msg, short argc, t_atom *argv)
{
	t_float *pitchgrid = x->this_scale->pitchgrid;
    t_float dval;
	t_pvtuner_scale *s = x->this_scale;
	int i = 0;
	
	if( ! atom_getfloatarg(i,argc,argv) ){
		error("either zero length scale or 0.0 (prohibited) is first value");
		return;
	}
	pvtuner_copy_scale(x);
	/* first set every value to maximum */
	for(i=0; i < MAXTONES; i++){
		pitchgrid[i] = (t_float)x->fft->R / 2.0;
	}
    // now read scale
	for( i = 0; i < argc; i++ ){
        dval = atom_getfloatarg(i,argc,argv);
		pitchgrid[ i ] = dval;
	}
	s->scale_steps = argc;
	s->current_scale = IMPORTED_SCALE;
}

void pvtuner_binfo(t_pvtuner *x)
{
	t_fftease *fft = x->fft;
	post("%s: frequency targets: %f %f", OBJECT_NAME, x->lofreq, x->hifreq);
	post("synthesizing %d bins, from %d to %d",(fft->hi_bin - fft->lo_bin), fft->lo_bin, fft->hi_bin);
}

void pvtuner_frequency_range(t_pvtuner *x, t_floatarg lo, t_floatarg hi)
{
	t_fftease *fft = x->fft;
	x->lofreq = lo ;
	x->hifreq = hi;
	
	
	if( lo >= hi ){
		error("low frequency must be lower than high frequency");
		return;
	}
	x->curfreq = 0;
	fft->hi_bin = 0;
	
	while( x->curfreq < x->hifreq ) {
		++(fft->hi_bin);
		x->curfreq += x->fft->c_fundamental;
	}
	
	x->curfreq = 0;
	fft->lo_bin = 0;  
	while( x->curfreq < x->lofreq ) {
		++(fft->lo_bin);
		x->curfreq += x->fft->c_fundamental ;
	}
}

void do_pvtuner(t_pvtuner *x)
{
	t_fftease *fft = x->fft;
	int freq,chan;
	t_float *channel = fft->channel;
	t_float *this_pitchgrid = x->this_scale->pitchgrid;
	t_float *last_pitchgrid = x->last_scale->pitchgrid;
	int hi_bin = fft->hi_bin;
	int lo_bin = fft->lo_bin;
	int hi_tune_bin = x->hi_tune_bin;
	long scale_interpolation = x->scale_interpolation;
	t_float ip = x->ip;
	t_float freq_this, freq_last;
	fftease_fold(fft);
	fftease_rdft(fft, 1);
	fftease_convert(fft);
	// static case
	if( scale_interpolation == 0) {
		for ( chan = lo_bin; chan < hi_bin; chan++ ) {
			freq = (chan * 2) + 1;
			if(chan <= hi_tune_bin){
				channel[freq] = closestf(channel[freq], this_pitchgrid);
			}
		}
	}
	// interpolated case
	else if( scale_interpolation == 1) {
		// clip
		if( ip < 0 )
			ip = 0;
		if( ip > 1 )
			ip = 1;
		// degenerate cases first
		if( ip == 0 ){
			for ( chan = lo_bin; chan < hi_bin; chan++ ) {
				freq = (chan * 2) + 1;
				if(chan <= hi_tune_bin){
					channel[freq] = closestf(channel[freq], last_pitchgrid);
				}
			}
		} 
		else if ( ip == 1){
			for ( chan = lo_bin; chan < hi_bin; chan++ ) {
				freq = (chan * 2) + 1;
				if(chan <= hi_tune_bin){
					channel[freq] = closestf(channel[freq], this_pitchgrid);
				}
			}
		}
		else {
			for ( chan = lo_bin; chan < hi_bin; chan++ ) {
				freq = (chan * 2) + 1;
				if(chan <= hi_tune_bin){
					freq_this = closestf(channel[freq], this_pitchgrid);
					freq_last = closestf(channel[freq], last_pitchgrid);
					channel[freq] = freq_last + (freq_this - freq_last) * ip; // linear interpolation
				}
			}
		}
	}
	fftease_oscbank(fft);
}


t_int *pvtuner_perform(t_int *w)
{
	int 	i,j;
    t_pvtuner *x = (t_pvtuner *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *pitchfac = (t_float *)(w[3]);
	t_float *synth_thresh = (t_float *)(w[4]);
	t_float *ip = (t_float *)(w[5]);
	t_float *MSPOutputVector = (t_float *)(w[6]);
	t_fftease *fft = x->fft;
	
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
	
	if (x->mute) {
		memset(MSPOutputVector, 0.0, MSPVectorSize * sizeof(float));
		return w+7;
	}

    fft->P  = *pitchfac;
    fft->synt = *synth_thresh ;
    x->ip = *ip;


	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_pvtuner(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_pvtuner(x);
			
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
            
			do_pvtuner(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+7;
}

void pvtuner_basefreq( t_pvtuner *x, t_floatarg bassfreq)
{
    if(! x->fft->initialized){
        return;
    }
	if( bassfreq < 1 ){
		bassfreq = 1;
		post("%s: base frequency may not go lower than 1 Hz", OBJECT_NAME);
	}
	if( bassfreq > 10000. ){
		bassfreq = 10000.;
		post("%s: base frequency may not go higher than 10000 Hz", OBJECT_NAME);
	}
	x->pbase = bassfreq;
	if( x->current_scale == IMPORTED_SCALE ){
		pvtuner_update_imported( x );
	}
	else if( x->current_scale == DIATONIC ){
		pvtuner_diatonic( x );
	} 
	else if( x->current_scale == EASTERN) {
		pvtuner_eastern( x );
	}
	else if( x->current_scale == MINOR) {
		pvtuner_minor( x );
	}
	else if( x->current_scale == EQ12) {
		pvtuner_eq12( x );
	}
	else if( x->current_scale == PENTATONIC) {
		pvtuner_pentatonic( x );
	}
	else if( x->current_scale == MAJOR_ADDED_SIXTH) {
		pvtuner_major_added_sixth( x );
	}
	else if( x->current_scale == MINOR_ADDED_SIXTH) {
		pvtuner_minor_added_sixth( x );
	}
	else if( x->current_scale == MAJOR_SEVENTH_CHORD) {
		pvtuner_major_seventh_chord( x );
	}
	else if( x->current_scale == MINOR_SEVENTH_CHORD) {
		pvtuner_minor_seventh_chord( x );
	}
	else if( x->current_scale == DOMINANT_SEVENTH_CHORD) {
		pvtuner_dominant_seventh_chord( x );
	}
	else if( x->current_scale == EQ8) {
		pvtuner_eq8( x );
	}
	else if( x->current_scale == PENTACLUST) {
		pvtuner_pentaclust( x );
	}
	else if( x->current_scale == QUARTERCLUST ) {
		pvtuner_quarterclust( x );
	}
	else if( x->current_scale == EQ5 ) {
		pvtuner_eq5( x );
	}
	else if( x->current_scale == SLENDRO ) {
		pvtuner_slendro( x );
	}
	else if( x->current_scale == PELOG ) {
		pvtuner_pelog( x );
	}	
	else {
		post("unknown scale");
	}
}

void pvtuner_free(t_pvtuner *x)
{
    if(x->fft->initialized){
        free(x->this_scale);
        free(x->last_scale);
    }
	fftease_free(x->fft);
    free(x->fft);
}

void pvtuner_update_imported( t_pvtuner *x ){
	t_float *pitchgrid = x->this_scale->pitchgrid;
	t_float factor; 
	int i;

	if( pitchgrid[0] <= 0.0){
		error("%s: illegal first value of scale: %f",OBJECT_NAME,pitchgrid[0]);
		return;
	}
	
	factor = x->pbase / pitchgrid[0];
	pvtuner_copy_scale(x); // copies this scale to next
	for( i = 0; i < x->scale_len; i++ )
		pitchgrid[i] *= factor;
}

void pvtuner_diatonic( t_pvtuner *x ){
	int i, j;
	int octsteps = 7;
	t_pvtuner_scale *s = x->this_scale;
	
	pvtuner_copy_scale(x); // copies this scale to next

	s->pitchgrid[0] = x->pbase;
	s->pitchgrid[1] = x->pbase * (9./8.);
	s->pitchgrid[2] = x->pbase * (5./4.);
	s->pitchgrid[3] = x->pbase * (4./3.);
	s->pitchgrid[4] = x->pbase * (3./2.);
	s->pitchgrid[5] = x->pbase * (27./16.);
	s->pitchgrid[6] = x->pbase * (15./8.);
	s->scale_steps = 7;
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			s->pitchgrid[ i * octsteps + j] = s->pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = DIATONIC ;
	s->scale_steps = 70; // 10 * 7
}

void pvtuner_minor( t_pvtuner *x ){
	int i, j;
	int octsteps = 7;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = x->this_scale->pitchgrid;
	
	pvtuner_copy_scale(x); // copies this scale to next
	
	pitchgrid[0] = x->pbase;
	pitchgrid[1] = x->pbase * (9./8.);
	pitchgrid[2] = x->pbase * (6./5.);
	pitchgrid[3] = x->pbase * (4./3.);
	pitchgrid[4] = x->pbase * (3./2.);
	pitchgrid[5] = x->pbase * (8./5.);
	pitchgrid[6] = x->pbase * (9./5.);
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = MINOR;
	s->scale_steps = 70;
	
}

void pvtuner_pentatonic( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = x->this_scale->pitchgrid;
	int octsteps = 5;
	pvtuner_copy_scale(x); 
	pitchgrid[0] = x->pbase;
	pitchgrid[1] = x->pbase * (9./8.);
	pitchgrid[2] = x->pbase * (81./64.);
	pitchgrid[3] = x->pbase * (3./2.);
	pitchgrid[4] = x->pbase * (27./16.);
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = PENTATONIC;
	s->scale_steps = 50;
}

void pvtuner_eq12( t_pvtuner *x ){
	int i, j;
	t_float expon;
	int octsteps = 12;
	t_float pbase = x->pbase;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = x->this_scale->pitchgrid;
	
	pvtuner_copy_scale(x); // copies this scale to next
	
	// now refill this scale
	pitchgrid[0] = pbase;
	for( i = 0; i < octsteps; i++ ){
		expon = (float) i / (float) octsteps;
		pitchgrid[i] = pbase * pow(2.0,expon);
	}
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = EQ12;
	s->scale_steps = 120;
}

void pvtuner_major_added_sixth( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = x->this_scale->pitchgrid;
	t_float pbase = x->pbase;
	int octsteps = 4;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.259921;
	pitchgrid[2] = pbase * 1.498307;
	pitchgrid[3] = pbase * 1.681793;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = MAJOR_ADDED_SIXTH;
	s->scale_steps = 40;
}

void pvtuner_minor_added_sixth( t_pvtuner *x ){
	int i, j;
	// float *pitchgrid = x->pitchgrid;
	t_float pbase = x->pbase;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = x->this_scale->pitchgrid;
	int octsteps = 4;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.189207;
	pitchgrid[2] = pbase * 1.498307;
	pitchgrid[3] = pbase * 1.587401;
	// scale_steps = 4 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = MINOR_ADDED_SIXTH;
	s->scale_steps = 40;
}

void pvtuner_major_seventh_chord( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = s->pitchgrid;
	t_float pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.25;
	pitchgrid[2] = pbase * 1.5;
	pitchgrid[3] = pbase * 1.875;
	scale_steps = 4 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = MAJOR_SEVENTH_CHORD;
	s->scale_steps = 40;
}

void pvtuner_minor_seventh_chord( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = s->pitchgrid;
	t_float pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.2;
	pitchgrid[2] = pbase * 1.5;
	pitchgrid[3] = pbase * 1.781797;
	scale_steps = 4 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = MINOR_SEVENTH_CHORD;
	s->scale_steps = 40;
}

void pvtuner_dominant_seventh_chord( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = s->pitchgrid;
	t_float pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.25;
	pitchgrid[2] = pbase * 1.5;
	pitchgrid[3] = pbase * 1.781797;
	scale_steps = 4 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = DOMINANT_SEVENTH_CHORD;
	s->scale_steps = 40;
}
void pvtuner_eqn( t_pvtuner *x, t_floatarg steps )
{
int dexter = 0;
	if(steps <= 0.0){
		return;
	}
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = x->this_scale->pitchgrid;
	
	float pbase = x->pbase;
	float factor = pow(2.0, (1.0/steps) );
	
	pvtuner_copy_scale(x);
	
	while(pbase < (x->fft->R / 2.0) && dexter < MAXTONES ){
		pitchgrid[dexter] = pbase;
		pbase = pbase * factor;
		dexter = dexter + 1;
	}
	s->scale_steps = dexter;
	s->current_scale = EQN;
}

void pvtuner_eq8( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = s->pitchgrid;
	t_float pbase = x->pbase;
	int octsteps = 8;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.090508;
	pitchgrid[2] = pbase * 1.189207;
	pitchgrid[3] = pbase * 1.296840;
	pitchgrid[4] = pbase * 1.414214;
	pitchgrid[5] = pbase * 1.542211;
	pitchgrid[6] = pbase * 1.681793;
	pitchgrid[7] = pbase * 1.834008;
	
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = EQ8;
	s->scale_steps = 80;
}

void pvtuner_pentaclust( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = s->pitchgrid;
	t_float pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.059463;
	pitchgrid[2] = pbase * 1.122462;
	pitchgrid[3] = pbase * 1.189207;
	pitchgrid[4] = pbase * 1.259921;
	
	scale_steps = 5 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = PENTACLUST;
	s->scale_steps = 50;
}

void pvtuner_quarterclust( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = s->pitchgrid;
	t_float pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.029302;
	pitchgrid[2] = pbase * 1.059463;
	pitchgrid[3] = pbase * 1.090508;
	pitchgrid[4] = pbase * 1.122462;
	pitchgrid[5] = pbase * 1.155353;
	pitchgrid[6] = pbase * 1.189207;
	pitchgrid[7] = pbase * 1.224054;
	
	scale_steps = 8 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = QUARTERCLUST;
	s->scale_steps = 80;
}

void pvtuner_eq5( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = s->pitchgrid;
	t_float pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.148698;
	pitchgrid[2] = pbase * 1.319508;
	pitchgrid[3] = pbase * 1.515717;
	pitchgrid[4] = pbase * 1.741101;
	
	scale_steps = 5 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = EQ5;
	s->scale_steps = 50;
}

void pvtuner_pelog( t_pvtuner *x ){
	int i, j;
	t_float *pitchgrid = x->this_scale->pitchgrid;
	t_pvtuner_scale *s = x->this_scale;
	t_float pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.152;
	pitchgrid[2] = pbase * 1.340;
	pitchgrid[3] = pbase * 1.532;
	pitchgrid[4] = pbase * 1.756;
	scale_steps = 5 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = PELOG;
	s->scale_steps = 50;
}

void pvtuner_slendro( t_pvtuner *x ){
	int i, j;
	t_float *pitchgrid = x->this_scale->pitchgrid;
	t_pvtuner_scale *s = x->this_scale;
	t_float pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.104;
	pitchgrid[2] = pbase * 1.199;
	pitchgrid[3] = pbase * 1.404;
	pitchgrid[4] = pbase * 1.514;
	pitchgrid[5] = pbase * 1.615;    
	pitchgrid[6] = pbase * 1.787;    
	scale_steps = 7 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = SLENDRO;
	s->scale_steps = 70;
}
void pvtuner_eastern( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	t_float *pitchgrid = s->pitchgrid;
	pvtuner_copy_scale(x);
	int octsteps = 7;
	
	pitchgrid[0] = x->pbase;
	pitchgrid[1] = x->pbase * 1.059463;
	pitchgrid[2] = x->pbase * 1.259921;
	pitchgrid[3] = x->pbase * 1.334840;
	pitchgrid[4] = x->pbase * 1.498307;
	pitchgrid[5] = x->pbase * 1.587401;
	pitchgrid[6] = x->pbase * 1.887749;
	// scale_steps = 7 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < x->scale_steps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = EASTERN ;
	s->scale_steps = 70;
	//   post("eastern scale");
}

t_float closestf(t_float test, t_float *arr) 
{
	int i;
	i = 0;
	if( test <= arr[0] ){
		return arr[0];
	}
	while( i < MAXTONES ){
		if( arr[i] > test ){
			break;
		}
		++i;
	}
	if( i >= MAXTONES - 1) {
		return arr[MAXTONES - 1];
	}
	if( (test - arr[i-1]) > ( arr[i] - test) ) {
		return arr[i];
	} else {
		return arr[i-1];
	}
}

void pvtuner_interpolation(t_pvtuner *x, t_floatarg state)
{
	x->scale_interpolation = (short)state;
}

void pvtuner_mute(t_pvtuner *x, t_floatarg state)
{
	x->mute = (short)state;	
}

void pvtuner_dsp(t_pvtuner *x, t_signal **sp)
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
        pvtuner_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(pvtuner_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
    }
}

