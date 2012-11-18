#include "MSPd.h"
#include "fftease.h"


#if MSP
void *scrape_class;
#endif 

#if PD
static t_class *scrape_class;
#endif

#define OBJECT_NAME "scrape~"

typedef struct _scrape
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
	//
	float knee;
	float cutoff;
	float scrape_mult;
	float thresh1;
	float thresh2;
	float *threshfunc;
	short connected[8];
	short mute;
	short bypass;
	//
	float mult; 
	float *trigland;
	int *bitshuffle;
	int overlap;//overlap factor
		int winfac;//window factor
			int vs;//vector size
} t_scrape;

void *scrape_new(t_symbol *msg, short argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *scrape_perform(t_int *w);
void scrape_dsp(t_scrape *x, t_signal **sp, short *count);
void scrape_assist(t_scrape *x, void *b, long m, long a, char *s);
void scrape_float(t_scrape *x, double f) ;
void update_thresh_function( t_scrape *x );
void scrape_frowned( float *S, float *C, float *threshfunc, float fmult, int N2 );
void scrape_mute(t_scrape *x, t_floatarg toggle);
void scrape_bypass(t_scrape *x, t_floatarg toggle);
void scrape_dsp_free( t_scrape *x );
void update_thresh_function( t_scrape *x );
void scrape_init(t_scrape *x, short initialized);
void scrape_fftinfo(t_scrape *x);
void scrape_overlap(t_scrape *x, t_floatarg f);
void scrape_winfac(t_scrape *x, t_floatarg f);

#if MSP
void main(void)
{
	setup((t_messlist **) &scrape_class, (method)scrape_new, (method)scrape_dsp_free, 
		  (short)sizeof(t_scrape), 0, A_GIMME, 0);
	addmess((method)scrape_dsp, "dsp", A_CANT, 0);
	addmess((method)scrape_assist,"assist",A_CANT,0);
	addmess ((method)scrape_mute, "mute", A_FLOAT, 0);
	addmess ((method)scrape_bypass, "bypass", A_FLOAT, 0);
	addmess((method)scrape_overlap,"overlap",A_DEFFLOAT,0);
	addmess((method)scrape_winfac,"winfac",A_DEFFLOAT,0);
	addmess((method)scrape_fftinfo,"fftinfo",0);  addfloat((method) scrape_float);
	dsp_initclass();
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void scrape_tilde_setup(void)
{
	scrape_class = class_new(gensym("scrape~"), (t_newmethod)scrape_new, 
							 (t_method)scrape_dsp_free ,sizeof(t_scrape), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(scrape_class, t_scrape, x_f);
	class_addmethod(scrape_class, (t_method)scrape_dsp, gensym("dsp"), 0);
	class_addmethod(scrape_class, (t_method)scrape_assist, gensym("assist"), 0);
	class_addmethod(scrape_class, (t_method)scrape_mute, gensym("mute"), A_DEFFLOAT,0);
	class_addmethod(scrape_class, (t_method)scrape_bypass, gensym("bypass"), A_DEFFLOAT,0);
	class_addmethod(scrape_class, (t_method)scrape_overlap, gensym("overlap"), A_DEFFLOAT,0);
	class_addmethod(scrape_class,(t_method)scrape_winfac,gensym("winfac"),A_FLOAT,0);
	class_addmethod(scrape_class,(t_method)scrape_fftinfo,gensym("fftinfo"),0);
	
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif


void scrape_dsp_free( t_scrape *x )
{
#if MSP
	dsp_free( (t_pxobject *) x);
#endif
	
	freebytes(x->trigland,0);
	freebytes(x->bitshuffle,0);
	freebytes(x->Wanal,0);
	freebytes(x->Wsyn,0);
	freebytes(x->input,0);
	freebytes(x->Hwin,0);
	freebytes(x->buffer,0);
	freebytes(x->channel,0);
	freebytes(x->output,0);
	freebytes(x->threshfunc,0);
}

void scrape_assist (t_scrape *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input ");break;
			case 1: sprintf(dst,"(float/signal) Knee Frequency"); break;
			case 2: sprintf(dst,"(float/signal) Cutoff Frequency"); break;
			case 3: sprintf(dst,"(float/signal) Knee Threshold"); break;
			case 4: sprintf(dst,"(float/signal) Cutoff Threshold"); break;
			case 5: sprintf(dst,"(float/signal) Multiplier For Weak Bins"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void *scrape_new(t_symbol *msg, short argc, t_atom *argv)
{
#if MSP
	t_scrape *x = (t_scrape *)newobject(scrape_class);
	dsp_setup((t_pxobject *)x,6);
	outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
	int i;
	t_scrape *x = (t_scrape *)pd_new(scrape_class);
	for(i=0;i<5;i++)
		inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
#endif
	x->knee = atom_getfloatarg(0, argc, argv);
	x->cutoff = atom_getfloatarg(1, argc, argv);
	x->thresh1 = atom_getfloatarg(2, argc, argv);
	x->thresh2 = atom_getfloatarg(3, argc, argv);
	x->scrape_mult = atom_getfloatarg(4, argc, argv);  
	x->overlap = atom_getfloatarg(5, argc, argv); 
	x->winfac = atom_getfloatarg(6, argc, argv);  
	if(x->knee <= 0)
		x->knee = 1000.0;
	if(x->cutoff <= 0)
		x->cutoff = 4000.0;
	if(x->thresh1 <= 0)
		x->thresh1 = .0001 ;
	if(x->thresh2 <= 0)
		x->thresh2 = .09 ;
	if( x->scrape_mult < 0 || x->scrape_mult > 10 ){
		x->scrape_mult = 0.1;
	}
	if(!power_of_two(x->overlap))
		x->overlap = 4;
	if(!power_of_two(x->winfac))
		x->winfac = 1;
	
	x->vs = sys_getblksize();
	x->R = sys_getsr();
	scrape_init(x,0);
	return (x);
}

void scrape_init(t_scrape *x, short initialized)
{
	int i;
	
	x->D = x->vs;
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->inCount = -(x->Nw);
	x->mult = 1. / (float) x->N;
	
	if(!initialized){
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
		x->threshfunc = (float *) getbytes(MAX_N2 * sizeof(float));
	}
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));  
	makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
	init_rdft( x->N, x->bitshuffle, x->trigland);
	if(initialized != 2)
		update_thresh_function(x);
}

void scrape_overlap(t_scrape *x, t_floatarg f)
{
	int i = (int) f;
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->overlap = i;
	scrape_init(x,1);
}

void scrape_winfac(t_scrape *x, t_floatarg f)
{
	int i = (int)f;
	
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = i;
	scrape_init(x,2);
}

void scrape_fftinfo(t_scrape *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

#if MSP
void scrape_float(t_scrape *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		if( f> 50  && f < 18000) {
			x->knee = f ;
			update_thresh_function( x );	
		}	
		
    }
	else if (inlet == 2)
    {
		if( (f > x->knee)  && (f < 20000) ) {
			x->cutoff = f ;
			update_thresh_function( x );	
		}
    }
	else if (inlet == 3)
    {
		x->thresh1 = f;
		update_thresh_function( x );
    }
	else if (inlet == 4)
    {
		x->thresh2 = f;
		update_thresh_function( x );
    }
	else if (inlet == 5)
    {
		if( f > 0 ) {
			x->scrape_mult = f;
		}
    }
} 
#endif
void update_thresh_function( t_scrape *x ) 
{
	float funda, curfreq, m1, m2;
	int i;
	
	funda = (float)  x->R / ((float)x->N * 2.0 );
	curfreq = funda ;
	for( i = 0; i < x->N2; i++ ) {
		if( curfreq  < x->knee ){
			x->threshfunc[i] = 0.0 ;
		} else if( curfreq >= x->knee && curfreq < x->cutoff ) {
			m2 = (x->knee - curfreq) / (x->cutoff - x->knee) ;
			m1 = 1.0 - m2 ;
			x->threshfunc[i] = m1 * x->thresh1 + m2 * x->thresh2 ;
		} else {
			x->threshfunc[i] = x->thresh2;
		}
		curfreq += funda ;
	}
}

void scrape_mute(t_scrape *x, t_floatarg toggle)
{
	x->mute = (short)toggle;	
}

void scrape_bypass(t_scrape *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;	
}


t_int *scrape_perform(t_int *w)
{
	float sample, outsamp ;
	
	
	
	int	i,j;
	float tmp ;	
	
	short update = 0;
	
	t_scrape *x = (t_scrape *) (w[1]);
	t_float *in = (t_float *)(w[2]);
	t_float *knee_freq = (t_float *)(w[3]);
	t_float *cut_freq = (t_float *)(w[4]);
	t_float *thresh1 = (t_float *)(w[5]);
	t_float *thresh2 = (t_float *)(w[6]);
	t_float *scrape_mult = (t_float *)(w[7]);
	t_float *out = (t_float *)(w[8]);
	t_int n = w[9];
	
	int inCount = x->inCount;
	int R = x->R;
	int N = x->N;
	int N2 = x->N2;
	int D = x->D;
	int Nw = x->Nw;
	float *Wanal = x->Wanal;
	float *Wsyn = x->Wsyn;		
	float *input = x->input;
	float *output = x->output;
	float *buffer = x->buffer;
	float *channel = x->channel;
	float mult = x->mult ;
	int *bitshuffle = x->bitshuffle;
	float *trigland = x->trigland ;      
	float *threshfunc = x->threshfunc ;
	short *connected = x->connected;
	
	if( x->mute ){
		while( n-- ){
			*out++ = 0.0;
		}
		return (w+10); // always what dsp_add says + 1
	}
	if( x->bypass ){
		while( n-- ){
			*out++ = *in++ * 0.5; // gain compensation
		}
		return (w+10); // always what dsp_add says + 1
	}
	if( connected[1] ){
		tmp = *knee_freq++;
		if( tmp > 50 && tmp < 20000 ){
			x->knee = tmp;
			update = 1;
		}
	}
	if( connected[2] ){
		tmp = *cut_freq++;
		if( tmp > x->knee && tmp < 20000 ){
			x->cutoff = *cut_freq++;
			update = 1;
		}
	}
	if( connected[3] ){
		x->thresh1 = *thresh1++ ;
		update = 1;
	}
	if( connected[4] ){
		x->thresh2 = *thresh2++ ;
		update = 1;
	}
	if( connected[5] ){
		x->scrape_mult = *scrape_mult++ ;
	}
	
	if( update ){
		update_thresh_function( x );	
	}
	
	x->inCount += D;
	
	for ( j = 0 ; j < Nw - D ; j++ ){
		input[j] = input[j+D];
	}
	for ( j = Nw - D; j < Nw; j++ ) {
		input[j] = *in++;
	}
	
	fold( input, Wanal, Nw, buffer, N, inCount );	
	rdft( N, 1, buffer, bitshuffle, trigland );
	
	scrape_frowned( buffer, channel, threshfunc, x->scrape_mult, N2 );
	
	rdft( N, -1, buffer, bitshuffle, trigland );
	overlapadd( buffer, N, Wsyn, output, Nw, inCount);
	
	for ( j = 0; j < D; j++ )
		*out++ = output[j] * mult;
	
	for ( j = 0; j < Nw - D; j++ )
		output[j] = output[j+D];
	
	for ( j = Nw - D; j < Nw; j++ )
		output[j] = 0.;
	
	return (w+10); // always what dsp_add says + 1
}		



void scrape_frowned( float *S, float *C, float *threshfunc, float fmult, int N2 )

{
	int real, imag, amp, phase;
	float a, b;
	int i;
	float maxamp = 1.;
	
	for( i = 0; i <= N2; i++ ){
		amp = i<<1;
		if( maxamp < C[amp] ){
			maxamp = C[amp];
		}
	}
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		a = ( i == N2 ? S[1] : S[real] );
		b = ( i == 0 || i == N2 ? 0. : S[imag] );
		C[amp] = hypot( a, b );
		
		if ( (C[amp]) < threshfunc[i] * maxamp ){
			C[amp] *= fmult;
		}
		C[phase] = -atan2( b, a );
	}
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		S[real] = *(C+amp) * cos( *(C+phase) );
		if ( i != N2 )
			S[imag] = -*(C+amp) * sin( *(C+phase) );
	}
}

void scrape_dsp(t_scrape *x, t_signal **sp, short *count)
{
	long i;
#if MSP
	for( i = 0; i < 6; i++ ){
		x->connected[i] = count[i];
	}
#endif
#if PD
	for( i = 0; i < 6; i++ ){
		x->connected[i] = 1;
	}
#endif
	if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr){
		x->vs = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		scrape_init(x,1);
	}
	dsp_add(scrape_perform, 9, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,  sp[3]->s_vec, 
			sp[4]->s_vec, sp[5]->s_vec, 
			sp[6]->s_vec, sp[0]->s_n);
}

