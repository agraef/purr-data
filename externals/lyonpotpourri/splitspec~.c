#include "MSPd.h"

#define MAXSTORE (128)
#define OBJECT_NAME "splitspec~"

/* Pd version of splitspec~ */

static t_class *splitspec_class;

typedef struct _splitspec
{
    t_object x_obj;
    t_float x_f;
    int N;
    int N2;
    void *list_outlet;
    void *phase_outlet;
    t_atom *list_data;
    // Avoid buffer sharing overwrites with local copies
    t_float *inmag_loc;
    t_float *inphase_loc;
    t_float *t_offset_loc;
    t_float *b_offset_loc;
    t_float *manual_control_loc;
    float frame_duration;
    int table_offset;
    int bin_offset;
    float *last_mag;
    float *current_mag;
    int *last_binsplit;
    int *current_binsplit;
    int **stored_binsplits;
    short *stored_slots;
    short new_distribution;
    short interpolation_completed;
    short bypass;
    short initialize;
    int manual_override;
    long countdown_samps; // samps for a given fadetime
    long counter;
    int overlap_factor; // compensate for overlap in fade
    float sr;
    float fl_phase; // show phase as float
    int hopsamps; // number of samples to hop
    int channel_count; // number of channels to split
    t_clock *phase_clock;
    t_float **magvecs;// connect to mag input vectors
    t_float **phasevecs;// point to phase input vectors
} t_splitspec;

void *splitspec_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *splitspec_perform(t_int *w);
void splitspec_dsp(t_splitspec *x, t_signal **sp);
void splitspec_showstate( t_splitspec *x );
void splitspec_bypass( t_splitspec *x, t_floatarg toggle);
void splitspec_manual_override( t_splitspec *x, t_floatarg toggle );
void splitspec_setstate (t_splitspec *x, t_symbol *msg, int argc, t_atom *argv);
void splitspec_ramptime (t_splitspec *x, t_symbol *msg, int argc, t_atom *argv);
int rand_index( int max);
void splitspec_scramble (t_splitspec *x);
void splitspec_spiral(t_splitspec *x);
void splitspec_squantize(t_splitspec *x, t_floatarg blockbins);
void splitspec_overlap( t_splitspec *x, t_floatarg factor);

void splitspec_store( t_splitspec *x, t_floatarg floc);
void splitspec_recall( t_splitspec *x, t_floatarg floc);
void splitspeci( int *current_binsplit, int *last_binsplit, int bin_offset, int table_offset,
                float *current_mag, float *last_mag, float *inmag, float *dest_mag, int start, int end, int n,
                float oldfrac, float newfrac );
void splitspec( int *binsplit, int bin_offset, int table_offset,
               float *inmag, float *dest_mag, int start, int end, int n );

void splitspec_dsp_free( t_splitspec *x );

void splitspec_phaseout(t_splitspec *x);
int splitspec_closestPowerOfTwo(int p);
void splitspec_tilde_setup(void){
   splitspec_class = class_new(gensym("splitspec~"), (t_newmethod)splitspec_new,
                                (t_method)splitspec_dsp_free, sizeof(t_splitspec),0,A_GIMME,0);
    
    /* splitspec_class = class_new(gensym("splitspec~"), (t_newmethod)splitspec_new,
                                0, sizeof(t_splitspec),0,A_GIMME,0); */
	CLASS_MAINSIGNALIN(splitspec_class, t_splitspec, x_f);
    class_addmethod(splitspec_class, (t_method)splitspec_dsp, gensym("dsp"),0);
    class_addmethod(splitspec_class, (t_method)splitspec_showstate, gensym("showstate"),0);
    class_addmethod(splitspec_class, (t_method)splitspec_scramble, gensym("scramble"),0);
    class_addmethod(splitspec_class, (t_method)splitspec_spiral, gensym("spiral"),0);
    class_addmethod(splitspec_class, (t_method)splitspec_squantize, gensym("squantize"),A_FLOAT,0);
    class_addmethod(splitspec_class, (t_method)splitspec_manual_override, gensym("manual_override"),A_FLOAT,0);
    class_addmethod(splitspec_class, (t_method)splitspec_bypass, gensym("bypass"),A_FLOAT,0);
    class_addmethod(splitspec_class, (t_method)splitspec_store, gensym("store"),A_FLOAT,0);
    class_addmethod(splitspec_class, (t_method)splitspec_recall, gensym("recall"),A_FLOAT,0);
    class_addmethod(splitspec_class, (t_method)splitspec_overlap, gensym("overlap"),A_FLOAT,0);
    class_addmethod(splitspec_class, (t_method)splitspec_setstate, gensym("setstate"),A_GIMME,0);
    class_addmethod(splitspec_class, (t_method)splitspec_ramptime, gensym("ramptime"),A_GIMME,0);
    potpourri_announce(OBJECT_NAME);
}

