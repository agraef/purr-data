#include "MSPd.h"
#include "fftease.h"

#if MSP
void *pvtuner_class;
#endif 

#if PD
static t_class *pvtuner_class;
#endif

#define OBJECT_NAME "pvtuner~"

#define MAXTONES (1024)
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


typedef struct _pvtuner
{
#if MSP
	t_pxobject x_obj;
#endif
#if PD
	t_object x_obj;
	float x_f;
#endif
	int R;
	int	N;
	int	N2;
	int	Nw;
	int	Nw2; 
	int	D; 
	int	i;
	int	inCount;
	float *Wanal;	
	float *Wsyn;	
	float *input;	
	float *Hwin;
	float *buffer;
	float *channel;
	float *output;
	float *bindex;
	// for convert
	float *c_lastphase_in;
	float *c_lastphase_out;
	float c_fundamental;
	float c_factor_in;
	float c_factor_out;
	
	// for oscbank
	int NP;
	float P;
	int L;
	int first;
	float Iinv;
	float *lastamp;
	float *lastfreq;
	//  float *osc_index;
	float *table;
	float myPInc;
	float ffac;
	//
	int lo_bin;
	int hi_bin;
	int hi_tune_bin;
	float topfreq;
	float synt;
	// for fast fft
	float mult; 
	float *trigland;
	int *bitshuffle;
	//
	//  float *prebuffer;
	//  float *postbuffer;
	//
	int bypass_state;
	int pitch_connected;
	int synt_connected;
	// TUNING
	float *pitchgrid ;
	float pbase ;
	int scale_steps;
	short current_scale;
	short mute;
	//
	float lofreq;
	float hifreq;
	int vs;
	float funda;
	float curfreq;
	int overlap;
	int winfac;
	float tabscale;
	//  int quality;
	int scale_len;
} t_pvtuner;


float closestf(float test, float *arr) ;
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
void pvtuner_init(t_pvtuner *x,short initialized);
void *pvtuner_new(t_symbol *s, int argc, t_atom *argv);
void pvtuner_import_scale(t_pvtuner *x, t_symbol *filename);
void pvtuner_list (t_pvtuner *x, t_symbol *msg, short argc, t_atom *argv);
void pvtuner_topfreq( t_pvtuner *x, t_floatarg f );
void pvtuner_toptune( t_pvtuner *x, t_floatarg f );
void pvtuner_frequency_range(t_pvtuner *x, t_floatarg lo, t_floatarg hi);
t_int *pvtuner_perform(t_int *w);
void pvtuner_basefreq( t_pvtuner *x, t_floatarg bassfreq);
void pvtuner_free(t_pvtuner *x);
void pvtuner_assist (t_pvtuner *x, void *b, long msg, long arg, char *dst);
void pvtuner_bypass(t_pvtuner *x, t_floatarg state);
void pvtuner_mute(t_pvtuner *x, t_floatarg state);
void pvtuner_dsp(t_pvtuner *x, t_signal **sp, short *count);
void pvtuner_float(t_pvtuner *x, double f) ;
void pvtuner_toptune(t_pvtuner *x, t_floatarg f);
void pvtuner_topfreq(t_pvtuner *x, t_floatarg f);
void pvtuner_list (t_pvtuner *x, t_symbol *msg, short argc, t_atom *argv);
void pvtuner_fftinfo(t_pvtuner *x);
void pvtuner_overlap(t_pvtuner *x, t_floatarg f);
void pvtuner_winfac(t_pvtuner *x, t_floatarg f);
void pvtuner_binfo(t_pvtuner *x);

