#include "MSPd.h"
#include "fftease.h"

#if MSP
void *shapee_class;
#endif 

#if PD
static t_class *shapee_class;
#endif

#define OBJECT_NAME "shapee~"

typedef struct _shapee
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
    int widthConnected;
    int *bitshuffle;
    
    float shapeWidth;
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
} t_shapee;


/* msp function prototypes */

void *shapee_new(t_symbol *s, int argc, t_atom *argv);
t_int *shapee_perform(t_int *w);
void shapee_dsp(t_shapee *x, t_signal **sp, short *count);
void shapee_float(t_shapee *x, double myFloat);
void shapee_assist(t_shapee *x, void *b, long m, long a, char *s);
void shapee_init(t_shapee *x, short initialized);
void shapee_mute(t_shapee *x, t_floatarg state);
void shapee_free(t_shapee *x);
void shapee_overlap(t_shapee *x, t_floatarg o);
void shapee_winfac(t_shapee *x, t_floatarg wf);
void shapee_fftinfo(t_shapee *x);

/* first calling */

/* float input handling routine for shape width */
#if MSP
void shapee_float( t_shapee *x, double myFloat )
{

	if ( x->x_obj.z_in == 2 ) {

		if ( myFloat >= 1. && myFloat <= (double) x->N )
			x->shapeWidth = (float)myFloat;
	}	
}

void main(void)
{
    setup( (t_messlist **) &shapee_class, (method) shapee_new,
    		(method) shapee_free, (short) sizeof(t_shapee), 0, A_GIMME, 0);
    		
    addmess((method)shapee_dsp, "dsp", A_CANT, 0);
    addmess((method)shapee_assist,"assist",A_CANT,0);
    addfloat((method)shapee_float);	
    addmess((method)shapee_mute,"mute",A_FLOAT,0);
	addmess((method)shapee_overlap,"overlap",A_FLOAT,0);
	addmess((method)shapee_winfac,"winfac",A_FLOAT,0);
	addmess((method)shapee_fftinfo,"fftinfo",0);
    dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
  }
#endif