void splitspec_phaseout(t_splitspec *x)
{
    outlet_float(x->phase_outlet, x->fl_phase);
}

void splitspec_overlap( t_splitspec *x, t_floatarg fol )
{
    int overlap = (int)fol;
	if( overlap < 2 ){
		post("splitspec~: illegal overlap %d",overlap);
	}
	x->overlap_factor = overlap;
}

void splitspec_spiral(t_splitspec *x)
{
    int i,j,k;
    int offset;

    int channel_count = x->channel_count;
    int *current_binsplit = x->current_binsplit;
    int *last_binsplit = x->last_binsplit;
    
    int N2 = x->N2;
    
    offset = N2 / channel_count;
    
    x->new_distribution = 1;
    x->interpolation_completed = 0;
    for( i = 0; i < N2; i++ ){
        last_binsplit[i] = current_binsplit[i];
    }
    k = 0;
    for( i = 0; i < N2 / channel_count; i++){
        for(j = 0; j < channel_count; j++){
            current_binsplit[i + (j * offset)] = k++;
        }
    }
    if(! x->counter) { // Ramp Off - Immediately set last to current
        for( i = 0; i < N2; i++ ){
            last_binsplit[ i ] = current_binsplit[ i ];
        }
    }
}

void splitspec_squantize(t_splitspec *x, t_floatarg bb)
{
    int i, j, k;
    
    int maxblock;
    int iterations;
    int bincount = 0;
    int *current_binsplit = x->current_binsplit;
    int *last_binsplit = x->last_binsplit;
    int blockbins = (int) bb;
    int N2 = x->N2;
    int channel_count = x->channel_count;
    blockbins = splitspec_closestPowerOfTwo( blockbins );
    maxblock = N2 / channel_count;
    if( blockbins < 1 || blockbins > maxblock ){
        error("%d is out of bounds - must be between 1 and %d", blockbins, maxblock);
        return;
    }
    
    iterations = N2 /  channel_count /  blockbins;
    x->new_distribution = 1;
    x->interpolation_completed = 0;
    
    
    for( i = 0; i < N2; i++ ){
        last_binsplit[i] = current_binsplit[i];
    }
    
    if( iterations == 1 ){
        for( i = 0; i < N2 ; i++ ){
            current_binsplit[i] = i;
        }
    }
    else {
        for( k = 0; k < iterations; k++ ) {
            for( i = 0; i < N2; i += maxblock  ){
                for( j = 0; j < blockbins; j++ ){
                    if( i + j + k * blockbins < N2 ){
                        current_binsplit[i + j + k * blockbins] = bincount++;
                        // post("assigning %d to position %d", bincount-1, i+j+k*blockbins);
                    } else {
                        // error("%d out of range", i + j + k * blockbins);
                    }
                }
            }
        }
    }
    
    
//    x->frames_left = x->ramp_frames;
    if(! x->counter) { // Ramp Off - Immediately set last to current
        for( i = 0; i < N2; i++ ){
            last_binsplit[ i ] = current_binsplit[ i ];
        }
    }
}

void splitspec_bypass( t_splitspec *x, t_floatarg toggle )
{
    x->bypass = (int)toggle;
}

void splitspec_manual_override( t_splitspec *x, t_floatarg toggle )
{
    x->manual_override = (int) toggle;
}

void splitspec_dsp_free( t_splitspec *x ){
    int i;
    if(x->initialize == 0){
        free(x->list_data);
        free(x->last_binsplit);
        free(x->current_binsplit);
        free(x->last_mag);
        free(x->current_mag);
        free(x->stored_slots);
        for(i = 0; i < MAXSTORE; i++){
            free(x->stored_binsplits[i]);
        }
        free(x->stored_binsplits);
        free(x->inmag_loc);
        free(x->inphase_loc);
        free(x->t_offset_loc);
        free(x->b_offset_loc);
        free(x->manual_control_loc);
        free(x->magvecs);
        free(x->phasevecs);
    }
}