void *pvtuner_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
	t_pvtuner *x = (t_pvtuner *)newobject(pvtuner_class);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
	t_pvtuner *x = (t_pvtuner *)pd_new(pvtuner_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
#endif
	
	x->R = sys_getsr();
	x->D = sys_getblksize();
	
	x->lofreq = atom_getfloatarg(0, argc, argv);  
	x->hifreq = atom_getfloatarg(1, argc, argv); 
	x->overlap = atom_getfloatarg(2, argc, argv);
	x->winfac = atom_getfloatarg(3, argc, argv); 
	
	if(x->lofreq <= 0 || x->lofreq >= x->R/2)
		x->lofreq = 0;
	if(x->hifreq <= 0 || x->hifreq > x->R/2)
		x->hifreq = 4000.0;
	
	
	pvtuner_init(x,0);
	
	return (x);
}

void pvtuner_init(t_pvtuner *x,short initialized)
{
	int i, j;
	int mem;
	float curfreq;
	
	if(!x->R)//temp init if MSP functions returned zero
		x->R = 44100;
	if(!x->D)
		x->D = 256;
  	
	if(!power_of_two(x->overlap))
		x->overlap = 4;
	if(!power_of_two(x->winfac))
		x->winfac = 2;
	
	x->Iinv = 1./x->D;
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	
	x->inCount = -(x->Nw);
	x->mult = 1. / (float) x->N;
	x->c_fundamental =  (float) x->R/(float)( (x->N2)<<1 );
	x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
	x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
    
	if(!initialized){
		x->P = 1.0 ; // default
		x->bypass_state = 0;
		x->mute = 0;
		x->L = 8192;
		x->synt = .000001;
		mem = (MAX_Nw)*sizeof(float);
		x->Wanal = (float *) getbytes(mem);	
		x->Wsyn = (float *) getbytes(mem);	
		x->Hwin = (float *) getbytes(mem);	
		x->input = (float *) getbytes(mem);		
		x->output = (float *) getbytes(mem);	
		mem = (MAX_N)*sizeof(float);
		x->buffer = (float *) getbytes(mem);	
		mem = (MAX_N+2)*sizeof(float);
		x->channel = (float *) getbytes(mem);	
		mem = (MAX_N*2)*sizeof(int);
		x->bitshuffle = (int *) getbytes(mem);	
		mem = (MAX_N*2)*sizeof(float);
		x->trigland = (float *) getbytes(mem);	
		mem = (MAXTONES+1)*sizeof(float);
		x->pitchgrid = (float *) getbytes(mem);	
		mem = (MAX_N+1)*sizeof(float);
		x->lastamp = (float *) getbytes(mem);	
		x->lastfreq = (float *) getbytes(mem);	
		x->bindex = (float *) getbytes(mem);	
		mem = (x->L)*sizeof(float);
		x->table = (float *) getbytes(mem);	
		mem = (MAX_N2+1)*sizeof(float);
		x->c_lastphase_in = (float *) getbytes(mem);	
		x->c_lastphase_out = (float *)getbytes(mem);	
		
		x->pbase = BASE_FREQ;
		pvtuner_diatonic(x);// default scale
	} 
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->lastamp,0,(x->N+1) * sizeof(float));
	memset((char *)x->lastfreq,0,(x->N+1) * sizeof(float));
	memset((char *)x->bindex,0,(x->N+1) * sizeof(float));
	memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
	memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));
	
    for ( i = 0; i < x->L; i++ ) {
		x->table[i] = (float) x->N * cos((float)i * TWOPI / (float)x->L);
    }
	
	if( x->hifreq < x->c_fundamental ) {
		x->hifreq = 3000.0 ;
	}
	
	x->hi_bin = 1;  
	x->curfreq = 0;
	
	while( x->curfreq < x->hifreq ) {
		++(x->hi_bin);
		x->curfreq += x->c_fundamental ;
	}
	
	x->lo_bin = 0;  
	x->curfreq = 0;
	while( x->curfreq < x->lofreq ) {
		++(x->lo_bin);
		x->curfreq += x->c_fundamental ;
	}
	
	if( x->hi_bin >= x->N2 )
		x->hi_bin = x->N2 - 1;
	
	x->hi_tune_bin = x->hi_bin;
	x->myPInc = x->P*x->L/x->R;
	x->ffac = x->P * PI/x->N;
    
	init_rdft( x->N, x->bitshuffle, x->trigland);
	makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);

}

