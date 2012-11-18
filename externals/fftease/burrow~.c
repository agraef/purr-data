#include "MSPd.h"
#include "fftease.h"

#if MSP
	void *burrow_class;
#endif
#if PD
	static t_class *burrow_class;
#endif

#define OBJECT_NAME "burrow~"

/* after adding fixes, window factors > 1 are defective. Is there
a remaining bug, or is this a problem for FFT-only processors? */

/* A few changes:

Threshold and Multiplier now have their own
inlets, which accept (signal/float). The input
is now linear, rather than in dB. Reasons for this:

1) Linear input is the Max/MSP convention
2) It is easy to convert from linear to dB in Max
3) (My favorite) This cuts down on programmer overhead.
 
 */

typedef struct _burrow
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
    float multiplier;
    float mult;
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
    float *trigland;

	short connected[8];
	short mute;
	int overlap;//overlap factor
	int winfac;//window factor
	int vs;//vector size    	
} t_burrow;


/* msp function prototypes */

void *burrow_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *burrow_perform(t_int *w);
void burrow_dsp(t_burrow *x, t_signal **sp, short *count);
void burrow_assist(t_burrow *x, void *b, long m, long a, char *s);
void burrow_float(t_burrow *x, t_floatarg myFloat);
void burrow_init(t_burrow *x, short initialized);
void burrow_free(t_burrow *x);
void burrow_invert(t_burrow *x, t_floatarg toggle);
void burrow_mute(t_burrow *x, t_floatarg toggle);
void burrow_fftinfo(t_burrow *x);
void burrow_tilde_setup(void);
void burrow_overlap(t_burrow *x, t_floatarg o);
void burrow_winfac(t_burrow *x, t_floatarg f);


#if MSP
void main(void)
{
    setup((t_messlist **)&burrow_class,(method) burrow_new, 
(method)burrow_free, (short) sizeof(t_burrow),0, A_GIMME, 0);
    addmess((method)burrow_dsp, "dsp", A_CANT, 0);
    addmess((method)burrow_assist,"assist",A_CANT,0);    
    addmess((method)burrow_invert,"invert", A_FLOAT, 0);
	addmess((method)burrow_overlap,"overlap", A_FLOAT, 0);
	addmess((method)burrow_mute,"mute", A_FLOAT, 0);
    addmess((method)burrow_winfac,"winfac",A_FLOAT,0);
	addmess((method)burrow_fftinfo,"fftinfo", 0);
    addfloat((method)burrow_float);
    dsp_initclass();
    post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}

/* float input handling routines (MSP only) */

