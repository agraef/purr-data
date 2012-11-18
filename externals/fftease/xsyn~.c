#include "MSPd.h"
#include "fftease.h"

#if MSP
void *xsyn_class;
#endif 

#if PD
static t_class *xsyn_class;
#endif

#define OBJECT_NAME "xsyn~"

typedef struct _xsyn
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
	float *input1;	
	float *buffer1;
	float *channel1;
	
	
	float *input2;	
	float *buffer2;
	float *channel2;
	//
	int	inCount;
	float *Hwin;
	float *Wanal;	
	float *Wsyn;	
	float *output;
	/* xsyn vars */
	
	float *c_lastphase_in1;
	float *c_lastphase_in2;
	float *c_lastphase_out;
	float c_fundamental;
	float c_factor_in;
	float c_factor_out;
	
	//	float *filter ;
	// for fast fft
	float mult; 
	float *trigland;
	int *bitshuffle;
	int overlap;//overlap factor
		int winfac;//window factor
			int vs;//vector size
				short mute;//flag
} t_xsyn;

void *xsyn_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *xsyn_perform(t_int *w);
void xsyn_dsp(t_xsyn *x, t_signal **sp, short *count);
void xsyn_assist(t_xsyn *x, void *b, long m, long a, char *s);
void xsyn_dsp_free( t_xsyn *x );
void xsyn_init(t_xsyn *x, short initialized);
void xsyn_mute(t_xsyn *x, t_floatarg toggle);
void xsyn_fftinfo(t_xsyn *x);
void xsyn_overlap(t_xsyn *x, t_floatarg f);
void xsyn_winfac(t_xsyn *x, t_floatarg f);

void xsyn_dsp_free( t_xsyn *x )
{
#if MSP
	dsp_free( (t_pxobject *) x);
#endif
	freebytes(x->c_lastphase_in1,0); 
	freebytes(x->c_lastphase_in2,0); 
	freebytes(x->c_lastphase_out,0); 
	freebytes(x->trigland,0); 
	freebytes(x->bitshuffle,0); 
	freebytes(x->Wanal,0); 
	freebytes(x->Wsyn,0);
	freebytes(x->Hwin,0);
	freebytes(x->input1,0);
	freebytes(x->buffer1,0);
	freebytes(x->channel1,0);
	freebytes(x->input2,0);
	freebytes(x->buffer2,0);
	freebytes(x->channel2,0);
	freebytes(x->output,0);
}