void pvtuner_toptune(t_pvtuner *x, t_floatarg f)
{
	int tbin;
	float curfreq;
	float fundamental = x->c_fundamental;
	tbin = 1;  
	curfreq = 0;
	
	if( f < 0 || f > x->R / 2.0 ){
		error("frequency %f out of range", f);
		return;
	}
	
	while( curfreq < f ) {
		++tbin;
		curfreq += fundamental ;
	}
	
	if( tbin > x->lo_bin && tbin <= x->hi_bin ){
		x->hi_tune_bin = tbin;
	} else {
		error("bin %d out of range", tbin);
	}
	
}

void pvtuner_topfreq(t_pvtuner *x, t_floatarg f)
{
	int tbin;
	float curfreq;
	float fundamental = x->c_fundamental;
	tbin = 1;  
	curfreq = 0;
	
	if( f < 0 || f > x->R / 2.0 ){
		error("frequency %f out of range", f);
		return;
	}
	
	while( curfreq < f ) {
		++tbin;
		curfreq += fundamental ;
	}
	
	if( tbin > x->lo_bin && tbin < x->N2 - 1 ){
		x->hi_bin = tbin;
	} else {
		error("bin %d out of range", tbin);
	}
	
}

void pvtuner_list (t_pvtuner *x, t_symbol *msg, short argc, t_atom *argv)
{
	float val;
	float *pitchgrid = x->pitchgrid;
	
	
	int i = 0;
	
	if( ! atom_getfloatarg(i,argc,argv) ){
		error("either zero length scale or 0.0 (prohibited) is first value");
		return;
	}
	/* first set every value to maximum */
	for(i=0; i < MAXTONES; i++){
		pitchgrid[i] = (float)x->R / 2.0; 
		
	}
	for( i = 0; i < argc; i++ ){
		pitchgrid[ i ] = atom_getfloatarg(i,argc,argv) ;
	}
	
	x->scale_len = i;
	//  post("list scale is length %d", i);
	x->current_scale = IMPORTED_SCALE;
}

void pvtuner_import_scale(t_pvtuner *x, t_symbol *filename) // seems to be broken now
{
	FILE *fp;
	float val;
	float *pitchgrid = x->pitchgrid;
	int scale_len = x->scale_len;
	
	
	if( ! (fp = fopen( filename->s_name, "r")) ){
		error("could not open file %s", filename);
		return;
	}
	scale_len = 0;
	while( ( (fscanf(fp, "%f", &val)) != EOF) && (scale_len < MAXTONES) ){
		pitchgrid[ scale_len++ ] = val;
	}
	fclose( fp );
	x->scale_len = scale_len;
	x->current_scale = IMPORTED_SCALE;
	//  post("read %s", filename->s_name);
}

void pvtuner_binfo(t_pvtuner *x)
{
	post("%s: frequency targets: %f %f", OBJECT_NAME, x->lofreq, x->hifreq);
	post("synthesizing %d bins, from %d to %d",(x->hi_bin - x->lo_bin), x->lo_bin, x->hi_bin);
}

void pvtuner_frequency_range(t_pvtuner *x, t_floatarg lo, t_floatarg hi)
{
	x->lofreq = lo ;
	x->hifreq = hi;
	
	
	if( lo >= hi ){
		error("low frequency must be lower than high frequency");
		return;
	}
	x->curfreq = 0;
	x->hi_bin = 0;
	
	while( x->curfreq < x->hifreq ) {
		++(x->hi_bin);
		x->curfreq += x->c_fundamental ;
	}
	
	
	x->curfreq = 0;
	x->lo_bin = 0;  
	while( x->curfreq < x->lofreq ) {
		++(x->lo_bin);
		x->curfreq += x->c_fundamental ;
	}
	
}


