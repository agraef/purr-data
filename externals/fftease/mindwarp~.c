#include "MSPd.h"
#include "fftease.h"

#if MSP
void *mindwarp_class;
#endif
#if PD
static t_class *mindwarp_class;
#endif

#define OBJECT_NAME "mindwarp~"


#define MAX_WARP 16.0

/* 12.11.05 fixed divide-by-zero bug */

typedef struct _mindwarp
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
    int	warpConnected;
    int widthConnected;
    int *bitshuffle;
    
    float warpFactor;
    float shapeWidth;
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
} t_mindwarp;


/* msp function prototypes */

void *mindwarp_new(t_symbol *s, int argc, t_atom *argv);

t_int *mindwarp_perform(t_int *w);
void mindwarp_dsp(t_mindwarp *x, t_signal **sp, short *count);
void mindwarp_float(t_mindwarp *x, double myFloat);
void mindwarp_assist(t_mindwarp *x, void *b, long m, long a, char *s);
void mindwarp_dest(t_mindwarp *x, double f);
void mindwarp_init(t_mindwarp *x, short initialized);
void mindwarp_free(t_mindwarp *x);
void mindwarp_mute(t_mindwarp *x, t_floatarg toggle);
void mindwarp_fftinfo(t_mindwarp *x);
void mindwarp_overlap(t_mindwarp *x, t_floatarg o);
void mindwarp_winfac(t_mindwarp *x, t_floatarg o);
void mindwarp_tilde_setup(void);

#if MSP

void mindwarp_float( t_mindwarp *x, double df )
{
float myFloat = (float)df;

int inlet = x->x_obj.z_in;

//post("float input to mindwarp: %f",myFloat);

	if ( inlet == 1 ) {
	
		x->warpFactor = myFloat;
			
		if ( x->warpFactor > MAX_WARP )
			x->warpFactor = MAX_WARP;
				
		if ( x->warpFactor < (1. / MAX_WARP) )
			x->warpFactor = (1. / MAX_WARP);
	}	

	if ( inlet == 2 ) {

		if ( myFloat >= 1. && myFloat <= (double) x->N )
			x->shapeWidth = myFloat;
	}
	
}

