#include "MSPd.h"
#include "fftease.h"

#if MSP
void *pvharm_class;
#endif 

#if PD
static t_class *pvharm_class;
#endif

#define OBJECT_NAME "pvharm~"

typedef struct _pvharm
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
	// for convert
	float *c_lastphase_in;
	//  float *c_lastphase_out;
	float c_fundamental;
	float c_factor_in;
	float c_factor_out;
	
	float *table;
	int NP;
	
	int L;
	int first;
	float Iinv;
	
	float ffac;
	// for oscbank 1
	float *lastamp;
	float *lastfreq;
	float *index;
	int lo_bin;
	int hi_bin;
	float topfreq;
	float P;
	float myPInc;
	//
	// for oscbank 2
	float *lastamp2;
	float *lastfreq2;
	float *index2;
	int lo_bin2;
	int hi_bin2;
	float topfreq2;
	float P2;
	float myPInc2;
	//
	
	float synt;
	float myPI;
	float TWOmyPI;
	// for fast fft
	float mult; 
	float *trigland;
	int *bitshuffle;
	short pitch1_connected;
	short pitch2_connected;
	short synt_connected;
	short mute;
	int vs;//vector size
		int overlap;//overlap factor
			int winfac;//window size factor
				float hifreq;//highest frequency to synthesize
					float lofreq;//lowest frequency to synthesize
} t_pvharm;

void *pvharm_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *pvharm_perform(t_int *w);
void pvharm_dsp(t_pvharm *x, t_signal **sp, short *count);
void pvharm_assist(t_pvharm *x, void *b, long m, long a, char *s);
void pvharm_float(t_pvharm *x, double f);
void pvharm_mute(t_pvharm *x, t_floatarg f);
void pvharm_init(t_pvharm *x, short initialized);
void pvharm_lowfreq(t_pvharm *x, t_floatarg f);
void pvharm_highfreq(t_pvharm *x, t_floatarg f);
void pvharm_free(t_pvharm *x);
void pvharm_fftinfo(t_pvharm *x);
void pvharm_overlap(t_pvharm *x, t_floatarg f);
void pvharm_winfac(t_pvharm *x, t_floatarg f);