t_int *pvtuner_perform(t_int *w)
{
	int 	i,j, in,on;
	int    amp,freq,chan;
	
	float    a,ainc,f,finc,address;
	int breaker = 0;
	t_pvtuner *x = (t_pvtuner *) (w[1]);
	
	t_float *inbuf = (t_float *)(w[2]);
	t_float *in2 = (t_float *)(w[3]);
	t_float *in3 = (t_float *)(w[4]);
	t_float *outbuf = (t_float *)(w[5]);
	int n = (int)(w[6]);
	
	int D = x->D;
	int I = D;
	int R = x->R;
	int Nw = x->Nw;
	int N = x->N ;
	int N2 = x-> N2;
	int Nw2 = x->Nw2;
	float fundamental = x->c_fundamental;
	float factor_in =  x->c_factor_in;
	float factor_out = x->c_factor_out;
	int *bitshuffle = x->bitshuffle;
	float *trigland = x->trigland;
	float mult = x->mult;	
	float synt = x->synt;
	float P  = x->P; // myPItchfac
	float Iinv = x->Iinv;
	float myPInc = x->myPInc;
	int L = x->L;
	
	/* assign pointers */
	float *table = x->table;
	float *lastamp = x->lastamp ;
	float *lastfreq = x->lastfreq ;
	float *bindex = x->bindex;
	float *lastphase_in = x->c_lastphase_in;
	float *lastphase_out = x->c_lastphase_out;
	
	float *Wanal = x->Wanal;
	float *Wsyn = x->Wsyn;
	float *input = x->input;
	float *Hwin = x->Hwin;
	float *buffer = x->buffer;
	float *channel = x->channel;
	float *output = x->output;
	float *pitchgrid = x->pitchgrid;
	int hi_bin = x->hi_bin;
	int lo_bin = x->lo_bin;
	int hi_tune_bin = x->hi_tune_bin;
	int inCount = x->inCount;
	
	in = on = x->inCount ;
	
	if( x->pitch_connected ) {
  		x->P  = *in2++ ; // myPItchfac
  		x->myPInc = x->P*(float)x->L/(float)x->R;
  	}
  	if ( x->synt_connected ) {
  		synt = *in3++ ;
  	}
	
	
	if (x->bypass_state) { // just send through
		for( j = 0; j < D; j++ ) {
			*outbuf++ = *inbuf++;
		}
		return (w+7);
	}
	
	if (x->mute) {
		for( j = 0; j < D; j++ ) {
			*outbuf++ = 0.0;
		}
		return (w+7);
	}
	
	
	inCount += D;
	
	for ( j = 0 ; j < (Nw - D) ; j++ ){
		input[j] = input[j+D];
	}
	for ( j = (Nw-D), i = 0 ; j < Nw; j++, i++ ) {
		input[j] = *inbuf++;
	}
	
	fold( input, Wanal, Nw, buffer, N, inCount );   
	rdft( N, 1, buffer, bitshuffle, trigland );
	convert( buffer, channel, N2, lastphase_in, fundamental, factor_in );
	
	// start osc bank 
    
	for ( chan = lo_bin; chan < hi_bin; chan++ ) {
		
		freq = ( amp = ( chan << 1 ) ) + 1;
		if ( channel[amp] < synt ){ 
			breaker = 1;
		}
		if( breaker ) {
			breaker = 0 ;
		} else {
			if(chan <= hi_tune_bin){
				channel[freq] = closestf(channel[freq], pitchgrid);
			}
			channel[freq] *= myPInc;
			finc = ( channel[freq] - ( f = lastfreq[chan] ) )*Iinv;
			ainc = ( channel[amp] - ( a = lastamp[chan] ) )*Iinv;
			address = bindex[chan];
			for ( n = 0; n < I; n++ ) {
				output[n] += a*table[ (int) address ];
				
				address += f;
				while ( address >= L )
					address -= L;
				while ( address < 0 )
					address += L;
				a += ainc;
				f += finc;
			}
			lastfreq[chan] = channel[freq];
			lastamp[chan] = channel[amp];
			bindex[chan] = address;
		}
	}
	
	for ( j = 0; j < D; j++ ){
		*outbuf++ = output[j] * mult;
	}
	for ( j = 0; j < Nw - D; j++ ){
		output[j] = output[j+D];
	}
	
	for ( j = Nw - D; j < Nw; j++ ){
		output[j] = 0.;
	}	
	
	x->inCount = inCount % Nw;
	
	return (w+7);
}		