void main(void)
{
    setup( (struct messlist **) &mindwarp_class, (method) mindwarp_new,
    		(method) mindwarp_free, (short) sizeof(t_mindwarp), 0, A_GIMME, 0);
    		
    addmess((method)mindwarp_dsp, "dsp", A_CANT, 0);
    addmess((method)mindwarp_assist,"assist",A_CANT,0);
    addmess((method)mindwarp_mute,"mute", A_FLOAT, 0);
     addmess((method)mindwarp_overlap,"overlap", A_FLOAT, 0);
     addmess((method)mindwarp_winfac,"winfac", A_FLOAT, 0);
     addmess((method)mindwarp_fftinfo,"fftinfo", 0);  
    addfloat((method)mindwarp_float);
    dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void mindwarp_tilde_setup(void)
{
  mindwarp_class = class_new(gensym("mindwarp~"), (t_newmethod)mindwarp_new, 
			     (t_method)mindwarp_free ,sizeof(t_mindwarp), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(mindwarp_class, t_mindwarp, x_f);
  class_addmethod(mindwarp_class, (t_method)mindwarp_dsp, gensym("dsp"), 0);
  class_addmethod(mindwarp_class, (t_method)mindwarp_assist, gensym("assist"), 0);
  class_addmethod(mindwarp_class, (t_method)mindwarp_overlap, gensym("overlap"), A_FLOAT,0);
 class_addmethod(mindwarp_class, (t_method)mindwarp_winfac, gensym("winfac"), A_FLOAT,0);

  class_addmethod(mindwarp_class, (t_method)mindwarp_mute, gensym("mute"), A_FLOAT,0);
  class_addmethod(mindwarp_class, (t_method)mindwarp_fftinfo, gensym("fftinfo"), A_CANT,0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif



/* diagnostic messages for Max */

void mindwarp_assist (t_mindwarp *x, void *b, long msg, long arg, char *dst)
{

  if (msg == 1) {

    switch (arg) {

    	case 0:		sprintf(dst,"(signal) Formant Input");
			break;

    	case 1:		sprintf(dst,"(signal/float) Warp Factor");
			break;
			
		case 2:		sprintf(dst,"(signal/float) Shape Width");
			break;		
    }
  }

  else {

    if (msg == 2)
      sprintf(dst,"(signal) Mindwarp Output");

  }
}


void *mindwarp_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_mindwarp 	*x = (t_mindwarp *) newobject(mindwarp_class);
  dsp_setup((t_pxobject *)x, 3);
  outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
  t_mindwarp *x = (t_mindwarp *)pd_new(mindwarp_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif
  
  
/* args: warpfactor, shape width, overlap, window factor */


  x->warpFactor = atom_getfloatarg(0,argc,argv);
  x->shapeWidth = atom_getfloatarg(1,argc,argv);
  x->overlap = atom_getfloatarg(2,argc,argv);
  x->winfac = atom_getfloatarg(3,argc,argv);

  if(!power_of_two(x->overlap)){
    x->overlap = 4;
  }
  if(!power_of_two(x->winfac)){
    x->winfac = 1;
  }
  
  if(x->warpFactor <= 0 || x->warpFactor > 100.0)
  	x->warpFactor = 1.0;
  if(x->shapeWidth <= 0 || x->shapeWidth > 64)
  	x->shapeWidth = 3.0;
  
  x->vs = sys_getblksize();
  x->R = sys_getsr();
  mindwarp_init(x,0);

  return (x);

}

void mindwarp_init(t_mindwarp *x, short initialized)
{
	
	
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
		x->Wanal = (float *)  getbytes (MAX_Nw * sizeof(float));
		x->Wsyn = (float *)  getbytes (MAX_Nw * sizeof(float));
		x->Hwin = (float *)  getbytes (MAX_Nw * sizeof(float));
		x->inputOne = (float *)  getbytes (MAX_Nw * sizeof(float));
		x->bufferOne = (float *)  getbytes (MAX_N * sizeof(float));
		x->channelOne = (float *)  getbytes (MAX_N+2 * sizeof(float));
		x->newAmplitudes = (float *)  getbytes (((MAX_N2 + 1) * 16) * sizeof(float));
		x->newChannel = (float *)  getbytes ((MAX_N + 1) * sizeof(float));
		x->output = (float *)  getbytes (MAX_Nw * sizeof(float));
		x->bitshuffle = (int *)  getbytes (MAX_N * 2 * sizeof(int));
		x->trigland = (float *)  getbytes (MAX_N * 2 * sizeof(float));
	} 
	memset((char *)x->inputOne,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	
	
	init_rdft( x->N, x->bitshuffle, x->trigland);
	makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 1);
	
}

void mindwarp_free(t_mindwarp *x)
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
  freebytes(x->bufferOne,0);
  freebytes(x->channelOne,0);
  freebytes(x->newAmplitudes,0);
  freebytes(x->newChannel,0);
  freebytes(x->output,0);
}



t_int *mindwarp_perform(t_int *w)
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
			invert = 1,
  			shapeWidth,
  			remainingWidth,
  			newLength,
  			even, odd,
  	 		*bitshuffle;

  float		maxamp,	
  			threshMult = 1.,
  			warpFactor,
			mult,
			cutoff,
			filterMult,
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
			*channelOne,
			*newChannel,
			*newAmplitudes,
			*trigland;

/* get our inlets and outlets */
	
  t_mindwarp *x = (t_mindwarp *) (w[1]);
  t_float *inOne = (t_float *) (w[2]);
  t_float *vec_warpFactor = (t_float *) (w[3]);
  t_float *vec_shapeWidth = (t_float *) (w[4]);
  t_float *out = (t_float *)(w[5]);
  t_int n = w[6];
  
  short *connected = x->connected;

  if(x->mute){
	while(n--)
		*out++ = 0.0;
	return w+7;
  }
  
  warpFactor = connected[1] ? *vec_warpFactor : x->warpFactor;
  shapeWidth = connected[2] ? (int) (*vec_shapeWidth) : (int) x->shapeWidth;

  if(warpFactor <= 0.0){
  	warpFactor = 0.1;
  	error("zero warp factor reported");
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
  
  cutoff = (float) N2 * .9;
  filterMult = .00001;
    


	
	
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


  
  /* set the number of expected new amplitudes */
  if(warpFactor <= 0){
  	error("bad warp, resetting");
  	warpFactor = 1.0;
  }

  newLength = (int) ((float) N2 / warpFactor);
   
  if(newLength <= 0){
  	error("bad length: resetting");
  	newLength = 1.0;
  }

  interpIncr = (float) N2 / (float) newLength;

  interpPhase = 0.;


  /* do simple linear interpolation on magnitudes */

  for ( bindex=0; bindex < newLength; bindex++ ) {

	int		localbindex = ((int) interpPhase) << 1;

	float	lower = *(channelOne + localbindex),
			upper = *(channelOne + localbindex + 2),
			diff = interpPhase - ( (float) ( (int) interpPhase ) );

	*(newAmplitudes+bindex) = lower + ( ( upper - lower ) * diff );

	interpPhase += interpIncr;
  }


  
/* replace magnitudes with warped values */

  if (warpFactor > 1.) {

	  int	until = (int) ( cutoff / warpFactor );

	  for ( bindex=0; bindex < until; bindex++ ) {
	    register int	amp = bindex<<1;

	    *(newChannel+amp) = *(newAmplitudes+bindex);
	  }


	  /* filter remaining spectrum as spectral envelope has shrunk */

	  for ( bindex=until; bindex < N2; bindex++ ) {
	    register int	amp = bindex<<1;

	    *(newChannel+amp) *= filterMult;
	  }
  }


//OK
  
  /* spectral envelope has enlarged, no post filtering is necessary */
  
  else {

	for ( bindex=0; bindex <= N2; bindex++ ) {
	    register int	amp = bindex<<1;

	    *(newChannel+amp) = *(newAmplitudes+bindex);
	}
  }


  
/* constrain our shapeWidth value */

  if ( shapeWidth > N2 )
  	shapeWidth = N2;
  	
  if ( shapeWidth < 1 )
  	shapeWidth = 1;

/* lets just shape the entire signal by the shape width */


  for ( i=0; i < N; i += shapeWidth << 1 ) {
	  
	float       amplSum = 0.,
    		    freqSum = 0.,
          		factor = 1.0;

	for ( j = 0; j < shapeWidth << 1; j += 2 ) {

  		amplSum += *(newChannel+i+j);
  		freqSum += *(channelOne+i+j);
	}

	if (amplSum < 0.000000001)
		factor = 0.000000001;	
	
	/* this can happen, crashing external; now fixed.*/
		
	if( freqSum <= 0 ){
//		error("bad freq sum, resetting");
		freqSum = 1.0;
	}
	else	
		factor = amplSum / freqSum;

	for ( j = 0; j < shapeWidth << 1; j += 2 )
  		*(channelOne+i+j) *= factor;
  }

/* copy remaining magnitudes (fixed shadowed variable warning by renaming bindex)*/

  if ( (remainingWidth = N2 % shapeWidth) ) {

	int			lbindex = (N2 - remainingWidth) << 1;


	float       amplSum = 0.,
    		    freqSum = 0.,
          		factor;

	for ( j = 0; j < remainingWidth << 1; j += 2 ) {

  		amplSum += *(newChannel+lbindex+j);
  		freqSum += *(channelOne+lbindex+j);
	}

	if (amplSum < 0.000000001)
		factor = 0.000000001;	
		
	else	
		factor = amplSum / freqSum;

	for ( j = 0; j < remainingWidth << 1; j += 2 )
  		*(channelOne+bindex+j) *= factor;
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
  return (w+7);
}		

void mindwarp_mute(t_mindwarp *x, t_floatarg toggle)
{
  x->mute = (short)toggle;
}

void mindwarp_overlap(t_mindwarp *x, t_floatarg o)
{
  if(!power_of_two((int)o)){
    error("%f is not a power of two",o);
    return;
  }
  x->overlap = (int)o;
  mindwarp_init(x,1);
}

void mindwarp_winfac(t_mindwarp *x, t_floatarg f)
{
  if(!power_of_two((int)f)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = (int)f;
  mindwarp_init(x,1);
}

void mindwarp_fftinfo( t_mindwarp *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void mindwarp_dsp(t_mindwarp *x, t_signal **sp, short *count)
{
long i;
#if MSP
  for( i = 0; i < 3; i++ ){
    x->connected[i] = count[i];
  }
#endif
  /* signal is always connected in Pd */
#if PD 
  for( i = 0; i < 3; i++ ){
    x->connected[i] = 1;
  }
#endif

  /* reinitialize if vector size or sampling rate has been changed */
  if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr){
    x->vs = sp[0]->s_n;
    x->R = sp[0]->s_sr;
    mindwarp_init(x,1);
  }		
  
  dsp_add(mindwarp_perform, 6, x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[2]->s_vec,
		sp[3]->s_vec,
		sp[0]->s_n);
}