void splitspeci( int *current_binsplit, int *last_binsplit, int bin_offset, int table_offset,
                float *current_mag, float *last_mag, float *inmag, float *dest_mag, int start, int end, int n,
                float oldfrac, float newfrac )
{
    int i;
    int bindex;
    
    for( i = 0; i < n; i++ ){
        last_mag[i] = current_mag[i] = 0.0;
    }
    for( i = start; i < end; i++ ){
        bindex = current_binsplit[ (i + table_offset) % n ];
        bindex = ( bindex + bin_offset ) % n;
        current_mag[ bindex ] = inmag[ bindex ];
        bindex = last_binsplit[ (i + table_offset) % n ];
        bindex = ( bindex + bin_offset ) % n;
        last_mag[ bindex ] = inmag[ bindex ];
    }
    for( i = 0; i < n; i++){
        if(! current_mag[i] && ! last_mag[i]){
            dest_mag[i] = 0.0;
        } else {
            dest_mag[i] = oldfrac * last_mag[i] + newfrac * current_mag[i];
        }
    }
    
}

void splitspec( int *binsplit, int bin_offset, int table_offset,
               float *inmag, float *dest_mag, int start, int end, int n )
{
    int i;
    int bindex;
    // n is actually N2
    for( i = start; i < end; i++){
        bindex = binsplit[ (i + table_offset) % n ];
        bindex = ( bindex + bin_offset ) % n;
        dest_mag[ bindex ] = inmag[ bindex ];
    }
}


void splitspec_store( t_splitspec *x, t_floatarg floc)
{
    int **stored_binsplits = x->stored_binsplits;
    int *current_binsplit = x->current_binsplit;
    short *stored_slots = x->stored_slots;
    int location = (int)floc;
    int i;
    
    if( location < 0 || location > MAXSTORE - 1 ){
        error("location must be between 0 and %d, but was %d", MAXSTORE, location);
        return;
    }
    for(i = 0; i < x->N2; i++ ){
        stored_binsplits[location][i] = current_binsplit[i];
    }
    stored_slots[location] = 1;
    
//    post("stored bin split at location %d", location);
}

void splitspec_recall( t_splitspec *x, t_floatarg floc)
{
    int **stored_binsplits = x->stored_binsplits;
    int *current_binsplit = x->current_binsplit;
    int *last_binsplit = x->last_binsplit;
    short *stored_slots = x->stored_slots;
    int i;
    int location = (int)floc;
    if( location < 0 || location > MAXSTORE - 1 ){
        error("location must be between 0 and %d, but was %d", MAXSTORE, location);
        return;
    }
    if( ! stored_slots[location] ){
        error("nothing stored at location %d", location);
        return;
    }
    
    for(i = 0; i < x->N2; i++ ){
        last_binsplit[i] = current_binsplit[i];
        current_binsplit[i] = stored_binsplits[location][i];
    }
    
    x->new_distribution = 1;
    x->interpolation_completed = 0;
 //   x->frames_left = x->ramp_frames;
    if(! x->counter) { // Ramp Off - Immediately set last to current
        for( i = 0; i < x->N2; i++ ){
            x->last_binsplit[ i ] = x->current_binsplit[ i ];
        }
    }
}

void *splitspec_new(t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_splitspec *x = (t_splitspec *)pd_new(splitspec_class);
    
    // x->channel_count = 8; // hard wire just for now
    x->channel_count = (int) atom_getfloatarg(0, argc, argv);
    x->channel_count = splitspec_closestPowerOfTwo( x->channel_count );
    // post("Channel count is: %d", x->channel_count);
    srand( time( 0 ) );
    
    for(i=0; i < 4; i++){
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"),gensym("signal"));
    }
    for(i=0; i < x->channel_count * 2; i++){
        outlet_new(&x->x_obj, gensym("signal"));
    }
    x->list_outlet = (t_outlet *) outlet_new(&x->x_obj, gensym("list")); // to report random distribution
    x->phase_outlet = (t_outlet *) outlet_new((t_object *)x, &s_float);
    x->phase_clock = (t_clock *) clock_new((void *)x, (t_method)splitspec_phaseout);
    x->bypass = 0;
    x->table_offset = 0;
    x->bin_offset = 0;
    
    x->sr = sys_getsr();
 	x->counter = 0;
    x->overlap_factor = 8; // default
    
	x->countdown_samps = 1.0 * x->sr; // 1 second fade time by default
    
    x->initialize = 1;
    x->manual_override = 0;
    
    x->inmag_loc = (t_float *) calloc(8192,sizeof(t_float));
    x->inphase_loc = (t_float *) calloc(8192,sizeof(t_float));
    x->t_offset_loc = (t_float *) calloc(8192,sizeof(t_float));
    x->b_offset_loc = (t_float *) calloc(8192,sizeof(t_float));
    x->manual_control_loc = (t_float *) calloc(8192,sizeof(t_float));
    x->magvecs = (t_float **) malloc(x->channel_count * sizeof(t_float *));
    x->phasevecs = (t_float **) malloc(x->channel_count * sizeof(t_float *));
    return x;
}