void pvtuner_basefreq( t_pvtuner *x, t_floatarg bassfreq)
{
	if( bassfreq < 10. )
		bassfreq = 10.  ;
	if( bassfreq > 10000. )
		bassfreq = 10000.;
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
#if MSP
	dsp_free( (t_pxobject *) x);
#endif
	freebytes(x->c_lastphase_in,0);
	freebytes(x->c_lastphase_out,0);
	freebytes(x->trigland,0);
	freebytes(x->bitshuffle,0);
	freebytes(x->Wanal,0);
	freebytes(x->Wsyn,0);
	freebytes(x->input,0);
	freebytes(x->Hwin,0);
	freebytes(x->buffer,0);
	freebytes(x->channel,0);
	freebytes(x->output,0);
	freebytes(x->lastamp,0);
	freebytes(x->lastfreq,0);
	freebytes(x->bindex,0);
	freebytes(x->table,0);
	freebytes(x->pitchgrid,0);
}

#if MSP
void pvtuner_assist (t_pvtuner *x, void *b, long msg, long arg, char *dst)
{
	if (msg==ASSIST_INLET) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Input");
				break;
			case 1:
				sprintf(dst,"(signal/float) Pitch Modification Factor");
				break;
			case 2:
				sprintf(dst,"(signal/float) Synthesis Threshold");
				break;
		}
	} else if (msg==ASSIST_OUTLET) {
		sprintf(dst,"(signal) Output");
	}
}
#endif

#if PD
void pvtuner_assist (t_pvtuner *x, void *b, long msg, long arg, char *dst)
{
	post("INLETS: input pitch_factor synthesis_threshold");
	post("ARGUMENTS: lo_freq hi_freq");
}
#endif


void pvtuner_update_imported( t_pvtuner *x ){
	float *pitchgrid = x->pitchgrid;
	float factor = x->pbase / pitchgrid[0];
	int i;
	for( i = 0; i < x->scale_len; i++ )
		pitchgrid[i] *= factor;
}

void pvtuner_diatonic( t_pvtuner *x ){
	int i, j;
	//  post("calling diatonic for vs %d",x->vs);
	x->pitchgrid[0] = x->pbase;
	x->pitchgrid[1] = x->pbase * (9./8.);
	x->pitchgrid[2] = x->pbase * (5./4.);
	x->pitchgrid[3] = x->pbase * (4./3.);
	x->pitchgrid[4] = x->pbase * (3./2.);
	x->pitchgrid[5] = x->pbase * (27./16.);
	x->pitchgrid[6] = x->pbase * (15./8.);
	x->scale_steps = 7;
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < x->scale_steps; j++ ){
			x->pitchgrid[ i*x->scale_steps + j] = x->pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	x->current_scale = DIATONIC ;
}

void pvtuner_minor( t_pvtuner *x ){
	int i, j;
	x->pitchgrid[0] = x->pbase;
	x->pitchgrid[1] = x->pbase * (9./8.);
	x->pitchgrid[2] = x->pbase * (6./5.);
	x->pitchgrid[3] = x->pbase * (4./3.);
	x->pitchgrid[4] = x->pbase * (3./2.);
	x->pitchgrid[5] = x->pbase * (8./5.);
	x->pitchgrid[6] = x->pbase * (9./5.);
	x->scale_steps = 7;
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < x->scale_steps; j++ ){
			x->pitchgrid[ i*x->scale_steps + j] = x->pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	x->current_scale = MINOR ;
}