#if MSP
void main(void)
{
	setup((t_messlist **)&xsyn_class, (method) xsyn_new, (method)xsyn_dsp_free, 
		  (short)sizeof(t_xsyn), 0, A_GIMME, 0);
	addmess((method)xsyn_dsp, "dsp", A_CANT, 0);
	addmess((method)xsyn_assist,"assist",A_CANT,0);
	addmess((method)xsyn_mute,"mute",A_FLOAT,0);
	addmess((method)xsyn_overlap,"overlap",A_FLOAT,0);
	addmess((method)xsyn_winfac,"winfac",A_DEFFLOAT,0);
	addmess((method)xsyn_fftinfo,"fftinfo",0);
	dsp_initclass();
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void xsyn_tilde_setup(void)
{
	xsyn_class = class_new(gensym("xsyn~"), (t_newmethod)xsyn_new, 
						   (t_method)xsyn_dsp_free ,sizeof(t_xsyn), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(xsyn_class, t_xsyn, x_f);
	class_addmethod(xsyn_class, (t_method)xsyn_dsp, gensym("dsp"), 0);
	class_addmethod(xsyn_class, (t_method)xsyn_assist, gensym("assist"), 0);
	class_addmethod(xsyn_class, (t_method)xsyn_mute, gensym("mute"), A_DEFFLOAT,0);
	class_addmethod(xsyn_class, (t_method)xsyn_overlap, gensym("overlap"), A_DEFFLOAT,0);
	class_addmethod(xsyn_class,(t_method)xsyn_winfac,gensym("winfac"),A_FLOAT,0);
	class_addmethod(xsyn_class,(t_method)xsyn_fftinfo,gensym("fftinfo"),0);
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void xsyn_mute(t_xsyn *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void xsyn_overlap(t_xsyn *x, t_floatarg f)
{
	int i = (int) f;
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->overlap = i;
	xsyn_init(x,1);
}

void xsyn_winfac(t_xsyn *x, t_floatarg f)
{
	int i = (int)f;
	
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = i;
	xsyn_init(x,2);
}

void xsyn_fftinfo(t_xsyn *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void xsyn_assist (t_xsyn *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Input 1 ");
				break;
			case 1:
				sprintf(dst,"(signal) Input 2 ");
				break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output ");
	}
}

void *xsyn_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
	t_xsyn *x = (t_xsyn *)newobject(xsyn_class);
	dsp_setup((t_pxobject *)x,2);
	outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
	t_xsyn *x = (t_xsyn *)pd_new(xsyn_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
#endif
	x->overlap = atom_getfloatarg(0,argc,argv);
	x->winfac = atom_getfloatarg(1,argc,argv);
	if(!power_of_two(x->overlap))
		x->overlap = 4;
	if(!power_of_two(x->winfac))
		x->winfac = 1;
	
	x->R = sys_getsr();
	x->vs = sys_getblksize();
	
	xsyn_init(x,0);
	return (x);
}

void xsyn_init(t_xsyn *x, short initialized)
{
	
	
	x->D = x->vs;
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->inCount = -(x->Nw);
	x->mult = 1. / (float) x->N;
	
	x->c_fundamental =  (float) x->R/( (x->N2)<<1 );
	x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
	x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
	if(!initialized){
		x->mute = 0;
		x->Wanal = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->Wsyn = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->Hwin = (float *) getbytes( MAX_Nw * sizeof(float) ); 
		x->input1 = (float *) getbytes(MAX_Nw * sizeof(float));	
		x->buffer1 = (float *) getbytes(MAX_N  * sizeof(float));
		x->channel1 = (float *) getbytes((MAX_N+2)  * sizeof(float));
		x->input2 = (float *) getbytes(MAX_Nw * sizeof(float));	
		x->buffer2 = (float *) getbytes(MAX_N  * sizeof(float));
		x->channel2 = (float *) getbytes((MAX_N+2)  * sizeof(float));
		x->output = (float *) getbytes(MAX_Nw * sizeof(float));
		x->bitshuffle = (int *) getbytes(MAX_N * 2 * sizeof(int));
		x->trigland = (float *) getbytes(MAX_N * 2 * sizeof(float));
		x->c_lastphase_in1 = (float *) getbytes((MAX_N2+1) * sizeof(float));
		x->c_lastphase_in2 = (float *) getbytes((MAX_N2+1) * sizeof(float));
		x->c_lastphase_out = (float *) getbytes((MAX_N2+1) * sizeof(float));
	} 
	memset((char *)x->input1,0,x->Nw * sizeof(float));
	memset((char *)x->input2,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->c_lastphase_in1,0,(x->N2+1) * sizeof(float));
	memset((char *)x->c_lastphase_in2,0,(x->N2+1) * sizeof(float));
	memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));
		
	init_rdft( x->N, x->bitshuffle, x->trigland);
	makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
}

t_int *xsyn_perform(t_int *w)
{
	t_float *in1,*out, *in2;
	
	float sample, outsamp ;
	
	float	*input1, *input2,
		*output,
		*buffer1, *buffer2,
		*Wanal,
		*Wsyn,
		*channel1, *channel2;
	
	int		n,
		i,j,
		inCount,
		R,
		N,
		N2,
		D,
		Nw;
	float maxamp ;	
	int	*bitshuffle;	
	float *trigland;	
	float mult;
	float a1, a2, b1, b2;
	int even, odd;
	
	
	
	t_xsyn *x = (t_xsyn *) (w[1]);
	in1 = (t_float *)(w[2]);
	in2 = (t_float *)(w[3]);
	out = (t_float *)(w[4]);
	n = (int)(w[5]);
	
	/* dereference struncture  */	
	input1 = x->input1;
	input2 = x->input2;
	buffer1 = x->buffer1;
	buffer2 = x->buffer2;
	inCount = x->inCount;
	R = x->R;
	N = x->N;
	N2 = x->N2;
	D = x->D;
	Nw = x->Nw;
	Wanal = x->Wanal;
	Wsyn = x->Wsyn;
	output = x->output;
	buffer1 = x->buffer1;
	buffer2 = x->buffer2;
	channel1 = x->channel1;
	channel2 = x->channel2;
	bitshuffle = x->bitshuffle;
	trigland = x->trigland;
	mult = x->mult;	
	
	if(x->mute){
		while(n--){
			*out++ = 0.0;
		}
		return (w+6);
	}
	x->inCount += D;
	
	for ( j = 0 ; j < Nw - D ; j++ ){
		input1[j] = input1[j+D];
		input2[j] = input2[j+D];
	}
	for ( j = Nw - D; j < Nw; j++ ) {
		input1[j] = *in1++;
		input2[j] = *in2++;
	}
	
	fold( input1, Wanal, Nw, buffer1, N, inCount );		
	fold( input2, Wanal, Nw, buffer2, N, inCount );	
	rdft( N, 1, buffer1, bitshuffle, trigland );
	rdft( N, 1, buffer2, bitshuffle, trigland );
	leanconvert( buffer1, channel1, N2 );
	leanconvert( buffer2, channel2, N2 );
	maxamp = 0 ;
	for( i = 0; i < N; i+= 2 ) {
		if( channel2[i] > maxamp ) {
			maxamp = channel2[i];
		}
	}
	if( maxamp == 0.0 )
		maxamp = 1.0 ;
	for( i = 0; i < N; i+= 2 ) {
		channel1[i] *= (channel2[i] / maxamp );
	}
	
	leanunconvert( channel1, buffer1,  N2 );
	
	rdft( N, -1, buffer1, bitshuffle, trigland );
	
	overlapadd( buffer1, N, Wsyn, output, Nw, inCount);
	
	for ( j = 0; j < D; j++ )
		*out++ = output[j] * mult;
	
	for ( j = 0; j < Nw - D; j++ )
		output[j] = output[j+D];
	
	for ( j = Nw - D; j < Nw; j++ )
		output[j] = 0.;
	
	
	
	/* restore state variables */
	x->inCount = inCount;
	return (w+6);
}		

void xsyn_dsp(t_xsyn *x, t_signal **sp, short *count)
{
	if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr ){
		x->vs = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		xsyn_init(x,1);
	}
	
	dsp_add(xsyn_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,  sp[0]->s_n);
}