#if PD
void shapee_tilde_setup(void)
{
  shapee_class = class_new(gensym("shapee~"), (t_newmethod)shapee_new, 
			 (t_method)shapee_free ,sizeof(t_shapee), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(shapee_class, t_shapee, x_f);
  class_addmethod(shapee_class, (t_method)shapee_dsp, gensym("dsp"), 0);
  class_addmethod(shapee_class, (t_method)shapee_mute, gensym("mute"), A_DEFFLOAT,0);
  class_addmethod(shapee_class, (t_method)shapee_overlap, gensym("overlap"), A_DEFFLOAT,0);
  class_addmethod(shapee_class, (t_method)shapee_winfac, gensym("winfac"), A_DEFFLOAT,0);
  class_addmethod(shapee_class, (t_method)shapee_fftinfo, gensym("fftinfo"),0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif



/* diagnostic messages for Max */

void shapee_assist (t_shapee *x, void *b, long msg, long arg, char *dst)
{

  if (msg == 1) {
    switch (arg) {
    	case 0:		sprintf(dst,"(signal) Frequency Reference");break;
    	case 1:		sprintf(dst,"(signal) Amplitude Reference");break;
	  	case 2:		sprintf(dst,"(signal/float) Shape Width"); break;		
    }
  }

  else {

    if (msg == 2)
      sprintf(dst,"(signal) Output");
  }
}


void *shapee_new(t_symbol *s, int argc, t_atom *argv)
{
			
#if MSP
  t_shapee 	*x = (t_shapee *) newobject(shapee_class);
  dsp_setup((t_pxobject *)x, 3);
  outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
  t_shapee *x = (t_shapee *)pd_new(shapee_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
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
  shapee_init(x,0); 

  return (x);
}

void shapee_init(t_shapee *x, short initialized)
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
	  x->mute = 0;

	  x->Wanal = (float *) getbytes( MAX_Nw * sizeof(float) );	
	  x->Wsyn = (float *) getbytes( MAX_Nw * sizeof(float) );	
	  x->Hwin = (float *) getbytes( MAX_Nw * sizeof(float) ); 
	  x->inputOne = (float *) getbytes( MAX_Nw * sizeof(float) );
	  x->inputTwo = (float *) getbytes( MAX_Nw * sizeof(float) );	
	  x->bufferOne = (float *) getbytes( MAX_N  * sizeof(float) );
	  x->bufferTwo = (float *) getbytes( MAX_N  * sizeof(float) ); 
	  x->channelOne = (float *) getbytes(  (MAX_N+2) * sizeof(float) );
	  x->channelTwo = (float *) getbytes(  (MAX_N+2) * sizeof(float) );
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

void shapee_fftinfo(t_shapee *x)
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void shapee_mute(t_shapee *x, t_floatarg state)
{
	x->mute = (short)state;
}

void shapee_overlap(t_shapee *x, t_floatarg o)
{
int test = (int) o;
	if(!power_of_two(test)){
		post("%d is not a power of two",test);
		return;
	}
	x->overlap = test;
	shapee_init(x,1);
}

void shapee_winfac(t_shapee *x, t_floatarg wf)
{
int test = (int) wf;
if(!power_of_two(test)){
		post("%d is not a power of two",test);
		return;
}
x->winfac = test;
shapee_init(x,1);

}

t_int *shapee_perform(t_int *w)
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
  			shapeWidth,
  			remainingWidth,
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
			*inShape,
			*out;


/* get our inlets and outlets */
	
  t_shapee *x = (t_shapee *) (w[1]);
  inOne = (t_float *) (w[2]);
  inTwo = (t_float *) (w[3]);
  inShape = (t_float *) (w[4]);
  out = (t_float *)(w[5]);
  n = (t_int) (w[6]);

/* get our shapeWidth -- from either a signal our float input */
#if MSP
  shapeWidth = x->widthConnected ? (int) *inShape : (int) x->shapeWidth;
#endif
#if PD
  shapeWidth = (int) *inShape;
#endif	

/* dereference structure  */	
if(x->mute){
	while(n--) *out++ = 0.0;
	return w+7;
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
  
    

  if(shapeWidth < 1 || shapeWidth > N2)
	shapeWidth = 1;
	
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

  for ( i = 0; i <= N2; i++ ) {
      odd = ( even = i<<1 ) + 1;

      a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
      b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
    
      a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
      b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );

/* replace signal one's phases with those of signal two */

      *(channelOne+even) = hypot( a1, b1 );
      *(channelOne+odd) = -atan2( b1, a1 );
    
      *(channelTwo+even) = hypot( a2, b2 );
      *(channelTwo+odd) = -atan2( b2, a2 );       
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
          		factor;

	for ( j = 0; j < shapeWidth << 1; j += 2 ) {

  		amplSum += *(channelTwo+i+j);
  		freqSum += *(channelOne+i+j);
	}
		if(freqSum <= 0.001){
			freqSum = 1.0;
		}
	if (amplSum < 0.000000001)
		factor = 0.000000001;	
		
	else	
		factor = amplSum / freqSum;

	for ( j = 0; j < shapeWidth * 2; j += 2 )
  		*(channelOne+i+j) *= factor;
  }

/* copy remaining magnitudes */

  if ( (remainingWidth = N2 % shapeWidth) ) {

		int			bindex = (N2 - remainingWidth) << 1;


		float       amplSum = 0.,
	    		    freqSum = 0.,
	          		factor;

		for ( j = 0; j < remainingWidth * 2; j += 2 ) {

	  		amplSum += *(channelTwo+bindex+j);
	  		freqSum += *(channelOne+bindex+j);
		}
		if(freqSum <= 0.00001){
			freqSum = 1.0;
		}
		if (amplSum < 0.000000001)
			factor = 0.000000001;	
			
		else	
			factor = amplSum / freqSum;

		for ( j = 0; j < remainingWidth * 2; j += 2 )
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
void shapee_free( t_shapee *x )
{
#if MSP
  dsp_free( (t_pxobject *) x);
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

void shapee_dsp(t_shapee *x, t_signal **sp, short *count)
{
#if MSP	
	x->widthConnected = count[2];
#endif
	if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr){
		x->vs = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		shapee_init(x,1);
	}
	dsp_add(shapee_perform, 6, x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[2]->s_vec,
		sp[3]->s_vec,
		sp[0]->s_n);
}

