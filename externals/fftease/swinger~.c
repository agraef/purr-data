#include "MSPd.h"
#include "fftease.h"

#if MSP
void *swinger_class;
#endif 

#if PD
static t_class *swinger_class;
#endif

#define OBJECT_NAME "swinger~"

typedef struct _swinger
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
    int *bitshuffle;
    
    float *Wanal;	
    float *Wsyn;	
    float *inputOne;
    float *inputTwo;
    float *Hwin;
    float *bufferOne;
    float *bufferTwo;
    float *channelOne;
 	float *channelTwo;
    float *output;
    float mult; 
    float *trigland;
	int overlap;//overlap factor
		int winfac;// window factor
			int vs;//last measurement of vector size
				short mute;
				
} t_swinger;


/* msp function prototypes */

void *swinger_new(t_symbol *s, int argc, t_atom *argv);
t_int *swinger_perform(t_int *w);
void swinger_dsp(t_swinger *x, t_signal **sp, short *count);
void swinger_assist(t_swinger *x, void *b, long m, long a, char *s);
void swinger_mute(t_swinger *x, t_floatarg state);
void swinger_init(t_swinger *x, short initialized);
void swinger_dsp_free(t_swinger *x);
void swinger_overlap(t_swinger *x, t_floatarg o);
void swinger_winfac(t_swinger *x, t_floatarg o);
//int power_of_two(int p);
void swinger_fftinfo(t_swinger *x);

#if MSP