void pvtuner_pentatonic( t_pvtuner *x ){
	int i, j;
	
	x->pitchgrid[0] = x->pbase;
	x->pitchgrid[1] = x->pbase * (9./8.);
	x->pitchgrid[2] = x->pbase * (81./64.);
	x->pitchgrid[3] = x->pbase * (3./2.);
	x->pitchgrid[4] = x->pbase * (27./16.);
	x->scale_steps = 5;
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < x->scale_steps; j++ ){
			x->pitchgrid[ i*x->scale_steps + j] = x->pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	x->current_scale = PENTATONIC ;
}

void pvtuner_eq12( t_pvtuner *x ){
	int i, j;
	float expon;
	x->pitchgrid[0] = x->pbase;
	for( i = 0; i < 12; i++ ){
		expon = (float) i / 12.0 ;
		x->pitchgrid[i] = x->pbase * pow(2.0,expon);
	}
	x->scale_steps = 12;
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < x->scale_steps; j++ ){
			x->pitchgrid[ i*x->scale_steps + j] = x->pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	x->current_scale = EQ12 ;
}

void pvtuner_major_added_sixth( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.259921;
	pitchgrid[2] = pbase * 1.498307;
	pitchgrid[3] = pbase * 1.681793;
	scale_steps = 4 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	x->current_scale = MAJOR_ADDED_SIXTH;
	x->scale_steps = scale_steps;
}

void pvtuner_minor_added_sixth( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.189207;
	pitchgrid[2] = pbase * 1.498307;
	pitchgrid[3] = pbase * 1.587401;
	scale_steps = 4 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	x->current_scale = MINOR_ADDED_SIXTH;
	x->scale_steps = scale_steps;
}

void pvtuner_major_seventh_chord( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
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
	x->current_scale = MAJOR_SEVENTH_CHORD;
	x->scale_steps = scale_steps;
}

void pvtuner_minor_seventh_chord( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
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
	x->current_scale = MINOR_SEVENTH_CHORD;
	x->scale_steps = scale_steps;
}

void pvtuner_dominant_seventh_chord( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
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
	x->current_scale = DOMINANT_SEVENTH_CHORD;
	x->scale_steps = scale_steps;
}

void pvtuner_eq8( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.090508;
	pitchgrid[2] = pbase * 1.189207;
	pitchgrid[3] = pbase * 1.296840;
	pitchgrid[4] = pbase * 1.414214;
	pitchgrid[5] = pbase * 1.542211;
	pitchgrid[6] = pbase * 1.681793;
	pitchgrid[7] = pbase * 1.834008;
	
	scale_steps = 8 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	x->current_scale = EQ8;
	x->scale_steps = scale_steps;
}

void pvtuner_pentaclust( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
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
	x->current_scale = PENTACLUST;
	x->scale_steps = scale_steps;
}

void pvtuner_quarterclust( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
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
	x->current_scale = QUARTERCLUST;
	x->scale_steps = scale_steps;
}

void pvtuner_eq5( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
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
	x->current_scale = EQ5;
	x->scale_steps = scale_steps;
}

void pvtuner_pelog( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
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
	x->current_scale = PELOG;
	x->scale_steps = scale_steps;
}