void burrow_float(t_burrow *x, t_floatarg myFloat)
{
int inlet = ((t_pxobject*)x)->z_in;
	if ( inlet == 2 ) // added two outlets so position is moved over
		x->threshold = myFloat; 

	if ( inlet == 3 )
		x->multiplier = myFloat;
}
#endif
#if PD
void burrow_tilde_setup(void)
{
  burrow_class = class_new(gensym("burrow~"), (t_newmethod)burrow_new, 
			  (t_method)burrow_free ,sizeof(t_burrow), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(burrow_class, t_burrow, x_f);
  class_addmethod(burrow_class, (t_method)burrow_dsp, gensym("dsp"), 0);
  class_addmethod(burrow_class, (t_method)burrow_assist, gensym("assist"), 0);
  class_addmethod(burrow_class, (t_method)burrow_invert, gensym("invert"), A_FLOAT,0);
  class_addmethod(burrow_class, (t_method)burrow_overlap, gensym("overlap"), A_FLOAT,0);
  class_addmethod(burrow_class, (t_method)burrow_mute, gensym("mute"), A_FLOAT,0);
  class_addmethod(burrow_class, (t_method)burrow_fftinfo, gensym("fftinfo"), A_CANT,0);
  class_addmethod(burrow_class,(t_method)burrow_winfac,gensym("winfac"),A_FLOAT,0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void burrow_free(t_burrow *x)
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


void burrow_invert(t_burrow *x, t_floatarg toggle)
{
	x->invert = toggle;
}

void burrow_mute(t_burrow *x, t_floatarg toggle)
{
  x->mute = toggle;
}

void burrow_overlap(t_burrow *x, t_floatarg o)
{
  if(!power_of_two(o)){
	error("%f is not a power of two",o);
    return;
  }
  x->overlap = o;
  burrow_init(x,1);
}

void burrow_winfac(t_burrow *x, t_floatarg f)
{
  if(!power_of_two(f)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = (int)f;
  burrow_init(x,1);
}

void burrow_fftinfo( t_burrow *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}



/* diagnostic messages for Max */

void burrow_assist (t_burrow *x, void *b, long msg, long arg, char *dst)
{

  if (msg == 1) {

    switch (arg) {
    	case 0: sprintf(dst,"(signal) Source Sound"); break;
		case 1: sprintf(dst,"(signal) Burrow Filtering Sound"); break;
		case 2: sprintf(dst,"(signal/float) Filter Threshold"); break;
		case 3: sprintf(dst,"(signal/float) Filter Multiplier"); break;
    }
  }

  else {
    if (msg == 2)
      sprintf(dst,"(signal) Output");
  }
}

void burrow_init(t_burrow *x, short initialized)
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
    x->invert = 0;
    x->inputOne = (float *) calloc(MAX_Nw, sizeof(float));	
    x->inputTwo = (float *) calloc(MAX_Nw, sizeof(float));
    x->bufferOne = (float *) calloc(MAX_N, sizeof(float));
    x->bufferTwo = (float *) calloc(MAX_N, sizeof(float));
    x->channelOne = (float *) calloc((MAX_N+2), sizeof(float));
    x->channelTwo = (float *) calloc((MAX_N+2), sizeof(float));
    x->Wanal = (float *) calloc(MAX_Nw, sizeof(float));	
    x->Wsyn = (float *) calloc(MAX_Nw, sizeof(float));	
    x->Hwin = (float *) calloc(MAX_Nw, sizeof(float));
    x->output = (float *) calloc(MAX_Nw, sizeof(float));
    x->bitshuffle = (int *) calloc(MAX_N * 2, sizeof(int));
    x->trigland = (float *) calloc(MAX_N * 2, sizeof(float));
  } 
	memset((char *)x->inputOne,0,x->Nw * sizeof(float));
	memset((char *)x->inputTwo,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->bufferOne,0,x->N * sizeof(float));
	memset((char *)x->bufferTwo,0,x->N * sizeof(float));
  
  makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
  init_rdft( x->N, x->bitshuffle, x->trigland);
}

void *burrow_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_burrow 	*x = (t_burrow *) newobject(burrow_class);
  dsp_setup((t_pxobject *)x,4);
  outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
  t_burrow *x = (t_burrow *)pd_new(burrow_class);
  /* add three additional signal inlets */
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif

/* optional arguments: threshold, multiplier, overlap, winfac */

  x->threshold = atom_getfloatarg(0,argc,argv);
  x->multiplier = atom_getfloatarg(1,argc,argv);
  x->overlap = atom_getfloatarg(2,argc,argv);
  x->winfac = atom_getfloatarg(3,argc,argv);
  
  if(!power_of_two(x->overlap)){
	x->overlap = 4;
  }
  if(!power_of_two(x->winfac)){
	x->winfac = 1;
  }
  if(x->threshold > 1.0 || x->threshold < 0.0){
	x->threshold = 0;
  }
  if(x->multiplier > 1.0 || x->multiplier < 0.0){
	x->multiplier = .01;
  }

 x->vs = sys_getblksize();
 x->R = sys_getsr();
 
 burrow_init(x,0);
 return(x);
 
}


t_int *burrow_perform(t_int *w)
{
/* get our inlets and outlets */
	
  t_burrow *x = (t_burrow *) (w[1]);
  t_float *inOne = (t_float *)(w[2]);
  t_float *inTwo = (t_float *)(w[3]);
  t_float *flt_threshold = (t_float *)(w[4]);
  t_float *flt_multiplier = (t_float *)(w[5]);
  t_float *out = (t_float *)(w[6]);
  t_int n = w[7];

  short *connected = x->connected;
  
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
  			multiplier = 1.,
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
  multiplier = x->multiplier;	
  threshold = x->threshold;
  mult = x->mult;
  invert = x->invert;
  
  if(connected[2]){
	threshold = *flt_threshold;
  } else {
	threshold = x->threshold;
  }
  
  if(connected[3]){
	multiplier = *flt_multiplier;
  } else {
	multiplier = x->multiplier;
  }

/* save some CPUs if muted */
  if(x->mute){
    while(n--)
      *out++ = 0.0;
    return (w+8);
  }
    
/* fill our retaining buffers */

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


if (invert) {

/* convert to polar coordinates from complex values */
 
    for ( i = 0; i <= N2; i++ ) {
    
      odd = ( even = i<<1 ) + 1;

      a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
      b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
    
      a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
      b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );

      *(channelOne+even) = hypot( a1, b1 );
      *(channelOne+odd) = -atan2( b1, a1 );
    
      *(channelTwo+even) = hypot( a2, b2 );

	/* use simple threshold from second signal to trigger filtering */
      
      if ( *(channelTwo+even) < threshold )
      	*(channelOne+even) *= multiplier;
      
/*      *(channelTwo+odd) = -atan2( b2, a2 );  */

    }  
}

else {

/* convert to polar coordinates from complex values */
 
    for ( i = 0; i <= N2; i++ ) {
    
      odd = ( even = i<<1 ) + 1;

      a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
      b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
    
      a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
      b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );

      *(channelOne+even) = hypot( a1, b1 );
      *(channelOne+odd) = -atan2( b1, a1 );
    
      *(channelTwo+even) = hypot( a2, b2 );

	/* use simple threshold from second signal to trigger filtering */
      
      if ( *(channelTwo+even) > threshold )
      	*(channelOne+even) *= multiplier;
      
/*      *(channelTwo+odd) = -atan2( b2, a2 );  */

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

void burrow_dsp(t_burrow *x, t_signal **sp, short *count)
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
    burrow_init(x,1);
  }	
	dsp_add(burrow_perform, 7, x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[2]->s_vec,
		sp[3]->s_vec,
		sp[4]->s_vec,
		sp[0]->s_n);
}

