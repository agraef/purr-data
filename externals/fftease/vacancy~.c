#include "MSPd.h"
#include "fftease.h"

#if MSP
void *vacancy_class;
#endif
#if PD
static t_class *vacancy_class;
#endif

#define OBJECT_NAME "vacancy~"

/*
Added inlet for compositing threshold, which is now
given linearly, not in dB (since Max can do that).

-EL 02.10.2005

*/
typedef struct _vacancy
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
    int useRms;
    int swapPhase;
    int *bitshuffle;
    
    float threshold;
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
    	
} t_vacancy;


/* msp function prototypes */

void *vacancy_new(t_symbol *s, int argc, t_atom *argv);
t_int *vacancy_perform(t_int *w);
void vacancy_dsp(t_vacancy *x, t_signal **sp, short *count);
void vacancy_assist(t_vacancy *x, void *b, long m, long a, char *s);
void vacancy_dest(t_vacancy *x, double f);

void vacancy_rms(t_vacancy *x, t_floatarg f);
void vacancy_invert(t_vacancy *x, t_floatarg f);
void vacancy_swapphase(t_vacancy *x, t_floatarg f);

void vacancy_free(t_vacancy *x);
void vacancy_mute(t_vacancy *x, t_floatarg toggle);
void vacancy_fftinfo(t_vacancy *x);
void vacancy_tilde_setup(void);
void vacancy_overlap(t_vacancy *x, t_floatarg o);
void vacancy_winfac(t_vacancy *x, t_floatarg o);
void vacancy_init(t_vacancy *x, short initialized);

#if MSP
void main(void)
{
    setup((t_messlist **)&vacancy_class, (method)vacancy_new, (method)vacancy_free, 
    (short) sizeof(t_vacancy), 0, A_GIMME, 0);
  
    addmess((method)vacancy_dsp, "dsp", A_CANT, 0);
    addmess((method)vacancy_assist,"assist",A_CANT,0);    
    addmess((method)vacancy_rms,"rms", A_FLOAT, 0);
    addmess((method)vacancy_invert,"invert", A_FLOAT, 0);
	addmess((method)vacancy_swapphase,"swapphase", A_FLOAT, 0);
	addmess((method)vacancy_mute,"mute", A_FLOAT, 0);
	addmess((method)vacancy_overlap,"overlap", A_FLOAT, 0);
	addmess((method)vacancy_winfac,"winfac", A_FLOAT, 0);
	addmess((method)vacancy_fftinfo,"fftinfo", 0);
	addfloat((method)vacancy_dest);
    dsp_initclass();
    post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}

void vacancy_dest(t_vacancy *x, double f)
{
int inlet = x->x_obj.z_in;

	if(inlet == 2)
		x->threshold = (float) f;
}
#endif

