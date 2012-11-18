#include "MSPd.h"
#include "fftease.h"

#if MSP
void *taint_class;
#endif
#if PD
static t_class *taint_class;
#endif

#define OBJECT_NAME "taint~"


/*
Adding -32dB pad for invert option. Also added latency mechanism in
 switching from normal to "invert" to avoid glitches from extreme
 amplitude disparities.
 
 Made all inlets of type signal (with float options).
 
 Threshold input is now linear, not dB (with Max doing the conversion
										if desired).
 
 -EL 10/1/2005
 
 */

typedef struct _taint
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
	int invert;
	int *bitshuffle;
    
	float threshold;
	float exponent;
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
	
	short connected[8];
	short mute;
	int overlap;//overlap factor
		int winfac;//window factor
			int vs;//vector size  
				int invert_countdown; // delay onset of invert effect to avoid loud glitches
				int invert_nextstate;// next state for invert   
					float invert_pad;   	
} t_taint;


/* msp function prototypes */

void *taint_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *taint_perform(t_int *w);
void taint_dsp(t_taint *x, t_signal **sp, short *count);
void taint_assist(t_taint *x, void *b, long m, long a, char *s);
void taint_dest(t_taint *x, double f);
void taint_invert(t_taint *x, t_floatarg toggle);
void taint_free(t_taint *x);
void taint_mute(t_taint *x, t_floatarg toggle);
void taint_fftinfo(t_taint *x);
void taint_tilde_setup(void);
void taint_overlap(t_taint *x, t_floatarg o);
void taint_winfac(t_taint *x, t_floatarg o);
void taint_init(t_taint *x, short initialized);
void taint_pad(t_taint *x, t_floatarg pad);


#if MSP
void main(void)
{
	setup( (struct messlist **) &taint_class, (void *) taint_new,
		   (method)dsp_free, (short) sizeof(t_taint),
		   0, A_GIMME, 0);
	
	addmess((method)taint_dsp, "dsp", A_CANT, 0);
	addmess((method)taint_assist,"assist",A_CANT,0);    
	addmess((method)taint_invert,"invert", A_FLOAT, 0);  
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
	
	addmess((method)taint_overlap,"overlap", A_FLOAT, 0);
	addmess((method)taint_winfac,"winfac", A_FLOAT, 0);
	addmess((method)taint_mute,"mute", A_FLOAT, 0);
	addmess((method)taint_pad,"pad", A_FLOAT, 0);
	addmess((method)taint_fftinfo,"fftinfo", 0);
	addfloat((method) taint_dest);
	dsp_initclass();
}

/* float input handling routine (MSP only)*/
void taint_dest(t_taint *x, double f)
{
	int inlet = x->x_obj.z_in;
	
	if ( inlet == 2 ) {
		x->exponent = f;	
	}
	
	if ( inlet == 3 ){
		/* x->threshold = (float) (pow( 10., (f * .05))); */
		x->threshold = f;
	}
}
#endif