int splitspec_closestPowerOfTwo(int p){
    int base = 2;
    while(base < p){
        base *= 2;
    }
    return base;
}

t_int *splitspec_perform(t_int *w)
{
	
    int i, j;

    t_splitspec *x = (t_splitspec *) (w[1]);
    int channel_count = x->channel_count;
    
    float *inmag = (t_float *)(w[2]);
    float *inphase = (t_float *)(w[3]);
    float *t_offset = (t_float *)(w[4]);
    float *b_offset = (t_float *)(w[5]);
    float *manual_control = (t_float *)(w[6]);

    t_float **magvecs = x->magvecs;
    t_float **phasevecs = x->phasevecs;
    int n = (int) w[7 + (channel_count * 2)];
    
    int table_offset = x->table_offset;
    int bin_offset = x->bin_offset;
    
    int *current_binsplit = x->current_binsplit;
    int *last_binsplit = x->last_binsplit;
    float *last_mag = x->last_mag;
    float *current_mag = x->current_mag;
    long counter = x->counter;
    long countdown_samps = x->countdown_samps;
    float frac, oldgain, newgain;
    t_float *inmag_loc = x->inmag_loc;
    t_float *inphase_loc = x->inphase_loc;
    t_float *t_offset_loc = x->t_offset_loc;
    t_float *b_offset_loc = x->b_offset_loc;
    t_float *manual_control_loc = x->manual_control_loc;
    int next_ptr = 8 + (channel_count * 2);
    int hopsamps = x->hopsamps;
    int N2 = x->N2;
    /****/
    
    // copy inputs to local buffers
    
    for(i = 0; i < n; i++){
        inmag_loc[i] = inmag[i];
        inphase_loc[i] = inphase[i];
        t_offset_loc[i] = t_offset[i];
        b_offset_loc[i] = b_offset[i];
        manual_control_loc[i] = manual_control[i];
    }
    
    // assign local vector pointers
    for(i = 0, j= 0; i < channel_count * 2; i+=2, j++){
        magvecs[j] = (t_float *) w[ 7 + i ];
        phasevecs[j] = (t_float *) w[ 8 + i ];
    }
    
    table_offset = *t_offset_loc * n;
    bin_offset = *b_offset_loc * n;
    
    if( table_offset  < 0 )
        table_offset *= -1;
    if( bin_offset  < 0 )
        bin_offset *= -1;
  	
    
    // n == fftsize / 2 (N2)
    // n is the number of "bins", and is also the number of values in each signal vector
    
    if( x->bypass ){
        for( i = 0; i < n; i++){
            for(j = 0; j < channel_count; j++){
                magvecs[j][i] = inmag_loc[i] * 0.5;;
                phasevecs[j][i] = inphase_loc[i];
            }
        }
        return (w + next_ptr);
    }
    
    // ZERO OUT MAGNITUDES AND COPY PHASES TO OUTPUT
    for( i = 0; i < n; i++ ){
        for(j = 0; j < channel_count; j++){
            magvecs[j][i] = 0.0;
            phasevecs[j][i] = inphase_loc[i];
        }
    }
    
    // Special case of live control over interpolation
    if( x->manual_override ){
        
        // do interpolation
        frac = *manual_control_loc;
        // sanity check here
        if( frac < 0 ) { frac = 0; }
        if( frac >1.0 ){ frac = 1.0; }
        oldgain = cos( frac * PIOVERTWO );
        newgain = sin( frac * PIOVERTWO );
        
        for(j = 0; j < channel_count; j++){
            splitspeci( current_binsplit, last_binsplit, bin_offset, table_offset,
                       current_mag, last_mag, inmag_loc, magvecs[j],
                       N2*j/channel_count, N2*(j+1)/channel_count, N2, oldgain, newgain );
        }
        return (w + next_ptr);
    }
    
    // Normal operation
    if( x->new_distribution ){
        x->new_distribution = 0;
        // put out contents of last distribution
        for(j = 0; j < channel_count; j++){
            splitspec(last_binsplit, bin_offset, table_offset, inmag_loc, magvecs[j],
                      N2*j/channel_count, N2*(j+1)/channel_count, N2);
        }
        frac = 0.0;
        
    } else if ( x->interpolation_completed ) {
        // put out contents of current distribution
        for(j = 0; j < channel_count; j++){
            splitspec(current_binsplit, bin_offset, table_offset, inmag_loc, magvecs[j],
                      N2*j/channel_count, N2*(j+1)/channel_count, N2);
        }
        frac = 1.0;
    } else {
        // do interpolation
        frac = (float) counter / (float) countdown_samps;
        oldgain = cos( frac * PIOVERTWO );
        newgain = sin( frac * PIOVERTWO );
        for(j = 0; j < channel_count; j++){
            splitspeci( current_binsplit, last_binsplit, bin_offset, table_offset,
                       current_mag, last_mag, inmag_loc, magvecs[j],
                       N2*j/channel_count, N2*(j+1)/channel_count, N2, oldgain, newgain );
        }
        // end of interpolation
        
        counter += hopsamps;
        if( counter >= countdown_samps )
        {
            counter = countdown_samps;
            x->interpolation_completed = 1;
        }
    }
    
    x->fl_phase = frac;
    clock_delay(x->phase_clock,0.0); // send current phase to float outlet
    x->counter = counter;
    return (w + next_ptr);
}


