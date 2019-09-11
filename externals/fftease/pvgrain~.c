/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *pvgrain_class;

#define OBJECT_NAME "pvgrain~"

typedef struct _pvgrain
{
    t_object x_obj;
    t_float x_f;
	t_fftease *fft;
	short *binsort;
	t_float topfreq;
	t_float bottomfreq;
	short bypass;
	int mute;
	t_float grain_probability;
	t_float sample_basefreq;
	int grains_per_frame;
	void *list_outlet;
	t_float *listdata;
	short list_count;
	void *m_clock;
	t_atom myList[2];

} t_pvgrain;

void pvgrain_dsp(t_pvgrain *x, t_signal **sp);
t_int *pvgrain_perform(t_int *w);
void *pvgrain_new(t_symbol *s, int argc, t_atom *argv);
void pvgrain_mute(t_pvgrain *x, t_floatarg state);
void pvgrain_tick(t_pvgrain *x);
void pvgrain_printchan(t_pvgrain *x);
void pvgrain_probability (t_pvgrain *x, t_floatarg prob);
void pvgrain_framegrains (t_pvgrain *x, t_floatarg grains);
void pvgrain_topfreq (t_pvgrain *x, t_floatarg top);
void pvgrain_bottomfreq (t_pvgrain *x, t_floatarg f);
void pvgrain_basefreq (t_pvgrain *x, t_floatarg base);
float pvgrain_randf(float min, float max);
void pvgrain_init(t_pvgrain *x);
void pvgrain_free(t_pvgrain *x);

void pvgrain_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("pvgrain~"), (t_newmethod)pvgrain_new,
                  (t_method)pvgrain_free,sizeof(t_pvgrain), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_pvgrain, x_f);
	class_addmethod(c,(t_method)pvgrain_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)pvgrain_mute,gensym("mute"),A_FLOAT,0);

    class_addmethod(c,(t_method)pvgrain_printchan,gensym("printchan"),A_DEFFLOAT,0);
	class_addmethod(c,(t_method)pvgrain_probability,gensym("probability"),A_DEFFLOAT,0);
	class_addmethod(c,(t_method)pvgrain_framegrains,gensym("framegrains"),A_DEFFLOAT,0);
	class_addmethod(c,(t_method)pvgrain_topfreq,gensym("topfreq"),A_FLOAT,0);
	class_addmethod(c,(t_method)pvgrain_basefreq,gensym("basefreq"),A_FLOAT,0);
    pvgrain_class = c;
    fftease_announce(OBJECT_NAME);
}
void pvgrain_printchan(t_pvgrain *x)
{
	int i;
	t_float *channel = x->fft->channel;
	post("***");
	for( i = 0 ; i < 30; i+= 2 ){
		post("amp %f freq %f", channel[i*2], channel[i*2 + 1]);
	}
	post("***");
}

void pvgrain_basefreq (t_pvgrain *x, t_floatarg base)
{
	int R = x->fft->R;
	if( base < 0.0 )
		base = 0. ;
	if( base > R / 2 )
		base = R / 2 ;
	x->sample_basefreq = base;
}

void pvgrain_topfreq (t_pvgrain *x, t_floatarg top)
{
	int R = x->fft->R;
	t_fftease *fft = x->fft;

	if( top < 50. )
		top = 50.;
	if( top > R / 2 )
		top = R / 2;
	x->topfreq = top ;
	fftease_oscbank_setbins(fft,x->bottomfreq, x->topfreq);
}

void pvgrain_bottomfreq (t_pvgrain *x, t_floatarg f)
{
	int R = x->fft->R;
	t_fftease *fft = x->fft;
	if( f >= x->topfreq || f >= R/2){
		post("%f is too high a bottom freq",f);
		return;
	}
	
	x->bottomfreq = f;
	fftease_oscbank_setbins(fft,x->bottomfreq, x->topfreq);
}

void pvgrain_probability (t_pvgrain *x, t_floatarg prob)
{
	if( prob < 0. )
		prob = 0.;
	if( prob > 1. )
		prob = 1.;
	x->grain_probability = prob ;
}

void pvgrain_framegrains (t_pvgrain *x, t_floatarg grains)
{
	int N2 = x->fft->N2;
	if( grains < 1 )
		grains = 1;
	if( grains > N2 - 1 )
		grains = N2 - 1;
	x->grains_per_frame = grains ;
}

void pvgrain_tick(t_pvgrain *x)
{
	t_atom *myList = x->myList;
	t_float *listdata = x->listdata;
	int i;
	
	for (i=0; i < 2; i++) {
		SETFLOAT(myList+i,listdata[i]); /* macro for setting a t_atom */
	}
	outlet_list(x->list_outlet,0,2,myList);
}

void *pvgrain_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_pvgrain *x = (t_pvgrain *)pd_new(pvgrain_class);

    x->list_outlet = outlet_new(&x->x_obj, gensym("list"));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	
	x->grain_probability = 0.0001;
	x->topfreq = 1000.0;

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;

    if(argc > 0){ fft->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void pvgrain_init(t_pvgrain *x)
{
	t_fftease  *fft = x->fft;
	short initialized = fft->initialized;
	fftease_init(fft);
	
	if(!initialized){
        x->m_clock = clock_new(x,(void *)pvgrain_tick);
		x->sample_basefreq = 261.0;
		x->bottomfreq = 0.0;
		x->mute = 0;
		x->binsort = (short *) calloc((fft->N2+1), sizeof(short) );
		x->listdata = (t_float *) calloc(40, sizeof(t_float));
	} else if(initialized == 1) {
		x->binsort = (short *) realloc(x->binsort,(fft->N2+1) * sizeof(short));
	}
	fftease_oscbank_setbins(fft,x->bottomfreq, x->topfreq);
}

void pvgrain_free(t_pvgrain *x)
{
    if(x->fft->initialized){
        free(x->binsort);
        free(x->listdata);
    }
	fftease_free(x->fft);
    free(x->fft);
}

void do_pvgrain(t_pvgrain *x)
{
	int 	i,j;
	t_float tmp, dice;
	short print_grain;
	t_fftease *fft = x->fft;
	t_float *channel = fft->channel;
	short *binsort = x->binsort;
	int grains_per_frame = x->grains_per_frame ;
	t_float selection_probability = x->grain_probability;
	int hi_bin = fft->hi_bin;
	int lo_bin = fft->lo_bin;
	t_float *listdata = x->listdata;
		
	x->list_count = 0;
	
	fftease_fold(fft);
	fftease_rdft(fft,FFT_FORWARD);
	fftease_convert(fft);
	if( grains_per_frame > hi_bin - lo_bin )
		grains_per_frame = hi_bin - lo_bin;
	//  binsort[0] = 0;
	for( i = 0; i < hi_bin; i++ ){// could be hi_bin - lo_bin
		binsort[i] = i + lo_bin;
	}
	for( i = lo_bin; i < hi_bin - 1; i++ ){
		for( j = i+1; j < hi_bin; j++ ){
			if(channel[binsort[j] * 2] > channel[binsort[i] * 2]) {
				tmp = binsort[j];
				binsort[j] = binsort[i];
				binsort[i] = tmp;
			}
		}
	}
	for( i = 0; i < grains_per_frame; i++ ){
		print_grain = 1;
		dice = fftease_randf(0.,1.);
		if( dice < 0.0 || dice > 1.0 ){
			error("dice %f out of range", dice);
		}
		if( selection_probability < 1.0 ){
			if( dice > selection_probability) {
				print_grain = 0;
			} 
		}
		if( print_grain ){
			listdata[ x->list_count * 2 ] = channel[ binsort[i]*2 ];
			listdata[ (x->list_count * 2) + 1 ] = channel[(binsort[i]*2) + 1] ;
			++(x->list_count);
			clock_delay(x->m_clock,0); 
		}
	}
}

t_int *pvgrain_perform(t_int *w)
{
	int 	i;
    t_pvgrain *x = (t_pvgrain *) (w[1]);
	t_float *MSPInputVector = (t_float *)(w[2]);
	t_fftease *fft = x->fft;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float *input = fft->input;
	int MSPVectorSize = fft->MSPVectorSize;
	t_float *internalInputVector = fft->internalInputVector;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;

	if (x->mute) {
		return w+3;
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(input, input + D, (Nw - D) * sizeof(t_float));
        memcpy(input + (Nw - D), MSPInputVector, D * sizeof(t_float));
		do_pvgrain(x);
	}	
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw-D), MSPInputVector + (D*i), D * sizeof(t_float));
			do_pvgrain(x);
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        memcpy(internalInputVector + (operationCount * MSPVectorSize), MSPInputVector,MSPVectorSize * sizeof(t_float));
		operationCount = (operationCount + 1) % operationRepeat;
		
		if( operationCount == 0 ) {
            memcpy(input, input + D, (Nw - D) * sizeof(t_float));
            memcpy(input + (Nw - D), internalInputVector, D * sizeof(t_float));
			do_pvgrain(x);
		}
		fft->operationCount = operationCount;
	}
	return w+3;
}	

void pvgrain_mute(t_pvgrain *x, t_floatarg state)
{
	x->mute = (short)state;	
}

void pvgrain_dsp(t_pvgrain *x, t_signal **sp)
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
        pvgrain_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(pvgrain_perform, 2, x, sp[0]->s_vec);
    }
}
