#include "MSPd.h"
#include "fftease.h"
#include "PenroseOscil.h"
#include "PenroseRand.h"

#define OBJECT_NAME "centerring~"
#define MAX_WARP 16.0

#if MSP
	void *centerring_class;
#endif
#if PD
	static t_class *centerring_class;
#endif

#define OBJECT_NAME "centerring~"

typedef struct _centerring
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
    int bufferLength;
    int recalc;
    int	seed;
    int *bitshuffle;
    
    float baseFreq;
    float constFreq;
    float bandFreq;
    float frameR;
    float *ringPhases;
    float *ringIncrements;
    float *sineBuffer;
    float *Wanal;	
    float *Wsyn;	
    float *inputOne;
    float *Hwin;
    float *bufferOne;
    float *channelOne;
    float *newChannel;
    float *newAmplitudes;
    float *output;
    float mult; 
    float *trigland;

	short connected[8];
	short mute;
	int overlap;//overlap factor
	int winfac;//window factor
	int vs;//vector size       	
} t_centerring;


/* msp function prototypes */

void *centerring_new(t_symbol *s, int argc, t_atom *argv);
t_int *centerring_perform(t_int *w);
void centerring_dsp(t_centerring *x, t_signal **sp, short *count);
void centerring_float(t_centerring *x, double myFloat);
void centerring_assist(t_centerring *x, void *b, long m, long a, char *s);
void centerring_dest(t_centerring *x, double f);
void centerring_messages(t_centerring *x, t_symbol *s, short argc, t_atom *argv);
void centerring_adjust( t_centerring *x );
void centerring_zerophases( t_centerring *x );
void centerring_randphases( t_centerring *x );
void centerring_free(t_centerring *x);
void centerring_init(t_centerring *x, short initialized);
void centerring_mute(t_centerring *x, t_floatarg toggle);
void centerring_overlap(t_centerring *x, t_floatarg o);
void centerring_fftinfo( t_centerring *x );
void centerring_winfac(t_centerring *x, t_floatarg f);


/* float input handling routine for shape width (MSP only) */
#if MSP
void centerring_float( t_centerring *x, t_floatarg df )
{
float myFloat = (float) df;
int inlet = x->x_obj.z_in;

	if ( inlet == 1 ) {
		x->baseFreq = myFloat;
		x->recalc = 1;
	}	
	if ( inlet == 2 ) {
		x->bandFreq = myFloat;
		x->recalc = 1;
	}
	if ( inlet == 3 ) {
		x->constFreq = myFloat;
		x->recalc = 1;
	}
}