void pvtuner_slendro( t_pvtuner *x ){
	int i, j;
	float *pitchgrid = x->pitchgrid;
	float pbase = x->pbase;
	int scale_steps;
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
	x->current_scale = SLENDRO;
	x->scale_steps = scale_steps;
}
void pvtuner_eastern( t_pvtuner *x ){
	int i, j;
	
	x->pitchgrid[0] = x->pbase;
	x->pitchgrid[1] = x->pbase * 1.059463;
	x->pitchgrid[2] = x->pbase * 1.259921;
	x->pitchgrid[3] = x->pbase * 1.334840;
	x->pitchgrid[4] = x->pbase * 1.498307;
	x->pitchgrid[5] = x->pbase * 1.587401;
	x->pitchgrid[6] = x->pbase * 1.887749;
	x->scale_steps = 7 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < x->scale_steps; j++ ){
			x->pitchgrid[ i*x->scale_steps + j] = x->pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	x->current_scale = EASTERN ;
	//   post("eastern scale");
}

#if MSP
void pvtuner_float(t_pvtuner *x, double f) 
{
	//	int inlet = ((t_pxobject*)x)->z_in;
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
	{
		x->P = f;
		x->myPInc = x->P*x->L/x->R;
		//		post("P set to %f",f);
	}
	else if (inlet == 2)
	{
		x->synt = f;
		//		post("synt set to %f",f);
	}
}
#endif

float closestf(float test, float *arr) 
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


void pvtuner_bypass(t_pvtuner *x, t_floatarg state)
{
	x->bypass_state = state;	
}


void pvtuner_mute(t_pvtuner *x, t_floatarg state)
{
	x->mute = state;	
	// post("mute: %d", state);
}

void pvtuner_overlap(t_pvtuner *x, t_floatarg f)
{
	int i = (int) f;
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->overlap = i;
	pvtuner_init(x,1);
}

void pvtuner_winfac(t_pvtuner *x, t_floatarg f)
{
	int i = (int)f;
	
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = i;
	pvtuner_init(x,2);
}

