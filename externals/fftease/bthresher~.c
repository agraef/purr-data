/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *bthresher_class;

#define OBJECT_NAME "bthresher~"

typedef struct _bthresher
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	/* bthresher vars */
	t_float *move_threshold;
	t_float *composite_frame ;
	int *frames_left;
	int max_hold_frames;
	t_float max_hold_time;
	int first_frame;
	t_float *damping_factor ;
	t_float thresh_scalar;
	t_float damp_scalar;
	short thresh_connected;
	short damping_connected;
	void *list_outlet;
	void *misc_outlet;
	t_atom *list_data;
	short mute;
	short bypass;
	t_float init_thresh;
	t_float init_damping;
	t_float tadv;
	short inf_hold;
} t_bthresher;


void *bthresher_new(t_symbol *s, int argc, t_atom *argv);
void bthresher_dsp(t_bthresher *x, t_signal **sp);
t_int *bthresher_perform(t_int *w);
void bthresher_mute(t_bthresher *x, t_float f);
void bthresher_fftinfo(t_bthresher *x);
void bthresher_free(t_bthresher *x);
void bthresher_bin(t_bthresher *x, t_float bin_num, t_float threshold, t_float damper);
void bthresher_rdamper(t_bthresher *x, t_float min, t_float max );
void bthresher_rthreshold(t_bthresher *x, t_float min, t_float max);
void bthresher_dump(t_bthresher *x );
void bthresher_list (t_bthresher *x, t_symbol *msg, short argc, t_atom *argv);
void bthresher_init(t_bthresher *x);
t_float bthresher_boundrand(t_float min, t_float max);
void bthresher_allthresh(t_bthresher *x, t_float f);
void bthresher_alldamp(t_bthresher *x, t_float f);
void bthresher_inf_hold(t_bthresher *x, t_float f);
void bthresher_max_hold(t_bthresher *x, t_float f);
void do_bthresher(t_bthresher *x);
void bthresher_oscbank(t_bthresher *x, t_float flag);
void bthresher_synthresh(t_bthresher *x, t_float thresh);
void bthresher_transpose(t_bthresher *x, t_float tf);