void main(void)
{
    setup( (struct messlist **) &centerring_class, (method) centerring_new,
    		(method) centerring_free, (short) sizeof(t_centerring), 0,A_GIMME, 0);
    		
    addmess((method)centerring_dsp, "dsp", A_CANT, 0);
    addmess((method)centerring_assist,"assist",A_CANT,0);
	addmess((method)centerring_messages,"seed", A_GIMME, 0);    
	addmess((method)centerring_messages,"zerophases", A_GIMME, 0);
	addmess((method)centerring_messages,"randphases", A_GIMME, 0);
	addmess((method)centerring_mute,"mute", A_FLOAT, 0);
	addmess((method)centerring_overlap,"overlap", A_FLOAT, 0);
	addmess((method)centerring_winfac,"winfac", A_FLOAT, 0);
	addmess((method)centerring_fftinfo,"fftinfo", 0);
    addfloat((method)centerring_float);
    dsp_initclass();
    post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void centerring_tilde_setup(void)
{
  centerring_class = class_new(gensym("centerring~"), (t_newmethod)centerring_new, 
			  (t_method)centerring_free ,sizeof(t_centerring), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(centerring_class, t_centerring, x_f);
  class_addmethod(centerring_class, (t_method)centerring_dsp, gensym("dsp"), 0);
  class_addmethod(centerring_class, (t_method)centerring_assist, gensym("assist"), 0);
  class_addmethod(centerring_class, (t_method)centerring_messages, gensym("seed"), A_GIMME,0);
  class_addmethod(centerring_class, (t_method)centerring_messages, gensym("zerophases"), A_GIMME,0);
  class_addmethod(centerring_class, (t_method)centerring_messages, gensym("randphases"), A_GIMME,0);
  class_addmethod(centerring_class, (t_method)centerring_overlap, gensym("overlap"), A_FLOAT,0);
  class_addmethod(centerring_class, (t_method)centerring_winfac, gensym("winfac"), A_FLOAT,0);
  class_addmethod(centerring_class, (t_method)centerring_mute, gensym("mute"), A_FLOAT,0);
  class_addmethod(centerring_class, (t_method)centerring_fftinfo, gensym("fftinfo"),0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void centerring_messages(t_centerring *x, t_symbol *s, short argc, t_atom *argv)
{	

	if (s == gensym("seed"))
		x->seed = (int) atom_getfloatarg(0,argc,argv);

	if (s == gensym("zerophases")) 
		centerring_zerophases( x );
		
	if (s == gensym("randphases"))
		centerring_randphases( x );
}



/* diagnostic messages for Max */

void centerring_assist (t_centerring *x, void *b, long msg, long arg, char *dst)
{

  if (msg == 1) {

    switch (arg) {

    	case 0:		sprintf(dst,"(signal) Input");
			break;

    	case 1:		sprintf(dst,"(signal/float) Base Modulation Frequency");
			break;
			
		case 2:		sprintf(dst,"(signal/float) Frequency Deviation Bandwidth");
			break;

		case 3:		sprintf(dst,"(signal/float) Frequency Deviation Constant");
			break;		
    }
  }

  else {

    if (msg == 2)
      sprintf(dst,"(signal) Output");

  }
}


void *centerring_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_centerring 	*x = (t_centerring *) newobject(centerring_class);
  dsp_setup((t_pxobject *)x, 4);
  outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
  t_centerring *x = (t_centerring *)pd_new(centerring_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif

  
  /* optional arguments: baseFreq, bandFreq, constFreq, seed, overlap, winfac */
  x->overlap = x->winfac = 0;
  x->baseFreq = atom_getfloatarg(0,argc,argv);
  x->bandFreq = atom_getfloatarg(1,argc,argv);
  x->constFreq = atom_getfloatarg(2,argc,argv);  
  x->seed = atom_getfloatarg(3,argc,argv);
  x->overlap = atom_getfloatarg(4,argc,argv);
  x->winfac = atom_getfloatarg(5,argc,argv);  

  if(!power_of_two(x->overlap)){
	x->overlap = 4;
  }
  if(!power_of_two(x->winfac)){
	x->winfac = 1;
  }  

if(x->baseFreq <= 0.0)
	x->baseFreq = 1.;
if(x->bandFreq <= 0.0)
	x->bandFreq = .2;
if(x->constFreq <= 0)
	x->constFreq = 1.;
		
  x->vs = sys_getblksize();
  x->R = sys_getsr();

  centerring_init(x,0);
  return(x);
}

void centerring_init(t_centerring *x, short initialized)
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
  x->frameR = (float) x->R / (float) x->D;
  
  if(!initialized){
    x->mute = 0;
	x->bufferLength = 131072;
	x->recalc = 0;
    x->Wanal = (float *) calloc(MAX_Nw, sizeof(float));	
    x->Wsyn = (float *) calloc(MAX_Nw, sizeof(float));	
    x->Hwin = (float *) calloc(MAX_Nw, sizeof(float));
    x->inputOne = (float *) calloc(MAX_Nw, sizeof(float));	
    x->bufferOne = (float *) calloc(MAX_N, sizeof(float));
    x->channelOne = (float *) calloc((MAX_N+2), sizeof(float));
	x->newChannel = (float *) calloc(MAX_N+2, sizeof(float));
	x->newAmplitudes = (float *) calloc(((MAX_N2 + 1) * 16), sizeof(float) );
	x->ringPhases = (float *) calloc((MAX_N2 + 1), sizeof(float));
	x->ringIncrements = (float *) calloc((MAX_N2 + 1), sizeof(float));
	x->sineBuffer = (float *) calloc((x->bufferLength + 1), sizeof(float));
    x->output = (float *) calloc(MAX_Nw, sizeof(float));
    x->bitshuffle = (int *) calloc(MAX_N * 2, sizeof(int));
    x->trigland = (float *) calloc(MAX_N * 2, sizeof(float));
  } 
		memset((char *)x->inputOne,0,x->Nw);
		memset((char *)x->output,0,x->Nw);
		memset((char *)x->bufferOne,0,x->N);
		memset((char *)x->channelOne,0,x->N+2);
		memset((char *)x->newChannel,0,x->N+2);
		memset((char *)x->ringPhases,0,(x->N2+1));
		memset((char *)x->ringIncrements,0,(x->N2+1));
		memset((char *)x->newAmplitudes,0,(x->N2+1));
 
  init_rdft(x->N, x->bitshuffle, x->trigland);
  makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
  makeSineBuffer(x->sineBuffer, x->bufferLength);
  centerring_adjust(x);
  centerring_zerophases(x); 
}

void centerring_free(t_centerring *x)
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
  free(x->bufferOne);
  free(x->channelOne);
  free(x->output);
  free(x->newChannel);
  free(x->newAmplitudes);
  free(x->ringPhases);
  free(x->ringIncrements);
  free(x->sineBuffer);
}

void centerring_adjust( t_centerring *x ) {

	int		i;

	float	*ringIncrements = x->ringIncrements,
			*ringPhases = x->ringPhases;

/* initialize oscillator increments and phases */

    for (i=0; i < x->N2; i++) {

      *(ringIncrements+i) = frequencyToIncrement( x->frameR,
                                x->baseFreq *
                                ( (rrand(&(x->seed)) * x->bandFreq) + x->constFreq ),
                                x->bufferLength );
    }
}


void centerring_zerophases( t_centerring *x ) {

  int	i;

  for (i=0; i < x->N2; i++)
    *((x->ringPhases)+i) = 0.;
}


void centerring_randphases( t_centerring *x ) {

  int	i;

  for (i=0; i < x->N2; i++)
    *((x->ringPhases)+i) = prand(&(x->seed)) * (float) (x->bufferLength);

}



t_int *centerring_perform(t_int *w)
{

  int		
			i,j,
			bindex,
			inCount,
			R,
			N,
			N2,
			D,
			Nw,
  			newLength,
  			bufferLength,
   			even, odd,
  	 		*bitshuffle;

  float		mult,
  			bandFreq,
			constFreq,
			baseFreq,
			a1, b1,
			interpIncr,
			interpPhase,
  			*inputOne,
			*inputTwo,
			*bufferOne,
			*bufferTwo,
			*output,
			*Wanal,
			*Wsyn,
			*ringPhases,
            *ringIncrements,
            *sineBuffer,
			*channelOne,
			*newChannel,
			*newAmplitudes,
			*trigland;

	
  t_centerring *x = (t_centerring *) (w[1]);
  t_float *inOne = (t_float *) (w[2]);  
  t_float *vec_baseFreq = (t_float *) (w[3]);
  t_float *vec_bandFreq = (t_float *) (w[4]);
  t_float *vec_constFreq = (t_float *) (w[5]);
  t_float *out = (t_float *)(w[6]);
  t_int n = (int) (w[7]);
  short *connected = x->connected;
  
  if(connected[1]){
	x->recalc = 1;
	x->baseFreq = *vec_baseFreq;	
  }
  if(connected[2]){
	x->recalc = 1;
	x->bandFreq = *vec_bandFreq;	
  }
  if(connected[3]){
	x->recalc = 1;
	x->constFreq = *vec_constFreq;	
  }
  
  if(x->mute){
    while(n--)
      *out++ = 0.0;
    return (w+8);
  }
  	
/* dereference structure  */	

  inputOne = x->inputOne;
  bufferOne = x->bufferOne;
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
  newChannel = x->newChannel;
  newAmplitudes = x->newAmplitudes;
  bitshuffle = x->bitshuffle;
  trigland = x->trigland; 
  mult = x->mult;
  
  bufferLength = x->bufferLength;
  
  ringPhases = x->ringPhases;
  ringIncrements = x->ringIncrements;
  sineBuffer = x->sineBuffer;

 /* recalculate our oscillator values if object inputs have been updated */

 if (x->recalc)
    centerring_adjust( x );
 
 x->recalc = 0;
  
/* fill our retaining buffers */

  inCount += D;

  for ( j = 0 ; j < Nw - D ; j++ )
    inputOne[j] = inputOne[j+D];

  for ( j = Nw - D; j < Nw; j++ )
    inputOne[j] = *inOne++;

/* apply hamming window and fold our window buffer into the fft buffer */ 

  fold( inputOne, Wanal, Nw, bufferOne, N, inCount );

/* do an fft */ 

  rdft( N, 1, bufferOne, bitshuffle, trigland );

/* convert to polar coordinates from complex values */ 

  for ( i = 0; i <= N2; i++ ) {
      odd = ( even = i<<1 ) + 1;

      a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
      b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
    
/* replace signal one's phases with those of signal two */

      *(channelOne+even) = hypot( a1, b1 );
      *(channelOne+odd) = -atan2( b1, a1 );      
  }
		
  
/* perform ring modulation on successive fft frames */

   for (i=0; i < N2; i++) {
	 even = i<<1;

     *(channelOne+even) *= bufferOscil( ringPhases+i,
                   *(ringIncrements+i), sineBuffer, bufferLength );
   }
		
/* convert from polar to cartesian */	

  for ( i = 0; i <= N2; i++ ) {

    odd = ( even = i<<1 ) + 1;
      
    *(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );

    if ( i != N2 )
      *(bufferOne+odd) = (*(channelOne+even)) * -sin( *(channelOne+odd) );
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



void centerring_mute(t_centerring *x, t_floatarg toggle)
{
  x->mute = (short)toggle;
}

void centerring_overlap(t_centerring *x, t_floatarg o)
{
  if(!power_of_two((int)o)){
	error("%f is not a power of two",o);
    return;
  }
  x->overlap = (int)o;
  centerring_init(x,1);
}

void centerring_winfac(t_centerring *x, t_floatarg f)
{
  if(!power_of_two((int)f)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = (int)f;
	centerring_init(x,1);
}

void centerring_fftinfo( t_centerring *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void centerring_dsp(t_centerring *x, t_signal **sp, short *count)
{
	int i;

	#if MSP
	  for( i = 0; i < 4; i++ ){
		x->connected[i] = count[i];
	  }
	#endif

	#if PD 
	  for( i = 0; i < 4; i++ ){
		x->connected[i] = 1;
	  }
	#endif	
	
  if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr){
    x->vs = sp[0]->s_n;
    x->R = sp[0]->s_sr;
    centerring_init(x,1);
  }		

	dsp_add(centerring_perform, 7, x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[2]->s_vec,
		sp[3]->s_vec,
		sp[4]->s_vec,
		sp[0]->s_n);
}