void splitspec_scramble (t_splitspec *x)
{
    int i;
    
    int max = x->N2;
    int swapi, tmp;
    int N2 = x->N2;
    int *current_binsplit = x->current_binsplit;
    int *last_binsplit = x->last_binsplit;
    
    x->new_distribution = 1;
    x->interpolation_completed = 0;

    // Copy current mapping to last mapping (first time this will be all zeros)
    
    for( i = 0; i < x->N2; i++ ){
        last_binsplit[i] = current_binsplit[i];
    }

    for( i = 0; i < N2; i++ ){
        current_binsplit[i] = i;
    }
    max = N2;
    for(i = 0; i < N2; i++){
        swapi = rand() % max;
        tmp = current_binsplit[swapi];
        current_binsplit[swapi] = current_binsplit[max - 1];
        current_binsplit[max - 1] = tmp;
        --max;
    }
    /*
    for(i = 0; i < N2; i++){
        post("i: %d, dex: %d", i, current_binsplit[i]);
    }
   */
    x->counter = 0;
    if(! x->countdown_samps ) { // Ramp Off - Immediately set last to current
        for( i = 0; i < x->N2; i++ ){
            last_binsplit[ i ] = current_binsplit[ i ];
        }
    }
}


void splitspec_setstate (t_splitspec *x, t_symbol *msg, int argc, t_atom *argv) {
    short i;
    
    if( argc != x->N2 ){
        error("list must be of length %d, but actually was %d", x->N2, argc);
        return;
    }
    for( i = 0; i < x->N2; i++ ){
        x->last_binsplit[ i ] = x->current_binsplit[ i ];
        x->current_binsplit[ i ] = 0;
    }
    for (i=0; i < argc; i++) {
        x->current_binsplit[i] = atom_getintarg(i, argc, argv );
		
    }
   // x->frames_left = x->ramp_frames;
    if(!x->counter) { // Ramp Off - Immediately set last to current
        for( i = 0; i < x->N2; i++ ){
            x->last_binsplit[ i ] = x->current_binsplit[ i ];
        }
    }
    
    return;
}

void splitspec_ramptime (t_splitspec *x, t_symbol *msg, int argc, t_atom *argv) {
    float rampdur;
    
 	rampdur = atom_getfloatarg(0,argc,argv) * 0.001; // convert from milliseconds
 	x->countdown_samps = rampdur * x->sr;
 	x->counter = 0;
    //  post("countdown samps :%d", x->countdown_samps  );
}

// REPORT CURRENT SHUFFLE STATUS
void splitspec_showstate (t_splitspec *x ) {
    
    t_atom *list_data = x->list_data;
    
    short i, count;
    
    count = 0;
    // post("showing %d data points", x->N2);
    
    if(! x->initialize){
        for( i = 0; i < x->N2; i++ ) {
            SETFLOAT(list_data+count,(float)x->current_binsplit[i]);
            ++count;
        }
        outlet_list(x->list_outlet,0,x->N2,list_data);
    }
    return;
}

