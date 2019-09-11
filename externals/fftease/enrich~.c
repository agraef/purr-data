/* Pd 32-bit FFTease 3.0 */

/* STILL NEEDS Pd Array code installed */

#include "fftease.h"

static t_class *enrich_class;

#define OBJECT_NAME "enrich~"

typedef struct _enrich
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_symbol *buffername;
	t_float lofreq;
	t_float hifreq;
	int lo_bin;
	int hi_bin;
	t_float topfreq;
	short mute;
    int b_valid;
    long b_frames;
    t_float *b_samples;
} t_enrich;

void enrich_dsp(t_enrich *x, t_signal **sp);
t_int *enrich_perform(t_int *w);
void *enrich_new(t_symbol *s, int argc, t_atom *argv);
void enrich_free(t_enrich *x);
void enrich_mute(t_enrich *x, t_floatarg tog);
void enrich_init(t_enrich *x);
void enrich_lowfreq(t_enrich *x, t_floatarg f);
void enrich_highfreq(t_enrich *x, t_floatarg f);
void enrich_fftinfo(t_enrich *x);
void enrich_setbuf(t_enrich *x, t_symbol *newbufname);
void enrich_dolowfreq(t_enrich *x);
void enrich_dohighfreq(t_enrich *x);
void enrich_binstats(t_enrich *x);
void enrich_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("enrich~"), (t_newmethod)enrich_new,
                  (t_method)enrich_free,sizeof(t_enrich), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_enrich, x_f);
	class_addmethod(c,(t_method)enrich_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)enrich_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)enrich_lowfreq,gensym("lowfreq"),A_FLOAT,0);
    class_addmethod(c,(t_method)enrich_highfreq,gensym("highfreq"),A_FLOAT,0);
    enrich_class = c;
    fftease_announce(OBJECT_NAME);
}

void enrich_mute(t_enrich *x, t_floatarg tog)
{
	x->mute = (short)tog;
}

void enrich_fftsize(t_enrich *x, t_floatarg f)
{	
	t_fftease *fft = x->fft;
	fft->N = (int) f;
	enrich_init(x);
}

void enrich_overlap(t_enrich *x, t_floatarg f)
{

	x->fft->overlap = (int) f;
	enrich_init(x);
}

void enrich_winfac(t_enrich *x, t_floatarg f)
{

	x->fft->winfac = (int) f;
	enrich_init(x);
}

void enrich_fftinfo(t_enrich *x)
{
	fftease_fftinfo( x->fft, OBJECT_NAME );
}

void enrich_free(t_enrich *x ){
	fftease_free(x->fft);
    free(x->fft);
}

void enrich_highfreq(t_enrich *x, t_floatarg f)
{
	t_fftease *fft = x->fft;

	float curfreq;
	
	if(f < x->lofreq){
		error("current minimum is %f",x->lofreq);
		return;
	}
	if(f > fft->R/2 ){
		f = fft->R/2;
	}	
	x->hifreq = f;
	fft->hi_bin = 1;  
	curfreq = 0;
	while(curfreq < x->hifreq) {
		++(fft->hi_bin);
		curfreq += fft->c_fundamental;
	}
}

void enrich_lowfreq(t_enrich *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	float curfreq;
	
	if(f > x->hifreq){
		error("current maximum is %f",x->lofreq);
		return;
	}
	if(f < 0 ){
		f = 0;
	}	
	x->lofreq = f;
	fft->lo_bin = 0;  
	curfreq = 0;
	while( curfreq < x->lofreq ) {
		++(fft->lo_bin);
		curfreq += fft->c_fundamental ;
	}
}



void enrich_dohighfreq(t_enrich *x)
{
	t_fftease *fft = x->fft;
    
	t_float curfreq;
    if( fft->c_fundamental <= 0.0){
        return;
    }
    if(x->hifreq <= 0.0){
        x->hifreq = 100.0;
    }
	fft->hi_bin = 1;
	curfreq = 0.0;
	while(curfreq < x->hifreq) {
		++(fft->hi_bin);
		curfreq += fft->c_fundamental;
	}
}

void enrich_dolowfreq(t_enrich *x)
{
	t_fftease *fft = x->fft;
    if(x->lofreq < 0){
        x->lofreq = 0.0;
    }
    if(x->lofreq >= x->hifreq){
        x->lofreq = 0.0;
    }
	t_float curfreq;
    if( fft->c_fundamental <= 0.0){
        return;
    }
	fft->lo_bin = 0;
	curfreq = 0;
	while( curfreq < x->lofreq ) {
		++(fft->lo_bin);
		curfreq += fft->c_fundamental ;
	}
}



void enrich_init(t_enrich *x)
{
	fftease_init(x->fft);
	fftease_oscbank_setbins(x->fft, x->lofreq, x->hifreq);
}

void enrich_setbuf(t_enrich *x, t_symbol *newbufname)
{
	x->buffername = newbufname;
}

void *enrich_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_enrich *x = (t_enrich *)pd_new(enrich_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));

	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
 	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ x->buffername = atom_getsymbolarg(0, argc, argv); }
    else { post("%s: Must specify array name", OBJECT_NAME); return NULL; }
    if(argc > 1){ fft->N = (int) atom_getfloatarg(1, argc, argv); }
    if(argc > 2){ fft->overlap = (int) atom_getfloatarg(2, argc, argv); }

	return x;
}

void enrich_attachbuf(t_enrich *x)
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

void enrich_binstats(t_enrich *x)
{
    post("lo freq %f hi freq %f lo bin %d hi bin %d",x->lofreq, x->fft->lo_bin,x->hifreq,x->fft->hi_bin);
}

void do_enrich(t_enrich *x)
{
	t_fftease *fft = x->fft;
    enrich_dolowfreq(x);
    enrich_dohighfreq(x);
    fftease_fold(fft);
    fftease_rdft(fft,1);
    fftease_convert(fft);
	fftease_oscbank(fft);
}

t_int *enrich_perform(t_int *w)
{
    t_enrich *x = (t_enrich *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_float *transpose = (t_float *)(w[3]);
	t_float *synt = (t_float *)(w[4]);
	t_float *MSPOutputVector = (t_float *)(w[5]);
	t_fftease *fft = x->fft;
	int i, j;
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
    float *b_samples;
    
    enrich_attachbuf(x);
	if(x->mute || ! x->b_valid){
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
		return w+6;
	}
    
    if(x->b_frames < fft->L){
        post("enrich~: table too small or not mono");
        return w+6;
    }
    b_samples = x->b_samples;
	mult *= fft->N;
	
	// copy buffer to internal table (try more efficient means later)
	for(i = 0; i < fft->L; i++){
		fft->table[i] = b_samples[i];
	}

	
    fft->P  = *transpose;
    fft->synt = *synt;


	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
        
		do_enrich(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
            
			do_enrich(x);
			
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
            
			do_enrich(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}

void enrich_dsp(t_enrich *x, t_signal **sp)
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
        enrich_init(x);
        enrich_dolowfreq(x);
        enrich_dohighfreq(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(enrich_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
    }
}