void main(void)
{
    setup((t_messlist **)&swinger_class, (method) swinger_new, (method)dsp_free, (short)sizeof(t_swinger),
		  0L, A_GIMME, 0);
	
    addmess((method)swinger_dsp, "dsp", A_CANT, 0);
    addmess((method)swinger_assist,"assist",A_CANT,0);
    addmess((method)swinger_mute,"mute",A_FLOAT,0);
	addmess((method)swinger_overlap,"overlap",A_FLOAT,0);
	addmess((method)swinger_winfac,"winfac",A_FLOAT,0);
	addmess((method)swinger_fftinfo,"fftinfo",0);
    dsp_initclass();
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif
#if PD
/* Pd Initialization */
void swinger_tilde_setup(void)
{
	swinger_class = class_new(gensym("swinger~"), (t_newmethod)swinger_new, 
							  (t_method)swinger_dsp_free ,sizeof(t_swinger), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(swinger_class, t_swinger, x_f);
	class_addmethod(swinger_class, (t_method)swinger_dsp, gensym("dsp"), 0);
	class_addmethod(swinger_class, (t_method)swinger_mute, gensym("mute"), A_DEFFLOAT,0);
	class_addmethod(swinger_class, (t_method)swinger_overlap, gensym("overlap"), A_DEFFLOAT,0);
	class_addmethod(swinger_class, (t_method)swinger_winfac, gensym("winfac"), A_DEFFLOAT,0);
	class_addmethod(swinger_class, (t_method)swinger_fftinfo, gensym("fftinfo"),0);
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

/* diagnostic messages for Max */
void swinger_fftinfo(t_swinger *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void swinger_mute(t_swinger *x, t_floatarg state)
{
	x->mute = state;	
}

void swinger_assist (t_swinger *x, void *b, long msg, long arg, char *dst)
{
	
	if (msg == 1) {
		
		switch (arg) {
			
			case 0:		sprintf(dst,"(signal) Signal to be Phase Replaced ");
				break;
				
			case 1:		sprintf(dst,"(signal) Signal to Supply Phase Information ");
				break;
		}
	}
	
	else {
		
		if (msg == 2)
			sprintf(dst,"(signal) Swinger Output");
		
	}
}


void *swinger_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
	t_swinger 	*x = (t_swinger *) newobject(swinger_class);
	dsp_setup((t_pxobject *)x,2);
	outlet_new((t_pxobject *)x, "signal");
	/* make sure that signal inlets and outlets have their own memory */
	x->x_obj.z_misc |= Z_NO_INPLACE;
#endif
#if PD
	t_swinger *x = (t_swinger *)pd_new(swinger_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
#endif
	
	/* INITIALIZATIONS */
	x->overlap = atom_getfloatarg(0,argc,argv);
	x->winfac = atom_getfloatarg(1,argc,argv);
	if(!x->winfac)	
		x->winfac = 1;
	if(!x->overlap)
		x->overlap = 4;
	x->vs = sys_getblksize();
	x->R = sys_getsr();
	swinger_init(x,0); 
	return (x);
}

void swinger_overlap(t_swinger *x, t_floatarg o)
{
	if(!power_of_two(o)){
		post("%f is not a power of two",o);
		return;
	}
	x->overlap = o;
	swinger_init(x,1);
}

void swinger_winfac(t_swinger *x, t_floatarg f)
{
	if(!power_of_two(f)){
	    error("%f is not a power of two",f);
	    return;
	}
	x->winfac = (int)f;
	swinger_init(x,1);
}

void swinger_init(t_swinger *x, short initialized)
{
	int i;
	x->D = x->vs;
	x->N = x->vs * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->inCount = -(x->Nw);
	x->mult = 1. / (float) x->N;
	
	if(!initialized){
		x->Wanal = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->Wsyn = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->Hwin = (float *) getbytes( MAX_Nw * sizeof(float) ); 
		x->inputOne = (float *) getbytes( MAX_Nw * sizeof(float) );
		x->inputTwo = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->bufferOne = (float *) getbytes( MAX_N * sizeof(float) );
		x->bufferTwo = (float *) getbytes( MAX_N * sizeof(float) ); 
		x->channelOne = (float *) getbytes( (MAX_N+2) * sizeof(float) );
		x->channelTwo = (float *) getbytes( (MAX_N+2) * sizeof(float) );
		x->output = (float *) getbytes( MAX_Nw * sizeof(float) );
		x->bitshuffle = (int *) getbytes( MAX_N * 2 * sizeof( int ) );
		x->trigland = (float *) getbytes( MAX_N * 2 * sizeof( float ) );
	}
	memset((char *)x->inputOne,0,x->Nw * sizeof(float));
	memset((char *)x->inputTwo,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	
	init_rdft(x->N, x->bitshuffle, x->trigland);
	makehanning(x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 1);// wants an ODD window
		
}

t_int *swinger_perform(t_int *w)
{
	
	int		n,
	i,j,
	inCount,
	R,
	N,
	N2,
	D,
	Nw,
	invert = 1,
	even, odd,
	*bitshuffle;
	
	float		maxamp,	
		threshMult = 1.,
		mult,
		a1, b1,
		a2, b2,
		*inputOne,
		*inputTwo,
		*bufferOne,
		*bufferTwo,
		*output,
		*Wanal,
		*Wsyn,
		*channelOne,
		*channelTwo,
		*trigland;
	t_float 	*inOne,
		*inTwo,	
		*out;
	
	/* get our inlets and outlets */
	
	t_swinger *x = (t_swinger *) (w[1]);
	inOne = (t_float *)(w[2]);
	inTwo = (t_float *)(w[3]);
	
	out = (t_float *)(w[4]);
	n = (int)(w[5]);
	
	/* dereference structure  */	
	
	inputOne = x->inputOne;
	inputTwo = x->inputTwo;
	bufferOne = x->bufferOne;
	bufferTwo = x->bufferTwo;
	inCount = x->inCount;
	R = x->R;
	N = x->N;
	N2 = x->N2;
	D = x->D;
	Nw = x->Nw;
	Wanal = x->Wanal;
	Wsyn = x->Wsyn;
	output = x->output;
	channelOne = x->channelOne;
	channelTwo = x->channelTwo;
	bitshuffle = x->bitshuffle;
	trigland = x->trigland;
	mult = x->mult;	
	
	/* no computation if muted */
	
	if(x->mute){
		while(n--) 
			*out++ = 0.0;
		return w+6;
	}
	
	inCount += D;
	
	for ( j = 0 ; j < Nw - D ; j++ ) {
		inputOne[j] = inputOne[j+D];
		inputTwo[j] = inputTwo[j+D];
	}
	
	for ( j = Nw-D; j < Nw; j++ ) {
		inputOne[j] = *inOne++;
		inputTwo[j] = *inTwo++;
	}
	
	/* apply hamming window and fold our window buffer into the fft buffer */ 
	
	fold( inputOne, Wanal, Nw, bufferOne, N, inCount );
	fold( inputTwo, Wanal, Nw, bufferTwo, N, inCount );
	
	/* do an fft */ 
	
	rdft( N, 1, bufferOne, bitshuffle, trigland );
	rdft( N, 1, bufferTwo, bitshuffle, trigland );
	
	/* use redundant coding for speed, even though moving the invert variable
		comparison outside of the for loop will give us only a minimal performance
		increase (hypot and atan2 are the most intensive portions of this code).
		consider adding a table lookup for atan2 instead.
		*/
	
	/* convert to polar coordinates from complex values */
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
		b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
		
		a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
		b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
		
		/* replace signal one's phases with those of signal two */
		
		*(channelOne+even) = hypot( a1, b1 );
		*(channelOne+odd) = -atan2( b2, a2 );        
	}
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	
	
	/* do an inverse fft */
	
	rdft( N, -1, bufferOne, bitshuffle, trigland );
	
	
	
	/* dewindow our result */
	
	overlapadd( bufferOne, N, Wsyn, output, Nw, inCount);
	
	/* set our output and adjust our retaining output buffer */
	
	for ( j = 0; j < D; j++ )
		*out++ = output[j] * mult;
	
	for ( j = 0; j < Nw - D; j++ )
		output[j] = output[j+D];
	
	for ( j = Nw - D; j < Nw; j++ )
		output[j] = 0.;
	
	
	/* restore state variables */
	
	x->inCount = inCount % Nw;
	return (w+6);
}	

void swinger_dsp_free( t_swinger *x )
{
#if MSP
	dsp_free((t_pxobject *) x);
#endif
	freebytes(x->trigland,0);
	freebytes(x->bitshuffle,0);
	freebytes(x->Wanal,0);
	freebytes(x->Wsyn,0);
	freebytes(x->Hwin,0);
	freebytes(x->inputOne,0);
	freebytes(x->inputTwo,0);
	freebytes(x->bufferOne,0);
	freebytes(x->bufferTwo,0);
	freebytes(x->channelOne,0);
	freebytes(x->channelTwo,0);
	freebytes(x->output,0);
}

void swinger_dsp(t_swinger *x, t_signal **sp, short *count)
{
	if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr){
		x->vs = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		swinger_init(x,1);
	}
	dsp_add(swinger_perform, 5, x,
			sp[0]->s_vec,sp[1]->s_vec,sp[2]->s_vec,sp[0]->s_n);
}