void splitspec_dsp(t_splitspec *x, t_signal **sp)
{
    int i;
    float R, funda;
    int vector_size;
    t_int **sigvec;
    int pointer_count;
    vector_size = sys_getblksize();
    
    pointer_count = (x->channel_count * 2) + 7;
    sigvec = (t_int **) malloc(sizeof(t_int *) * pointer_count);
	for(i = 0; i < pointer_count; i++){
		sigvec[i] = (t_int *) calloc(sizeof(t_int),1);
	}
	sigvec[0] = (t_int *)x; // first pointer is to the object
	sigvec[pointer_count - 1] = (t_int *)sp[0]->s_n; // last pointer is to vector size (N)
	for(i = 1; i < pointer_count - 1; i++){ // now attach the inlet and all outlets
		sigvec[i] = (t_int *)sp[i-1]->s_vec;
	}
    
    /*
     
     FFT size (N) == vector size. (Pd only. Max has a different approach.)
     There is one vector with the magnitudes, and another with the phases, each of size .
     Therefore N2+2 (or the vector size/2  + 1) is the actual number of bins.
     Therefore N2/split is the number of bins assigned to each splitted channel.
     
     */
    // post("vector size %d, sys vector size: %d",vector_size, sys_getblksize() );
    // post("splitspec: samples per vector: %d, sys blocksize %d", sp[0]->s_n, sys_getblksize());
    if( ! sp[0]->s_sr ){
        error("splitspec~: zero sample rate!");
        return;
    }
	
    
    if( x->initialize || x->sr != sys_getsr() || x->N != sp[0]->s_n){
        
        x->sr = sys_getsr();
        x->N = sp[0]->s_n;
        x->N2 = sp[0]->s_n / 2;
 //       post("FFT size is %d, N2 is %d",x->N, x->N2);
        R = sys_getsr();
//        post("sampling rate: %f, vector thinks it is: %f", sys_getsr(), sp[0]->s_sr);
        funda = R / (2. * (float) x->N) ;
        
        if(x->initialize){
            x->list_data = (t_atom *) calloc((x->N + 2),sizeof(t_atom));
            x->last_binsplit = (int *) calloc( x->N2,sizeof(int));
            x->current_binsplit = (int *) calloc( x->N2,sizeof(int));
            x->last_mag = (float *) calloc(x->N2,sizeof(float)) ;
            x->current_mag = (float *) calloc(x->N2,sizeof(float)) ;
            x->stored_slots = (short *) calloc(x->N2,sizeof(short));
            x->stored_binsplits = (int **) calloc(MAXSTORE,sizeof(int *));
            for( i = 0; i < MAXSTORE; i++ ){
                x->stored_binsplits[i] = (int *)calloc(x->N2,sizeof(int));
            }
        } else {
            x->list_data = (t_atom *) realloc((void *)x->list_data,(x->N + 2) * sizeof(t_atom));
            x->last_binsplit = (int *) realloc((void *)x->last_binsplit,x->N2 * sizeof(int));
            x->current_binsplit = (int *) realloc((void *)x->current_binsplit,x->N2 * sizeof(int));
            x->last_mag = (float *) realloc((void *)x->last_mag,x->N2 * sizeof(float));
            x->current_mag = (float *) realloc((void *)x->current_mag,x->N2 * sizeof(float));
            x->stored_slots = (short *) realloc((void *)x->stored_slots,x->N2 * sizeof(short));
            for( i = 0; i < MAXSTORE; i++ ){
                x->stored_binsplits[i] = (int *) realloc((void *)x->stored_binsplits[i],x->N2 * sizeof(int));
            }
            for(i = 0; i < x->N2; i++){
                x->last_mag[i] = 0.0;
                x->current_mag[i] = 0.0;
                x->current_binsplit[i] = i;
                x->last_binsplit[i] = i;
            }
        }
        
        x->frame_duration = (float) sp[0]->s_n / sp[0]->s_sr;
        
        splitspec_scramble( x );
        for( i = 0; i < x->N2; i++ ){
            x->last_binsplit[i] = x->current_binsplit[i];
        }
        
        x->initialize = 0;
        x->counter = 0;
    }
    
    if(vector_size == 0) {
        // post("zero vector size!");
        return;
    } else {
        x->hopsamps = x->N / x->overlap_factor;
//        post("hop samps: %d, overlap: %d", x->hopsamps, x->overlap_factor);
    }
    dsp_addv(splitspec_perform, pointer_count, (t_int *) sigvec);
    free(sigvec);
}