#if PD
void vacancy_tilde_setup(void)
{
  vacancy_class = class_new(gensym("vacancy~"), (t_newmethod)vacancy_new, 
			     (t_method)vacancy_free ,sizeof(t_vacancy), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(vacancy_class, t_vacancy, x_f);
  class_addmethod(vacancy_class, (t_method)vacancy_dsp, gensym("dsp"), 0);
  class_addmethod(vacancy_class, (t_method)vacancy_assist, gensym("assist"), 0);
  class_addmethod(vacancy_class, (t_method)vacancy_invert, gensym("invert"), A_FLOAT,0);
  class_addmethod(vacancy_class, (t_method)vacancy_swapphase, gensym("swapphase"), A_FLOAT,0);
  class_addmethod(vacancy_class, (t_method)vacancy_overlap, gensym("overlap"), A_FLOAT,0);
  class_addmethod(vacancy_class, (t_method)vacancy_winfac, gensym("winfac"), A_FLOAT,0);
  class_addmethod(vacancy_class, (t_method)vacancy_mute, gensym("mute"), A_FLOAT,0);
  class_addmethod(vacancy_class, (t_method)vacancy_fftinfo, gensym("fftinfo"), A_CANT,0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}

#endif


void vacancy_rms(t_vacancy *x, t_floatarg f)
{
	x->useRms = (int) f;
}

void vacancy_invert(t_vacancy *x, t_floatarg f)
{
	x->invert = (int) f;
}

void vacancy_swapphase(t_vacancy *x, t_floatarg f)
{
	x->swapPhase = (int) f;
}


void vacancy_assist (t_vacancy *x, void *b, long msg, long arg, char *dst)
{

  if (msg == 1) {

    switch (arg) {

    	case 0:		sprintf(dst,"(signal) Input One"); break;
    	case 1:		sprintf(dst,"(signal) Input Two"); break;
    	case 2:		sprintf(dst,"(signal) Mixing Threshold"); break;
    }
  }

  else {

    if (msg == 2)
      sprintf(dst,"(signal) Output");

  }
}

void vacancy_fftinfo( t_vacancy *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void vacancy_mute(t_vacancy *x, t_floatarg toggle)
{
  x->mute = (short)toggle;
}

void vacancy_overlap(t_vacancy *x, t_floatarg o)
{
  if(!power_of_two(o)){
    error("%f is not a power of two",o);
    return;
  }
  x->overlap = (int)o;
  vacancy_init(x,1);
}

void vacancy_winfac(t_vacancy *x, t_floatarg f)
{
  if(!power_of_two(f)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = (int)f;
  vacancy_init(x,1);
}

void *vacancy_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_vacancy 	*x = (t_vacancy *) newobject(vacancy_class);
  dsp_setup((t_pxobject *)x,3);
  outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
  t_vacancy *x = (t_vacancy *)pd_new(vacancy_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif



/* optional arguments: overlap, window factor */

  x->overlap = atom_getfloatarg(0,argc,argv);
  x->winfac = atom_getfloatarg(1,argc,argv);

  if(!power_of_two(x->overlap)){
    x->overlap = 4;
  }
  if(!power_of_two(x->winfac)){
    x->winfac = 1;
  }
  		
  x->vs = sys_getblksize();
  x->R = sys_getsr();
 
  vacancy_init(x,0);

  return (x);
}

void vacancy_init(t_vacancy *x, short initialized)
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
    x->threshold = 0.;
    x->useRms = 1;
    x->swapPhase = 0;
	  x->Wanal = (float *) getbytes( MAX_Nw * sizeof(float) );	
	  x->Wsyn = (float *) getbytes( MAX_Nw * sizeof(float) );	
	  x->Hwin = (float *) getbytes( MAX_Nw * sizeof(float) ); 
	  x->inputOne = (float *) getbytes( MAX_Nw * sizeof(float) );
	  x->inputTwo = (float *) getbytes( MAX_Nw * sizeof(float) );	
	  x->bufferOne = (float *) getbytes( MAX_N * sizeof(float) );
	  x->bufferTwo = (float *) getbytes( MAX_N * sizeof(float) ); 
	  x->channelOne = (float *) getbytes( (MAX_N+2)  * sizeof(float) );
	  x->channelTwo = (float *) getbytes( (MAX_N+2)  * sizeof(float) );
	  x->output = (float *) getbytes( MAX_Nw * sizeof(float) );
	  x->bitshuffle = (int *) getbytes( MAX_N * 2 * sizeof( int ) );
	  x->trigland = (float *) getbytes( MAX_N * 2 * sizeof( float ) );
  }
	memset((char *)x->inputOne,0,x->Nw * sizeof(float));
	memset((char *)x->inputTwo,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	
  init_rdft( x->N, x->bitshuffle, x->trigland);
  makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
  
}

void vacancy_free(t_vacancy *x)
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

t_int *vacancy_perform(t_int *w)
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
			useRms = 1,
			swapPhase = 0,
  			even, odd,
  	 		*bitshuffle;

  float		maxamp,	
  			threshold = .001,
			mult,
			useme,
			rms = 0.,
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
	
  t_vacancy *x = (t_vacancy *) (w[1]);
  t_float *inOne = (t_float *)(w[2]);
  t_float *inTwo = (t_float *)(w[3]);
  t_float *vec_threshold = (t_float *)(w[4]);
  t_float *out = (t_float *)(w[5]);
  t_int n = (t_int)(w[6]);
	
  short *connected = x->connected;

  if(x->mute){
	while(n--)
		*out++ = 0.0;
	return w+7;
  }

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
  invert = x->invert;
  useRms = x->useRms;
  swapPhase = x->swapPhase;
  
  threshold = connected[2] ? *vec_threshold : x->threshold;
  
 /*
  a bug?? Threshold could be -90 to 90 (dB)
  
  if ( x->threshold > 0. )
  	threshold = x->threshold;
  */
  
  
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

  if (useRms) {

    rms = 0.;

	for ( i=0; i < Nw; i++ )
	  rms += *(inputOne+i) * *(inputOne+i);

	rms = sqrt( rms / Nw );

	useme = rms * threshold;
  }
  
  else
  	useme = threshold;


/* apply hamming window and fold our window buffer into the fft buffer */ 

  fold( inputOne, Wanal, Nw, bufferOne, N, inCount );
  fold( inputTwo, Wanal, Nw, bufferTwo, N, inCount );


/* do an fft */ 

  rdft( N, 1, bufferOne, bitshuffle, trigland );
  rdft( N, 1, bufferTwo, bitshuffle, trigland );

/* use slow fft */

//  rfft( bufferOne, N2, FORWARD );
//  rfft( bufferTwo, N2, FORWARD );


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
      *(channelTwo+odd) = -atan2( b2, a2 ); 
  }
  
  
  /* composite here please */

  if (invert) {

    if (swapPhase) {

      for ( i=0; i < N2; i+=2 ) {
		if ( *(channelOne+i) > useme && *(channelTwo+i) < *(channelOne+i) ) {
		  *(channelOne+i) = *(channelTwo+i);
		  *(channelOne+i+1) = *(channelTwo+i+1);
		}
      }
    }

    else {

      for ( i=0; i < N2; i+=2 ) {
		if ( *(channelOne+i) > useme && *(channelTwo+i) < *(channelOne+i) ) {
		  *(channelOne+i) = *(channelTwo+i);
		  
		  if ( *(channelOne+i+1) == 0. )
		    *(channelOne+i+1) = *(channelTwo+i+1);
		}
      }
    }
  }

  else {

    if (swapPhase) {

      for ( i=0; i < N2; i+=2 ) {
		if ( *(channelOne+i) < useme && *(channelTwo+i) > *(channelOne+i) ) {
		  *(channelOne+i) = *(channelTwo+i);
		  *(channelOne+i+1) = *(channelTwo+i+1);
		}
      }
    }

    else {

      for ( i=0; i < N2; i+=2 ) {

		if ( *(channelOne+i) < useme && *(channelTwo+i) > *(channelOne+i) ) {
		  *(channelOne+i) = *(channelTwo+i);
		  
		  if ( *(channelOne+i+1) == 0. )
		    *(channelOne+i+1) = *(channelTwo+i+1);
		}
      }
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
  return (w+7);
}		





void vacancy_dsp(t_vacancy *x, t_signal **sp, short *count)
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
    vacancy_init(x,1);
  }	

	dsp_add(vacancy_perform, 6, x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[2]->s_vec,
		sp[3]->s_vec,
		sp[0]->s_n);
}