void pvtuner_fftinfo(t_pvtuner *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void pvtuner_dsp(t_pvtuner *x, t_signal **sp, short *count)
{
#if MSP
	x->pitch_connected = count[1];
	x->synt_connected = count[2];
#endif
#if PD
	x->pitch_connected = 1;
	x->synt_connected = 1;
#endif
	
	if(sp[0]->s_n != x->vs || x->R != sp[0]->s_sr ){
		
		x->D = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		pvtuner_init(x,1);
		
	}
	dsp_add(pvtuner_perform, 6, x, 
			sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, 
			sp[0]->s_n);
}

#if MSP
void main(void)
{
	setup((t_messlist **)&pvtuner_class, (method)pvtuner_new, (method)pvtuner_free, 
		  (short)sizeof(t_pvtuner), 0, A_GIMME, 0);
	addmess((method)pvtuner_dsp, "dsp", A_CANT, 0);
	addmess((method)pvtuner_assist,"assist",A_CANT,0);
	addmess((method)pvtuner_bypass,"bypass",A_DEFFLOAT,0);
	addmess((method)pvtuner_mute,"mute",A_DEFFLOAT,0);
	addmess((method)pvtuner_basefreq,"basefreq",A_DEFFLOAT,0);
	addmess((method)pvtuner_diatonic,"diatonic",0);
	addmess((method)pvtuner_eastern,"eastern",0);
	addmess((method)pvtuner_minor,"minor",0);
	addmess((method)pvtuner_eq12,"eq12",0);
	addmess((method)pvtuner_pentatonic,"pentatonic",0);
	addmess((method)pvtuner_major_added_sixth,"added_sixth_major",0);
	addmess((method)pvtuner_minor_added_sixth,"added_sixth_minor",0);
	addmess((method)pvtuner_major_seventh_chord,"major_seventh_chord",0);
	addmess((method)pvtuner_minor_seventh_chord,"minor_seventh_chord",0);
	addmess((method)pvtuner_dominant_seventh_chord,"dominant_seventh_chord",0);
	addmess((method)pvtuner_eq8,"eq8",0);               
	addmess((method)pvtuner_pentaclust,"pentaclust",0);
	addmess((method)pvtuner_quarterclust,"quarterclust",0);   
	addmess((method)pvtuner_eq5,"eq5",0); 
	addmess((method)pvtuner_slendro,"slendro",0);
	addmess((method)pvtuner_pelog,"pelog",0); 
	addmess((method)pvtuner_import_scale,"import_scale",A_DEFSYM,0);   
	addmess((method)pvtuner_list,"list",A_GIMME,0);    
	addmess((method)pvtuner_topfreq,"topfreq",A_DEFFLOAT,0); 
	addmess((method)pvtuner_toptune,"toptune",A_DEFFLOAT,0); 
	addmess((method)pvtuner_frequency_range,"frequency_range",A_FLOAT,A_FLOAT, 0);    
	addmess((method)pvtuner_overlap,"overlap",A_DEFFLOAT,0);
	addmess((method)pvtuner_winfac,"winfac",A_DEFFLOAT,0);
	addmess((method)pvtuner_fftinfo,"fftinfo",0);
	addmess((method)pvtuner_binfo,"binfo",0);
	addfloat((method)pvtuner_float);
	dsp_initclass();
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif
#if PD
void pvtuner_tilde_setup(void)
{
	pvtuner_class = class_new(gensym("pvtuner~"), (t_newmethod)pvtuner_new, 
							  (t_method)pvtuner_free ,sizeof(t_pvtuner), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(pvtuner_class, t_pvtuner, x_f );
	class_addmethod(pvtuner_class, (t_method)pvtuner_dsp, gensym("dsp"), 0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_mute, gensym("mute"), A_DEFFLOAT,0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_bypass, gensym("bypass"), A_DEFFLOAT,0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_assist, gensym("assist"), 0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_basefreq,gensym("basefreq"),A_DEFFLOAT,0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_frequency_range,gensym("frequency_range"),A_DEFFLOAT,A_DEFFLOAT,0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_diatonic,gensym("diatonic"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_eastern,gensym("eastern"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_minor,gensym("minor"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_eq12,gensym("eq12"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_pentatonic,gensym("pentatonic"),0);
	/* Pd cannot disambiguate */
	/*  
		class_addmethod(pvtuner_class, (t_method)pvtuner_added_sixth_major,gensym("added_sixth_major"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_added_sixth_minor,gensym("added_sixth_minor"),0);
	*/
	class_addmethod(pvtuner_class, (t_method)pvtuner_major_added_sixth,gensym("major_added_sixth"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_minor_added_sixth,gensym("minor_added_sixth"),0);
	
	class_addmethod(pvtuner_class, (t_method)pvtuner_major_seventh_chord,gensym("major_seventh_chord"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_minor_seventh_chord,gensym("minor_seventh_chord"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_dominant_seventh_chord,gensym("dominant_seventh_chord"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_eq8,gensym("eq8"),0);               
	class_addmethod(pvtuner_class, (t_method)pvtuner_pentaclust,gensym("pentaclust"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_quarterclust,gensym("quarterclust"),0);   
	class_addmethod(pvtuner_class, (t_method)pvtuner_eq5,gensym("eq5"),0); 
	class_addmethod(pvtuner_class, (t_method)pvtuner_slendro,gensym("slendro"),0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_pelog,gensym("pelog"),0);       
	class_addmethod(pvtuner_class, (t_method)pvtuner_import_scale,gensym("import_scale"),A_DEFSYM,0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_toptune,gensym("toptune"),A_FLOAT, 0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_topfreq,gensym("topfreq"),A_FLOAT, 0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_list,gensym("list"),A_GIMME, 0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_frequency_range,gensym("frequency_range"),A_FLOAT,A_FLOAT, 0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_overlap,gensym("overlap"),A_FLOAT, 0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_winfac,gensym("winfac"),A_FLOAT, 0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_fftinfo,gensym("fftinfo"), 0);
	class_addmethod(pvtuner_class, (t_method)pvtuner_binfo,gensym("binfo"), 0);
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif
