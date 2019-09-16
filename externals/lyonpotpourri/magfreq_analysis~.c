#include "MSPd.h"
#include "fftease.h"

static t_class *magfreq_analysis_class;

#define OBJECT_NAME "magfreq_analysis~"

typedef struct _magfreq_analysis
{
    
	t_object x_obj;
	float x_f;
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
	float *index;
	float *table;
	float myPInc;
	float ffac;
	//
	float lofreq;
	float hifreq;
	int lo_bin;
	int hi_bin;
	float topfreq;
	float synt;
	// for fast fft
	float mult;
	float *trigland;
	int *bitshuffle;
	//
	int bypass_state;
	int pitch_connected;
	int synt_connected;
	int overlap;
	int winfac;
	short mute;
} t_magfreq_analysis;

void *magfreq_analysis_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *magfreq_analysis_perform(t_int *w);
void magfreq_analysis_dsp(t_magfreq_analysis *x, t_signal **sp);
void magfreq_analysis_bypass(t_magfreq_analysis *x, t_floatarg state);//Pd does not accept A_LONG
void magfreq_analysis_float(t_magfreq_analysis *x, double f);
void magfreq_analysis_free(t_magfreq_analysis *x);
void magfreq_analysis_mute(t_magfreq_analysis *x, t_floatarg tog);
void magfreq_analysis_init(t_magfreq_analysis *x, short initialized);
void magfreq_analysis_lowfreq(t_magfreq_analysis *x, t_floatarg f);
void magfreq_analysis_highfreq(t_magfreq_analysis *x, t_floatarg f);
void magfreq_analysis_overlap(t_magfreq_analysis *x, t_floatarg o);
void magfreq_analysis_winfac(t_magfreq_analysis *x, t_floatarg f);
void magfreq_analysis_fftinfo(t_magfreq_analysis *x);;


void magfreq_analysis_tilde_setup(void)
{
	magfreq_analysis_class = class_new(gensym("magfreq_analysis~"), (t_newmethod)magfreq_analysis_new,
                                       (t_method)magfreq_analysis_free ,sizeof(t_magfreq_analysis), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(magfreq_analysis_class, t_magfreq_analysis, x_f);
	class_addmethod(magfreq_analysis_class, (t_method)magfreq_analysis_dsp, gensym("dsp"), 0);
	class_addmethod(magfreq_analysis_class, (t_method)magfreq_analysis_mute, gensym("mute"), A_DEFFLOAT,0);
	class_addmethod(magfreq_analysis_class, (t_method)magfreq_analysis_bypass, gensym("bypass"), A_DEFFLOAT,0);
	class_addmethod(magfreq_analysis_class, (t_method)magfreq_analysis_highfreq, gensym("highfreq"), A_DEFFLOAT,0);
	class_addmethod(magfreq_analysis_class, (t_method)magfreq_analysis_lowfreq, gensym("lowfreq"), A_DEFFLOAT,0);
	class_addmethod(magfreq_analysis_class, (t_method)magfreq_analysis_overlap, gensym("overlap"), A_DEFFLOAT,0);
	class_addmethod(magfreq_analysis_class, (t_method)magfreq_analysis_winfac, gensym("winfac"), A_DEFFLOAT,0);
	class_addmethod(magfreq_analysis_class, (t_method)magfreq_analysis_fftinfo, gensym("fftinfo"),0);
	potpourri_announce(OBJECT_NAME);
}


void magfreq_analysis_mute(t_magfreq_analysis *x, t_floatarg tog)
{
	x->mute = (short)tog;
}

void magfreq_analysis_overlap(t_magfreq_analysis *x, t_floatarg f)
{
	int i = (int) f;
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->overlap = i;
	magfreq_analysis_init(x,1);
}

void magfreq_analysis_winfac(t_magfreq_analysis *x, t_floatarg f)
{
	int i = (int)f;
	
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = i;
	magfreq_analysis_init(x,2);
}

void magfreq_analysis_fftinfo(t_magfreq_analysis *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void magfreq_analysis_free(t_magfreq_analysis *x ){
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
	freebytes(x->index,0);
	freebytes(x->table,0);
}

void magfreq_analysis_highfreq(t_magfreq_analysis *x, t_floatarg f)
{
	float curfreq;
	
	if(f < x->lofreq){
		error("current minimum is %f",x->lofreq);
		return;
	}
	if(f > x->R/2 ){
		f = x->R/2;
	}
	x->hifreq = f;
	x->hi_bin = 1;
	curfreq = 0;
	while(curfreq < x->hifreq) {
		++(x->hi_bin);
		curfreq += x->c_fundamental;
	}
}

void magfreq_analysis_lowfreq(t_magfreq_analysis *x, t_floatarg f)
{
	float curfreq;
	
	if(f > x->hifreq){
		error("current maximum is %f",x->lofreq);
		return;
	}
	if(f < 0 ){
		f = 0;
	}
	x->lofreq = f;
	x->lo_bin = 0;
	curfreq = 0;
	while( curfreq < x->lofreq ) {
		++(x->lo_bin);
		curfreq += x->c_fundamental ;
	}
}


void magfreq_analysis_init(t_magfreq_analysis *x, short initialized)
{
	int i;
	float curfreq;
	
	if(!x->R)//temp init if MSP functions returned zero
		x->R = 44100;
	if(!x->D)
		x->D = 256;
	if(x->P <= 0)
		x->P = 1.0;
	if(!power_of_two(x->overlap))
		x->overlap = 2;
	if(!power_of_two(x->winfac))
		x->winfac = 2;
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	// limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = x->N / 2;
	x->Nw2 = x->Nw / 2;
	x->inCount = -(x->Nw);
	x->bypass_state = 0;
	x->mult = 1. / (float) x->N;
	x->pitch_connected = 0;
	x->synt_connected = 0;
    
	x->L = 8192 ;
	x->c_fundamental =  (float) x->R/(float)( (x->N2)<<1 );
	x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
	x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
	x->Iinv = 1./x->D;
	x->myPInc = x->P*x->L/x->R;
	x->ffac = x->P * PI/x->N;
	
	if(!initialized){
		x->Wanal = (float *) getbytes( (MAX_Nw) * sizeof(float));
		x->Wsyn = (float *) getbytes( (MAX_Nw) * sizeof(float));
		x->Hwin = (float *) getbytes( (MAX_Nw) * sizeof(float));
		x->input = (float *) getbytes( MAX_Nw * sizeof(float) );
		x->output = (float *) getbytes( MAX_Nw * sizeof(float) );
		x->buffer = (float *) getbytes( MAX_N * sizeof(float) );
		x->channel = (float *) getbytes( (MAX_N+2) * sizeof(float) );
		x->bitshuffle = (int *) getbytes( MAX_N * 2 * sizeof( int ) );
		x->trigland = (float *) getbytes( MAX_N * 2 * sizeof( float ) );
		x->c_lastphase_in = (float *) getbytes( (MAX_N2+1) * sizeof(float) );
		x->c_lastphase_out = (float *) getbytes( (MAX_N2+1) * sizeof(float) );
		x->lastamp = (float *) getbytes( (MAX_N+1) * sizeof(float) );
		x->lastfreq = (float *) getbytes( (MAX_N+1) * sizeof(float) );
		x->index = (float *) getbytes( (MAX_N+1) * sizeof(float) );
		x->table = (float *) getbytes( x->L * sizeof(float) );
		x->P = 1.0;
		x->ffac = x->P * PI/MAX_N;
		x->mute = 0;
        //		x->threshgen = .0001;
        
	}
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
	memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));
	memset((char *)x->lastamp,0,(x->N+1) * sizeof(float));
	memset((char *)x->lastfreq,0,(x->N+1) * sizeof(float));
	memset((char *)x->index,0,(x->N+1) * sizeof(float));
    
	for ( i = 0; i < x->L; i++ ) {
		x->table[i] = (float) x->N * cos((float)i * TWOPI / (float)x->L);
	}
	init_rdft( x->N, x->bitshuffle, x->trigland);
	makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
	
	if( x->hifreq < x->c_fundamental ) {
		x->hifreq = 3000.0 ;
	}
	x->hi_bin = 1;
	curfreq = 0;
	while( curfreq < x->hifreq ) {
		++(x->hi_bin);
		curfreq += x->c_fundamental ;
	}
	
	x->lo_bin = 0;
	curfreq = 0;
	while( curfreq < x->lofreq ) {
		++(x->lo_bin);
		curfreq += x->c_fundamental ;
	}
	
}

void *magfreq_analysis_new(t_symbol *s, int argc, t_atom *argv)
{

	t_magfreq_analysis *x = (t_magfreq_analysis *)pd_new(magfreq_analysis_class);
	outlet_new(&x->x_obj, gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
	
	x->lofreq = atom_getfloatarg(0,argc,argv);
	x->hifreq = atom_getfloatarg(1,argc,argv);
	x->overlap = atom_getfloatarg(2,argc,argv);
	x->winfac = atom_getfloatarg(3,argc,argv);
	
	if(x->lofreq <0 || x->lofreq> 22050)
		x->lofreq = 0;
	if(x->hifreq <50 || x->hifreq> 22050)
		x->hifreq = 4000;
    
    
	if(!power_of_two(x->overlap)){
		x->overlap = 4;
	}
	if(!power_of_two(x->winfac)){
		x->winfac = 2;
	}
	x->R = sys_getsr();
	x->D = sys_getblksize();
	
	magfreq_analysis_init(x,0);
	
	return (x);
}

t_int *magfreq_analysis_perform(t_int *w)
{
	int 	j, in,on;
	int    amp,freq,chan;
	
	t_magfreq_analysis *x = (t_magfreq_analysis *) (w[1]);
	t_float *inbuf = (t_float *)(w[2]);
	t_float *magnitude_vec = (t_float *)(w[3]);
	t_float *frequency_vec = (t_float *)(w[4]);
	t_float *index_vec = (t_float *)(w[5]);
	int n = (int) w[6];
	
	int D = x->D;
	int Nw = x->Nw;
	int N = x->N ;
	int N2 = x-> N2;
	float fundamental = x->c_fundamental;
	float factor_in =  x->c_factor_in;
	int *bitshuffle = x->bitshuffle;
	float *trigland = x->trigland;
	float *lastphase_in = x->c_lastphase_in;
    
	
	float *Wanal = x->Wanal;
	float *input = x->input;;
	float *buffer = x->buffer;
	float *channel = x->channel;
	in = on = x->inCount ;
	
	
	if(x->mute){
		for( j = 0; j < n; j++ ) {
			*magnitude_vec++ = 0;
			*frequency_vec++ = 0;
			*index_vec++ = j;
		}
		return w+7;
	}
    
	
	if (x->bypass_state) {
		for( j = 0; j < n; j++ ) {
			*magnitude_vec++ = 0;
			*frequency_vec++ = 0;
			*index_vec++ = 0;
		}
		return w+7;
	}
	
    in = on = x->inCount ;
	
    in += D;
    //    on += I;
	
    for ( j = 0 ; j < (Nw - D) ; j++ ){
        input[j] = input[j+D];
    }
    for ( j = (Nw-D); j < Nw; j++) {
        input[j] = *inbuf++;
    }
	
    fold( input, Wanal, Nw, buffer, N, in );
    rdft( N, 1, buffer, bitshuffle, trigland );
    convert( buffer, channel, N2, lastphase_in, fundamental, factor_in );
	
	
    // start osc bank
	
    for ( chan = 0; chan < n; chan++ ) {
		
        freq = ( amp = ( chan << 1 ) ) + 1;
        frequency_vec[chan] = channel[freq];
        magnitude_vec[chan] = channel[amp];
        
    	index_vec[chan] = chan;
	}
    
	
	
    // restore state variables
    x->inCount = in % Nw;
    return w+7;
}

void magfreq_analysis_bypass(t_magfreq_analysis *x, t_floatarg state)
{
	x->bypass_state = state;
}

#if __MSP__
void magfreq_analysis_float(t_magfreq_analysis *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		/*x->P = (float)f;
         x->myPInc = x->P*x->L/x->R;*/
    }
	else if (inlet == 2)
    {
		//	x->threshgen = (float)f;
    }
	
}
#endif


void magfreq_analysis_dsp(t_magfreq_analysis *x, t_signal **sp)
{
	
	if(x->D != sp[0]->s_n || x->R != sp[0]->s_sr ){
		x->D = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		magfreq_analysis_init(x,1);
	}
	
	dsp_add(magfreq_analysis_perform, 6, x,
			sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
			(t_int)sp[0]->s_n);
}

