#include "MSPd.h"
#include "fftease.h"

#if MSP
void *pvoc_class;
#endif 

#if PD
static t_class *pvoc_class;
#endif

#define OBJECT_NAME "pvoc~"

typedef struct _pvoc
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
} t_pvoc;

void *pvoc_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *pvoc_perform(t_int *w);
void pvoc_dsp(t_pvoc *x, t_signal **sp, short *count);
void pvoc_assist(t_pvoc *x, void *b, long m, long a, char *s);
void pvoc_bypass(t_pvoc *x, t_floatarg state);//Pd does not accept A_LONG
void pvoc_float(t_pvoc *x, double f);
void pvoc_free(t_pvoc *x);
void pvoc_mute(t_pvoc *x, t_floatarg tog);
void pvoc_init(t_pvoc *x, short initialized);
void pvoc_lowfreq(t_pvoc *x, t_floatarg f);
void pvoc_highfreq(t_pvoc *x, t_floatarg f);
void pvoc_overlap(t_pvoc *x, t_floatarg o);
void pvoc_winfac(t_pvoc *x, t_floatarg f);
void pvoc_fftinfo(t_pvoc *x);;

#if PD
/* Pd Initialization */
void pvoc_tilde_setup(void)
{
	pvoc_class = class_new(gensym("pvoc~"), (t_newmethod)pvoc_new, 
						   (t_method)pvoc_free ,sizeof(t_pvoc), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(pvoc_class, t_pvoc, x_f);
	class_addmethod(pvoc_class, (t_method)pvoc_dsp, gensym("dsp"), 0);
	class_addmethod(pvoc_class, (t_method)pvoc_mute, gensym("mute"), A_DEFFLOAT,0);
	class_addmethod(pvoc_class, (t_method)pvoc_bypass, gensym("bypass"), A_DEFFLOAT,0);
	class_addmethod(pvoc_class, (t_method)pvoc_highfreq, gensym("highfreq"), A_DEFFLOAT,0);
	class_addmethod(pvoc_class, (t_method)pvoc_lowfreq, gensym("lowfreq"), A_DEFFLOAT,0);
	class_addmethod(pvoc_class, (t_method)pvoc_overlap, gensym("overlap"), A_DEFFLOAT,0);
	class_addmethod(pvoc_class, (t_method)pvoc_winfac, gensym("winfac"), A_DEFFLOAT,0);
	class_addmethod(pvoc_class, (t_method)pvoc_fftinfo, gensym("fftinfo"),0);
	class_addmethod(pvoc_class, (t_method)pvoc_assist, gensym("assist"), 0);
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if MSP
void main(void)
{
	setup((t_messlist **)&pvoc_class, (method)pvoc_new, (method)pvoc_free, (short)sizeof(t_pvoc), 0L, A_GIMME, 0);
	addmess((method)pvoc_dsp, "dsp", A_CANT, 0);
	addmess((method)pvoc_assist,"assist",A_CANT,0);
	addmess((method)pvoc_bypass,"bypass",A_DEFFLOAT,0);
	addmess((method)pvoc_mute,"mute",A_DEFFLOAT,0);
	addmess((method)pvoc_lowfreq,"lowfreq",A_DEFFLOAT,0);
	addmess((method)pvoc_highfreq,"highfreq",A_DEFFLOAT,0);
	addmess((method)pvoc_fftinfo,"fftinfo",0);
	addmess((method)pvoc_overlap, "overlap",  A_DEFFLOAT, 0);
	addmess((method)pvoc_winfac, "winfac",  A_DEFFLOAT, 0);
	addfloat((method)pvoc_float);
	dsp_initclass();
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void pvoc_mute(t_pvoc *x, t_floatarg tog)
{
	x->mute = (short)tog;
}

void pvoc_overlap(t_pvoc *x, t_floatarg f)
{
	int i = (int) f;
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->overlap = i;
	pvoc_init(x,1);
}

void pvoc_winfac(t_pvoc *x, t_floatarg f)
{
	int i = (int)f;
	
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = i;
	pvoc_init(x,2);
}

void pvoc_fftinfo(t_pvoc *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void pvoc_free(t_pvoc *x ){
#if MSP
	dsp_free((t_pxobject *) x);
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
	freebytes(x->index,0);
	freebytes(x->table,0);
}

/*MSP only but Pd crashes without it 
maybe because of function definition???
*/
void pvoc_assist (t_pvoc *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
			case 1: sprintf(dst,"(signal/float) Pitch Modification Factor"); break;
			case 2: sprintf(dst,"(signal/float) Synthesis Threshold"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void pvoc_highfreq(t_pvoc *x, t_floatarg f)
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

void pvoc_lowfreq(t_pvoc *x, t_floatarg f)
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


void pvoc_init(t_pvoc *x, short initialized)
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
		x->overlap = 4;
	if(!power_of_two(x->winfac))
		x->winfac = 2;
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = x->N / 2;
	x->Nw2 = x->Nw / 2;
	x->inCount = -(x->Nw);
	x->bypass_state = 0;
	x->mult = 1. / (float) x->N;
	x->pitch_connected = 0;
	x->synt_connected = 0;
	x->mute = 0;
	x->synt = .000001;
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

void *pvoc_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
	t_pvoc *x = (t_pvoc *)newobject(pvoc_class);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");
#endif
	
#if PD
	t_pvoc *x = (t_pvoc *)pd_new(pvoc_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
#endif
	
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
	
	pvoc_init(x,0);
	
	return (x);
}

t_int *pvoc_perform(t_int *w)
{
	int 	i,j, in,on;
	int    amp,freq,chan,L = 8192;
	
	float    a,ainc,f,finc,address;
	int breaker = 0;
	t_pvoc *x = (t_pvoc *) (w[1]);
	t_float *inbuf = (t_float *)(w[2]);
	t_float *in2 = (t_float *)(w[3]);
	t_float *in3 = (t_float *)(w[4]);
	t_float *outbuf = (t_float *)(w[5]);
	t_int n = w[6];
	
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
	float P  = x->P; 
	float Iinv = 1./x->D;
	float myPInc = x->myPInc;
	float *table = x->table;
	float *lastamp = x->lastamp ;
	float *lastfreq = x->lastfreq ;
	float *bindex = x->index;
	float *lastphase_in = x->c_lastphase_in;
	float *lastphase_out = x->c_lastphase_out;
	
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
	
	
	if(x->mute){
		while(n--){
			*outbuf++ = 0;
		}
		return w+7;
	}
#if MSP
	if(x->pitch_connected) {
		P  = *in2;
		myPInc = P * (float) x->L / (float)x->R;
	}
	
	if (x->synt_connected) {
		synt = *in3;
	}
#endif
	// Pd has superior float/signal inlet treatment
#if PD
	P = *in2; 
	synt = *in3;
	myPInc = P * (float) x->L / (float)x->R;
#endif
	
	if (x->bypass_state) {
		for( j = 0; j < D; j++ ) {
			*outbuf++ = *inbuf++;
		}
		return w+7;
	}
	
    in = on = x->inCount ;
	
    in += D;
    on += I;
	
    for ( j = 0 ; j < (Nw - D) ; j++ ){
		input[j] = input[j+D];
    }
    for ( j = (Nw-D), i = 0 ; j < Nw; j++, i++ ) {// unneeded loop variable
		input[j] = *inbuf++;
    }
	
    fold( input, Wanal, Nw, buffer, N, in );   
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
	
	
	
    // restore state variables
    x->inCount = in;
	return (w+7);
}	

void pvoc_bypass(t_pvoc *x, t_floatarg state)
{
	x->bypass_state = state;	
}

#if MSP
void pvoc_float(t_pvoc *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		x->P = (float)f;
		x->myPInc = x->P*x->L/x->R;
    }
	else if (inlet == 2)
    {
		x->synt = (float)f;
    }
	
}
#endif


void pvoc_dsp(t_pvoc *x, t_signal **sp, short *count)
{
	
#if MSP // these blew up Pd
	x->pitch_connected = count[1];
	x->synt_connected = count[2];
#endif
	
	if(x->D != sp[0]->s_n || x->R != sp[0]->s_sr ){
		x->D = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		pvoc_init(x,1);
	}
	
	dsp_add(pvoc_perform, 6, x, 
			sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, 
			sp[0]->s_n);
}