#if PD
void taint_tilde_setup(void)
{
	taint_class = class_new(gensym("taint~"), (t_newmethod)taint_new, 
							(t_method)taint_free ,sizeof(t_taint), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(taint_class, t_taint, x_f);
	class_addmethod(taint_class, (t_method)taint_dsp, gensym("dsp"), 0);
	class_addmethod(taint_class, (t_method)taint_assist, gensym("assist"), 0);
	class_addmethod(taint_class, (t_method)taint_invert, gensym("invert"), A_FLOAT,0);
	class_addmethod(taint_class, (t_method)taint_overlap, gensym("overlap"), A_FLOAT,0);
	class_addmethod(taint_class, (t_method)taint_mute, gensym("mute"), A_FLOAT,0);
	class_addmethod(taint_class, (t_method)taint_pad, gensym("pad"), A_FLOAT,0);
	class_addmethod(taint_class, (t_method)taint_fftinfo, gensym("fftinfo"), A_CANT,0);
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif


void taint_mute(t_taint *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
	//  post("mute set to %f, %d",toggle,x->mute);
}

void taint_overlap(t_taint *x, t_floatarg o)
{
	if(!power_of_two(o)){
		error("%f is not a power of two",o);
		return;
	}
	x->overlap = (int)o;
	taint_init(x,1);
}

void taint_winfac(t_taint *x, t_floatarg f)
{
	if(!power_of_two(f)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = (int)f;
	taint_init(x,1);
}

void taint_fftinfo( t_taint *x )
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
	
}

void taint_free(t_taint *x)
{
#if MSP
	dsp_free((t_pxobject *) x);
#endif
	free(x->trigland);
	free(x->bitshuffle);
	free(x->Wanal);
	free(x->Wsyn);
	free(x->Hwin);
	free(x->inputOne);
	free(x->inputTwo);
	free(x->bufferOne);
	free(x->bufferTwo);
	free(x->channelOne);
	free(x->channelTwo);
	free(x->output);
}

void taint_pad(t_taint *x, t_floatarg pad)
{
	x->invert_pad = pad;
	taint_invert(x,x->invert);//resubmit to invert
}

void taint_invert(t_taint *x, t_floatarg toggle)
{
	
	x->invert_nextstate = toggle;
	x->invert_countdown = x->overlap; // delay effect for "overlap" vectors
	
	if(x->invert_nextstate){ // lower gain immediately; delay going to invert
		x->mult = (1. / (float) x->N) * x->invert_pad;
	} else {
		x->invert = 0; //immediately turn off invert; delay raising gain
	}
	
}

void taint_assist (t_taint *x, void *b, long msg, long arg, char *dst)
{
	
	if (msg == 1) {
		
		switch (arg) {
			case 0:		sprintf(dst,"(signal) Input One");break;
			case 1:		sprintf(dst,"(signal) Input Two"); break;
			case 2:		sprintf(dst,"(signal/float) Scaling Exponent"); break;
			case 3:		sprintf(dst,"(signal/float) Inverse Threshold"); break;
		}
	}
	
	else {
		
		if (msg == 2)
			sprintf(dst,"(signal) Output");
		
	}
}


void *taint_new(t_symbol *s, int argc, t_atom *argv)
{
	
#if MSP
	t_taint 	*x = (t_taint *) newobject(taint_class);
	dsp_setup((t_pxobject *)x,4);
	outlet_new((t_pxobject *)x, "signal");
	// x->x_obj.z_misc |= Z_NO_INPLACE; // probably not needed
#endif
#if PD
	t_taint *x = (t_taint *)pd_new(taint_class);
	/* add three additional signal inlets */
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
#endif
	
	
	/* optional arguments: scaling exponent, threshold (now linear), overlap, winfac */
	x->exponent = atom_getfloatarg(0,argc,argv);
	x->threshold = atom_getfloatarg(1,argc,argv);
	x->overlap = atom_getfloatarg(2,argc,argv);
	x->winfac = atom_getfloatarg(3,argc,argv);
	
	/*	
		x->threshold = (float) pow(10.0,(x->threshold * .05));
	*/
	/* sanity check */
	if(x->exponent < 0.25)
		x->exponent = 0.25;
	
	if(!power_of_two(x->overlap)){
		x->overlap = 4;
	}
	if(!power_of_two(x->winfac)){
		x->winfac = 1;
	}
  		
	x->vs = sys_getblksize();
	x->R = sys_getsr();
	
	taint_init(x,0);
	
	return (x);
}

void taint_init(t_taint *x, short initialized)
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
		x->invert_pad = 0.025; // -32 dB
		x->invert_countdown = 0;
		x->mute = 0;
		x->invert = 0;
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
	if(x->invert){
		x->mult *= x->invert_pad;
    }
	memset((char *)x->inputOne,0,x->Nw * sizeof(float));
	memset((char *)x->inputTwo,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
		
	init_rdft( x->N, x->bitshuffle, x->trigland);
	makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 1);
	
}


t_int *taint_perform(t_int *w)
{
	
	int		
    i,j,
    inCount,
    R,
    N,
    N2,
    D,
    Nw,
    invert = 0,
    even, odd,
    *bitshuffle;
	
	float		maxamp,	
		threshold = 1.,
		mult,
		exponent,
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
	
	
	
	/* get our inlets and outlets */
	
	t_taint *x = (t_taint *) (w[1]);
	t_float *inOne = (t_float *)(w[2]);
	t_float *inTwo = (t_float *)(w[3]);
	t_float *vec_exponent = (t_float *)(w[4]);
	t_float *vec_threshold = (t_float *)(w[5]);
	t_float *out = (t_float *)(w[6]);
	t_int n = w[7];
	
	short *connected = x->connected;
	/* dereference structure  */	
	if(connected[2])
		x->exponent = *vec_exponent;
	if(connected[3]){
		x->threshold = *vec_threshold;
		/*
		x->threshold = (float) (pow( 10., (x->threshold * .05)));
		 */
	}
	
	if(x->mute){
		while(n--)
			*out++ = 0.0;
		return w+8;
	}
	// do countdown
	if(x->invert_countdown > 0){
		
		if(x->invert) { // we 
		} else {
		}  
		--(x->invert_countdown);
		if(! x->invert_countdown){ // countdown just ended
			if(x->invert_nextstate){ // moving to invert (gain is already down)
				x->invert = x->invert_nextstate;
			} else { // invert is already off - now reset gain
				x->mult = 1. / (float) x->N;
			}
		}
	}
	
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
	invert = x->invert;
	exponent = x->exponent;
	
	if ( x->threshold != 0. )
		threshold = x->threshold;
	
	/* fill our retaining buffers */
	
	inCount += D;
	
	for ( j = 0 ; j < Nw - D ; j++ ) {
		inputOne[j] = inputOne[j+D];
		inputTwo[j] = inputTwo[j+D];
	}
	
	for ( j = Nw - D; j < Nw; j++ ) {
		inputOne[j] = *inOne++;
		inputTwo[j] = *inTwo++;
	}
	
	/* apply hamming window and fold our window buffer into the fft buffer */ 
	
	fold( inputOne, Wanal, Nw, bufferOne, N, inCount );
	fold( inputTwo, Wanal, Nw, bufferTwo, N, inCount );
	
	/* do an fft */ 
	
	rdft( N, 1, bufferOne, bitshuffle, trigland );
	rdft( N, 1, bufferTwo, bitshuffle, trigland );
	
	/* convert to polar coordinates from complex values */
	
	if (invert) {
		
		for ( i = 0; i <= N2; i++ ) {
			
			float magnitude;	
			
			odd = ( even = i<<1 ) + 1;
			
			a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
			
			a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
			
			*(channelOne+even) = hypot( a1, b1 );
			*(channelOne+odd) = -atan2( b1, a1 );
			
			magnitude = *(channelTwo+even) = hypot( a2, b2 );
			*(channelTwo+odd) = -atan2( b2, a2 );
			
			/* use threshold for inverse filtering to avoid division by zero */
			
			if ( magnitude < threshold )
				magnitude = 0.;
			
			else  
				magnitude = 1. / magnitude;
			
			*(channelOne+even) *= magnitude;
			*(channelOne+even) = pow( *(channelOne+even), exponent );
		}
	}
	
	
	else {
		
		for ( i = 0; i <= N2; i++ ) {
			
			odd = ( even = i<<1 ) + 1;
			
			a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
			
			a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
			
			*(channelOne+even) = hypot( a1, b1 );
			*(channelOne+odd) = -atan2( b1, a1 );
			
			*(channelTwo+even) = hypot( a2, b2 );
			*(channelTwo+odd) = -atan2( b2, a2 );
			
			/* simple multiplication of magnitudes */
			
			*(channelOne+even) *= *(channelTwo+even);
			
			*(channelOne+even) = pow( *(channelOne+even), exponent );
		}
	}
	
	/* convert back to complex form, read for the inverse fft */
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	
	
	/* do an inverse fft */
	
	rdft( N, -1, bufferOne, bitshuffle, trigland );
	
	/* use slow inverse fft */
	
	//  rfft( bufferOne, N2, INVERSE );
	
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
	return (w+8);
}		


void taint_dsp(t_taint *x, t_signal **sp, short *count)
{
	long i;
	
#if MSP
	for( i = 0; i < 4; i++ ){
		x->connected[i] = count[i];
	}
#endif
	/* signal is always connected in Pd */
#if PD 
	for( i = 0; i < 4; i++ ){
		x->connected[i] = 1;
	}
#endif
	
	/* reinitialize if vector size or sampling rate has been changed */
	if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr){
		x->vs = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		taint_init(x,1);
	}	
	
	dsp_add(taint_perform, 7, x,
			sp[0]->s_vec,
			sp[1]->s_vec,
			sp[2]->s_vec,
			sp[3]->s_vec,
			sp[4]->s_vec,
			sp[0]->s_n);
}