#if MSP
void main(void)
{
	setup((t_messlist **) &pvharm_class, (method) pvharm_new, (method)pvharm_free, 
		  (short)sizeof(t_pvharm), 0, A_GIMME, 0);
	addmess((method)pvharm_dsp, "dsp", A_CANT, 0);
	addmess((method)pvharm_assist,"assist",A_CANT,0);
	addmess((method)pvharm_mute,"mute",A_DEFFLOAT,0);
	
	addmess((method)pvharm_lowfreq,"lowfreq",A_FLOAT,0);
	addmess((method)pvharm_highfreq,"highfreq",A_FLOAT,0);
	addmess((method)pvharm_overlap,"overlap",A_DEFFLOAT,0);
	addmess((method)pvharm_winfac,"winfac",A_DEFFLOAT,0);
	addmess((method)pvharm_fftinfo,"fftinfo",0);  addfloat((method)pvharm_float);
	dsp_initclass();
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void pvharm_tilde_setup(void)
{
	pvharm_class = class_new(gensym("pvharm~"), (t_newmethod)pvharm_new, 
							 (t_method)pvharm_free ,sizeof(t_pvharm), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(pvharm_class, t_pvharm, x_f);
	class_addmethod(pvharm_class, (t_method)pvharm_dsp, gensym("dsp"), 0);
	class_addmethod(pvharm_class, (t_method)pvharm_assist, gensym("assist"), 0);
	class_addmethod(pvharm_class, (t_method)pvharm_mute, gensym("mute"), A_DEFFLOAT,0);
	class_addmethod(pvharm_class, (t_method)pvharm_highfreq, gensym("highfreq"), A_DEFFLOAT,0);
	class_addmethod(pvharm_class, (t_method)pvharm_lowfreq, gensym("lowfreq"), A_DEFFLOAT,0);
	class_addmethod(pvharm_class,(t_method)pvharm_overlap,gensym("overlap"),A_FLOAT,0);
	class_addmethod(pvharm_class,(t_method)pvharm_winfac,gensym("winfac"),A_FLOAT,0);
	class_addmethod(pvharm_class,(t_method)pvharm_fftinfo,gensym("fftinfo"),0); 
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void pvharm_free(t_pvharm *x)
{
#if MSP
	dsp_free((t_pxobject *) x);
#endif
	freebytes(x->c_lastphase_in,0);
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
	freebytes(x->lastamp2,0);
	freebytes(x->lastfreq,0);
	freebytes(x->lastfreq2,0);
	freebytes(x->index,0);
	freebytes(x->index2,0);
	freebytes(x->table,0);
}

void pvharm_assist (t_pvharm *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
			case 1: sprintf(dst,"(signal/float) Pitch Multiplier 1"); break;
			case 2: sprintf(dst,"(signal/float) Pitch Multiplier 2"); break;
			case 3: sprintf(dst,"(signal/float) Synthesis Threshold"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void pvharm_overlap(t_pvharm *x, t_floatarg f)
{
	int i = (int) f;
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->overlap = i;
	pvharm_init(x,1);
}

void pvharm_winfac(t_pvharm *x, t_floatarg f)
{
	int i = (int)f;
	
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = i;
	pvharm_init(x,2);
}

void pvharm_fftinfo(t_pvharm *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}



void pvharm_highfreq(t_pvharm *x, t_floatarg f)
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

void pvharm_lowfreq(t_pvharm *x, t_floatarg f)
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

void *pvharm_new(t_symbol *s, int argc, t_atom *argv)
{
	//  int i;
#if MSP
	t_pvharm *x = (t_pvharm *)newobject(pvharm_class);
	dsp_setup((t_pxobject *)x,4);
	outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
	t_pvharm *x = (t_pvharm *)pd_new(pvharm_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
#endif
	
	x->vs = sys_getblksize();
	x->R = sys_getsr();
	
	x->lofreq = atom_getfloatarg(0,argc,argv);
	x->hifreq = atom_getfloatarg(1,argc,argv);
	x->overlap = atom_getfloatarg(2,argc,argv);
	x->winfac = atom_getfloatarg(3,argc,argv);
	if(x->lofreq <0 || x->lofreq> 22050)
		x->lofreq = 0;
	if(x->hifreq <50 || x->hifreq> 22050)
		x->hifreq = 4000;
    
	if(!power_of_two(x->overlap))
		x->overlap = 4;
	if(!power_of_two(x->winfac))
		x->winfac = 2;
	
	pvharm_init(x,0);
	return (x);
}

void pvharm_init(t_pvharm *x, short initialized)
{
	int i;
	float curfreq;
	
	x->D = x->vs;
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->inCount = -(x->Nw);
	x->mult = 1. / (float) x->N;
	x->Iinv = 1./x->D;
	x->c_fundamental =  (float) x->R/(float)((x->N2)<<1);
	x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
	x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
    
	if(!initialized){
		x->P = .5 ; // for testing purposes
		x->P2 = .6666666666 ; // for testing purposes
  		x->L = 8192 ;
  		x->synt = .00005;
  		x->mute = 0;
		x->Wanal = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->Wsyn = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->Hwin = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->input = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->output = (float *) getbytes( MAX_Nw * sizeof(float) );
		x->buffer = (float *) getbytes( MAX_N * sizeof(float) );
		x->channel = (float *) getbytes( (MAX_N+2) * sizeof(float) );
		x->bitshuffle = (int *) getbytes( MAX_N * 2 * sizeof( int ) );
		x->trigland = (float *) getbytes( MAX_N * 2 * sizeof( float ) );
		
		x->lastamp = (float *) getbytes((MAX_N+1) * sizeof(float));
		x->lastfreq = (float *) getbytes((MAX_N+1) * sizeof(float));
		x->lastamp2 = (float *) getbytes((MAX_N+1) * sizeof(float));
		x->lastfreq2 = (float *) getbytes((MAX_N+1) * sizeof(float));
		x->index = (float *) getbytes((MAX_N+1) * sizeof(float));
		x->index2 = (float *) getbytes((MAX_N+1) * sizeof(float));
		x->table = (float *) getbytes(x->L * sizeof(float));
		x->c_lastphase_in = (float *) getbytes((MAX_N2+1) * sizeof(float));
	}
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
	memset((char *)x->lastamp,0,(x->N+1) * sizeof(float));
	memset((char *)x->lastfreq,0,(x->N+1) * sizeof(float));
	memset((char *)x->lastamp2,0,(x->N+1) * sizeof(float));
	memset((char *)x->lastfreq2,0,(x->N+1) * sizeof(float));
	memset((char *)x->index,0,(x->N+1) * sizeof(float));
	memset((char *)x->index2,0,(x->N+1) * sizeof(float));
	
	x->myPInc = x->P * x->L/x->R;
	x->myPInc2 = x->P2 * x->L/x->R;
	x->ffac = x->P * PI/x->N;
	if(x->hifreq < x->c_fundamental) {
		x->hifreq = 3000.0 ;
	}
	x->hi_bin = 1;  
	curfreq = 0;
	while(curfreq < x->hifreq) {
		++(x->hi_bin);
		curfreq += x->c_fundamental ;
	}
	if( x->hi_bin >= x->N2 ){
		x->hi_bin = x->N2 - 1;
	}
	x->lo_bin = 0;  
	curfreq = 0;
	while(curfreq < x->lofreq) {
		++(x->lo_bin);
		curfreq += x->c_fundamental ;
	}
	
	for (i = 0; i < x->L; i++) {
		x->table[i] = (float) x->N * cos((float)i * TWOPI / (float)x->L);
	}
	init_rdft( x->N, x->bitshuffle, x->trigland);
	makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
	
}

t_int *pvharm_perform(t_int *w)
{
	int i,j, in,on;
	int    amp,freq,chan;  
	int breaker = 0;
	float    a,ainc,f,finc,address;
	float tmpfreq;
	t_pvharm *x = (t_pvharm *) (w[1]);
	t_float *inbuf = (t_float *)(w[2]);
	t_float *in2 = (t_float *)(w[3]);
	t_float *in3 = (t_float *)(w[4]);
	t_float *in4 = (t_float *)(w[5]);
	t_float *outbuf = (t_float *)(w[6]);
	t_int n = w[7];
	
	int D = x->D;
	int I = D;
	int R = x->R;
	int Nw = x->Nw;
	int N = x->N ;
	int N2 = x-> N2;
	int Nw2 = x->Nw2;
	int L = x->L;
	float fundamental = x->c_fundamental;
	float factor_in =  x->c_factor_in;
	float factor_out = x->c_factor_out;
	int *bitshuffle = x->bitshuffle;
	float *trigland = x->trigland;
	float mult = x->mult;	  		
	float synt = x->synt;
	float P  = x->P; 
	float P2  = x->P2; 
	float Iinv = x->Iinv;
	float myPInc = x->myPInc;
	float myPInc2 = x->myPInc2;
	float *table = x->table;
	float *lastamp = x->lastamp;
	float *lastamp2 = x->lastamp2;
	float *lastfreq = x->lastfreq;
	float *lastfreq2 = x->lastfreq2;
	float *bindex = x->index;
	float *bindex2 = x->index2;
	float *lastphase_in = x->c_lastphase_in;
	//  float *lastphase_out = x->c_lastphase_out;
	float *Wanal = x->Wanal;
	float *Wsyn = x->Wsyn;
	float *input = x->input;
	float *Hwin = x->Hwin;
	float *buffer = x->buffer;
	float *channel = x->channel;
	float *output = x->output;
	int hi_bin = x->hi_bin;
	int lo_bin = x->lo_bin;
	in = on = x->inCount ;
	
	if( x->mute ){
		for ( j = 0; j < n; j++ ){
			*outbuf++ = 0.0;
		}
		return (w+8);
	}
	
	
	if( x->synt_connected ){
		synt = *in4++;
	}
	if( x->pitch1_connected) {
		P = *in2++;
		myPInc = P * x->L/x->R;
	}
	if( x->pitch2_connected) {
		P2 = *in3++;
		myPInc2 = P2 * x->L/x->R;
	}
	in += D;
	on += I;
	
	for ( j = 0 ; j < (Nw - D) ; j++ ){
		input[j] = input[j+D];
	}
	for ( j = (Nw-D), i = 0 ; j < Nw; j++, i++ ) {
		input[j] = *inbuf++;
	}
	
	fold( input, Wanal, Nw, buffer, N, in );
	rdft( N, 1, buffer, bitshuffle, trigland );
	convert( buffer, channel, N2, lastphase_in, fundamental, factor_in );
	
	
	for ( chan = lo_bin; chan < hi_bin; chan++ ) {
		
		freq = ( amp = ( chan << 1 ) ) + 1;
		if ( channel[amp] < synt ){ 
			breaker = 1;
		}
		if( breaker ) {
			breaker = 0 ;
		} else {
			tmpfreq = channel[freq] * myPInc;
			finc = ( tmpfreq - ( f = lastfreq[chan] ) )*Iinv;
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
			lastfreq[chan] = tmpfreq;
			lastamp[chan] = channel[amp];
			bindex[chan] = address;
			// OSC BANK 2
			tmpfreq = channel[freq] * myPInc2;
			finc = ( tmpfreq - ( f = lastfreq2[chan] ) )*Iinv;
			ainc = ( channel[amp] - ( a = lastamp2[chan] ) )*Iinv;
			address = bindex2[chan];
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
			lastfreq2[chan] = tmpfreq;
			lastamp2[chan] = channel[amp];
			bindex2[chan] = address;
			
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
	
	x->P = P;
	x->P2 = P2;
	x->inCount = in;
	return (w+8);
}		

#if MSP
void pvharm_float(t_pvharm *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		x->P = f;
		x->myPInc = x->P*x->L/x->R;
		
    }
	else if (inlet == 2)
    {
		x->P2 = f;
		x->myPInc2 = x->P2*x->L/x->R;
    }
	else if (inlet == 3)
    {
		x->synt = f;
    }
	
}
#endif

void pvharm_mute(t_pvharm *x, t_floatarg state)
{
	x->mute = (short)state;	
}

void pvharm_dsp(t_pvharm *x, t_signal **sp, short *count)
{
#if MSP
	x->pitch1_connected = count[1];
	x->pitch2_connected = count[2];
	x->synt_connected = count[3];
#endif
#if PD
	x->pitch1_connected = 1;
	x->pitch2_connected = 1;
	x->synt_connected = 1;
#endif
	if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr){
		x->vs = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		pvharm_init(x,1);
	}
	//  post("pvharm: sampling rate is %f",sp[0]->s_sr );
	dsp_add(pvharm_perform, 7, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
			sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
}