void bthresher_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("bthresher~"), (t_newmethod)bthresher_new,
                  (t_method)bthresher_free,sizeof(t_bthresher), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_bthresher, x_f);
	class_addmethod(c,(t_method)bthresher_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)bthresher_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)bthresher_fftinfo,gensym("fftinfo"),0);
	class_addmethod(c,(t_method)bthresher_oscbank,gensym("oscbank"),A_FLOAT,0);
	class_addmethod(c,(t_method)bthresher_transpose,gensym("transpose"),A_FLOAT,0);
	class_addmethod(c,(t_method)bthresher_synthresh,gensym("synthresh"),A_FLOAT,0);
	class_addmethod(c,(t_method)bthresher_bin, gensym("bin"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
	class_addmethod(c,(t_method)bthresher_rdamper, gensym("rdamper"), A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(c,(t_method)bthresher_rthreshold, gensym("rthreshold"), A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(c,(t_method)bthresher_dump,gensym("dump"),0);
	class_addmethod(c,(t_method)bthresher_list,gensym("list"),A_GIMME,0);
	class_addmethod(c,(t_method)bthresher_alldamp,gensym("alldamp"),A_FLOAT,0);
	class_addmethod(c,(t_method)bthresher_allthresh,gensym("allthresh"),A_FLOAT,0);
	class_addmethod(c,(t_method)bthresher_inf_hold,gensym("inf_hold"),A_FLOAT,0);
	class_addmethod(c,(t_method)bthresher_max_hold,gensym("max_hold"),A_FLOAT,0);
    bthresher_class = c;
    fftease_announce(OBJECT_NAME);
}


void bthresher_fftinfo( t_bthresher *x )
{
	fftease_fftinfo( x->fft, OBJECT_NAME );
}

void bthresher_free( t_bthresher *x ){
	t_fftease *fft = x->fft;
	fftease_free(fft);
	/* external-specific memory */
	free(x->composite_frame);
	free(x->frames_left);
	free(x->move_threshold);
	free(x->damping_factor);
	free(x->list_data);
}

void bthresher_max_hold(t_bthresher *x, t_float f)
{
	if(f<=0)
		return;
	x->max_hold_time = f * .001;
	x->max_hold_frames = x->max_hold_time / x->tadv;
}

void bthresher_inf_hold(t_bthresher *x, t_float f)
{
	x->inf_hold = (int)f;
}

void bthresher_allthresh(t_bthresher *x, t_float f)
{
	int i;
	t_fftease *fft = x->fft;
	//post("thresh %f",f);
	for(i=0;i < fft->N2+1;i++)
		x->move_threshold[i] = f;
}

void bthresher_alldamp(t_bthresher *x, t_float f)
{
	int i;
	t_fftease *fft = x->fft;
    
	//post("damp %f",f);
	for(i=0;i < fft->N2+1;i++)
		x->damping_factor[i] = f;
}

void bthresher_mute(t_bthresher *x, t_float f){
	x->mute = f;
}


void bthresher_list (t_bthresher *x, t_symbol *msg, short argc, t_atom *argv) {
	int i, bin, idiv;
	t_float fdiv;
	t_float *damping_factor = x->damping_factor;
	t_float *move_threshold = x->move_threshold;
	
	//	post("reading %d elements", argc);
	idiv = fdiv = (t_float) argc / 3.0 ;
	if( fdiv - idiv > 0.0 ) {
		post("list must be in triplets");
		return;
	}
	
	for( i = 0; i < argc; i += 3 ) {
		bin = atom_getintarg(i,argc,argv);
		damping_factor[bin] = atom_getfloatarg(i+1,argc,argv);
		move_threshold[bin] = atom_getfloatarg(i+2,argc,argv);
		
	}
}

void bthresher_dump (t_bthresher *x) {
	
	t_atom *list_data = x->list_data;
	t_float *damping_factor = x->damping_factor;
	t_float *move_threshold = x->move_threshold;
	
	int i,j, count;
    
	for( i = 0, j = 0; i < x->fft->N2 * 3 ; i += 3, j++ ) {
        
		SETFLOAT(list_data+i,(t_float)j);
		SETFLOAT(list_data+(i+1),damping_factor[j]);
		SETFLOAT(list_data+(i+2),move_threshold[j]);
	}
	
	count = x->fft->N2 * 3;
	outlet_list(x->list_outlet,0,count,list_data);
	
	return;
}

void *bthresher_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fftease *fft;
    
	t_bthresher *x = (t_bthresher *)pd_new(bthresher_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->list_outlet = outlet_new(&x->x_obj, gensym("list"));
    
	
	x->fft = (t_fftease *) calloc( 1, sizeof(t_fftease) );
	
	fft = x->fft;
	fft->initialized = 0;
	x->init_thresh = 0.1;
	x->init_damping = 0.99;
    
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
    
	return x;
}



void bthresher_transpose(t_bthresher *x, t_float tf)
{
	x->fft->P = (t_float) tf;
}

void bthresher_synthresh(t_bthresher *x, t_float thresh)
{
	x->fft->synt = (t_float) thresh;
}

void bthresher_init(t_bthresher *x)
{
	int i;
	t_fftease *fft = x->fft;
    
	short initialized = fft->initialized;
	fftease_init(fft);
	
	if(!initialized){
		x->first_frame = 1;
		x->max_hold_time = 60.0 ;
		x->thresh_connected = 0;
		x->damping_connected = 0;
		x->thresh_scalar = 1;
		x->damp_scalar = 1;
		x->mute = 0;
		x->bypass = 0;
		x->inf_hold = 0;
		x->composite_frame = (t_float *) calloc(fft->N + 2, sizeof(t_float));
		x->frames_left = (int *) calloc(fft->N + 2, sizeof(int));
		
		
		// TRIPLETS OF bin# damp_factor threshold
		x->list_data = (t_atom *) calloc((fft->N2 + 1) * 3, sizeof(t_atom));
		x->move_threshold = (t_float *) calloc((fft->N2+1), sizeof(t_float));
		x->damping_factor = (t_float *) calloc((fft->N2+1), sizeof(t_float));
        
		for(i = 0; i < fft->N2+1; i++) {
			x->move_threshold[i] = x->init_thresh;
			x->damping_factor[i] = x->init_damping;
		}
	} else {
		x->list_data = (t_atom *) realloc((void *)x->list_data, (fft->N2 + 1) * 3 * sizeof(t_atom));
		x->move_threshold = (t_float *) realloc((void *)x->move_threshold, (fft->N2+1) * sizeof(t_float));
		x->damping_factor = (t_float *) realloc((void *)x->damping_factor, (fft->N2+1) * sizeof(t_float));
	}
    // NEED TO HANDLE REALLOC CASE!!!
    
	x->tadv = (t_float) fft->D / (t_float) fft->R;
	x->max_hold_frames = x->max_hold_time / x->tadv;
}

void bthresher_version(void)
{
	fftease_version(OBJECT_NAME);
}

void bthresher_rdamper(t_bthresher *x,  t_float min, t_float max)
{
	int i;
	
	for( i = 0; i < x->fft->N2; i++ ) {
		x->damping_factor[i] = bthresher_boundrand(min, max);
	}
}

void bthresher_rthreshold( t_bthresher *x,  t_float min, t_float max )
{
	int i;
	for( i = 0; i < x->fft->N2; i++ ) {
		x->move_threshold[i] = bthresher_boundrand(min, max);
	}
}


void bthresher_bin(t_bthresher *x, t_float bin_num, t_float damper, t_float threshold)
{
	int bn = (int) bin_num;
	if( bn >= 0 && bn < x->fft->N2 ){
		//    post("setting %d to %f %f",bn,threshold,damper);
		x->move_threshold[bn] = threshold;
		x->damping_factor[bn] = damper;
	} else {
		post("bthresher~: %d is out of range", bn);
	}
}


void do_bthresher(t_bthresher *x)
{
	t_fftease *fft = x->fft;
    
	int N = fft->N;
    
	t_float *channel = fft->channel;
	t_float *damping_factor = x->damping_factor;
	t_float *move_threshold = x->move_threshold;
	t_float *composite_frame = x->composite_frame;
	int max_hold_frames = x->max_hold_frames;
	int *frames_left = x->frames_left;
	t_float thresh_scalar = x->thresh_scalar;
	t_float damp_scalar = x->damp_scalar;
	short inf_hold = x->inf_hold;
	int i, j;
	
	fftease_fold(fft);
	fftease_rdft(fft,1);
	fftease_convert(fft);
	if( x->first_frame ){
		for ( i = 0; i < N+2; i++ ){
			composite_frame[i] = channel[i];
			x->frames_left[i] = max_hold_frames;
		}
		x->first_frame = 0;
	}
	else {
		if( thresh_scalar < .999 || thresh_scalar > 1.001 || damp_scalar < .999 || damp_scalar > 1.001 ) {
			for(i = 0, j = 0; i < N+2; i += 2, j++ ){
				if( fabs( composite_frame[i] - channel[i] ) > move_threshold[j] * thresh_scalar|| frames_left[j] <= 0 ){
					composite_frame[i] = channel[i];
					composite_frame[i+1] = channel[i+1];
					frames_left[j] = max_hold_frames;
				}
				else {
					if(!inf_hold){
						--(frames_left[j]);
					}
					composite_frame[i] *= damping_factor[j] * damp_scalar; /* denormals protection */
					if( composite_frame[i] < 0.000001 )
						composite_frame[i] = 0.0;
				}
			}
			
		}
		else {
			for( i = 0, j = 0; i < N+2; i += 2, j++ ){
				if( fabs( composite_frame[i] - channel[i] ) > move_threshold[j] || frames_left[j] <= 0 ){
					composite_frame[i] = channel[i];
					composite_frame[i+1] = channel[i+1];
					frames_left[j] = max_hold_frames;
				} else {
					if(!inf_hold){
						--(frames_left[j]);
					}
					// composite_frame[i] *= damping_factor[j]; // was a bug here ??
					composite_frame[i] *= damping_factor[j] * damp_scalar;
					if( composite_frame[i] < 0.000001 )  /* denormals protection */
						composite_frame[i] = 0.0;
				}
			}
		}
	}
    // use memcopy
	for(i = 0; i < N+2; i++){
		channel[i] = composite_frame[i];
	}
	if(fft->obank_flag){
		fftease_oscbank(fft);
	} else {
		fftease_unconvert(fft);
		fftease_rdft(fft,-1);
		fftease_overlapadd(fft);
	}
}

void bthresher_oscbank(t_bthresher *x, t_float flag)
{
	x->fft->obank_flag = (short) flag;
}

t_int *bthresher_perform(t_int *w)
{
	int	i,j;
    t_bthresher *x = (t_bthresher *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *inthresh = (t_float *)(w[3]);
	t_float *damping = (t_float *)(w[4]);
	t_float *MSPOutputVector = (t_float *)(w[5]);
	t_fftease *fft = x->fft;
	
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	t_float *internalInputVector = fft->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;
	t_float *input = fft->input;
	t_float *output = fft->output;
	int Nw = fft->Nw;
	t_float mult = fft->mult;
	int D = fft->D;
	
	if(x->mute) {
        for(i=0; i < MSPVectorSize; i++){
            MSPOutputVector[i] = 0.0;
        }
        return w+6;
	}
    
    x->thresh_scalar = *inthresh;
    x->damp_scalar = *damping;
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_bthresher(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
    }
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_bthresher(x);
			
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
            
			do_bthresher(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}


void bthresher_dsp(t_bthresher *x, t_signal **sp)
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
        bthresher_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(bthresher_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
    }
}

t_float bthresher_boundrand( t_float min, t_float max) {
	t_float frand;
	frand = (t_float) (rand() % 32768)/ 32768.0;
	return (min + frand * (max-min) );
}

