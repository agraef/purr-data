#include "MSPd.h"
#include "fftease.h"

#if MSP
void *morphine_class;
#endif
#if PD
static t_class *morphine_class;
#endif

#define OBJECT_NAME "morphine~"

/*

Added additional inlet for morphine index

-EL 

*/

typedef struct _pickme {

  int		bin;
  float		value;

} pickme;


typedef struct _morphine
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
    float morphIndex;
    float exponScale;
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
    pickme *picks;
    pickme *mirror;

  short connected[8];
  short mute;
  int overlap;//overlap factor
  int winfac;//window factor
  int vs;//vector size  

} t_morphine;


/* msp function prototypes */

void *morphine_new(t_symbol *s, int argc, t_atom *argv);
//t_int *offset_perform(t_int *w);
t_int *morphine_perform(t_int *w);
void morphine_dsp(t_morphine *x, t_signal **sp, short *count);
void morphine_assist(t_morphine *x, void *b, long m, long a, char *s);
void morphine_dest(t_morphine *x, double f);

int sortIncreasing( const void *a, const void *b );
//int qsortE (char *base_ptr, int total_elems, int size, int (*cmp)());
// avoid warning with legal function pointer prototype
int qsortE (char *base_ptr, int total_elems, int size, int (*cmp)(const void *a, const void *b));
void morphine_transition(t_morphine *x, t_floatarg f);
void morphine_free(t_morphine *x);
void morphine_mute(t_morphine *x, t_floatarg toggle);
void morphine_fftinfo(t_morphine *x);
void morphine_tilde_setup(void);
void morphine_overlap(t_morphine *x, t_floatarg o);
void morphine_winfac(t_morphine *x, t_floatarg o);
void morphine_init(t_morphine *x, short initialized);

int sortIncreasing( const void *a, const void *b )
{

  if ( ((pickme *) a)->value > ((pickme *) b)->value )
    return 1;

  if ( ((pickme *) a)->value < ((pickme *) b)->value )
    return -1;

  return 0;
}

#if MSP
void main(void)
{
    setup( (struct messlist **) &morphine_class, (void *) morphine_new, (method)morphine_free,
    		 (short) sizeof(t_morphine), 0, A_GIMME, 0);
  
    addmess((method)morphine_dsp, "dsp", A_CANT, 0);
    addmess((method)morphine_assist,"assist",A_CANT,0);    
    
    addmess((method)morphine_transition,"transition", A_FLOAT, 0);
  addmess((method)morphine_overlap,"overlap", A_FLOAT, 0);
  addmess((method)morphine_mute,"mute", A_FLOAT, 0);
  addmess((method)morphine_fftinfo,"fftinfo", 0);
  addmess((method)morphine_winfac,"winfac",A_FLOAT, 0);
  addfloat((method)morphine_dest);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
    dsp_initclass();
}

void morphine_dest(t_morphine *x, double f)
{
int inlet = x->x_obj.z_in;

	if(inlet == 2)
		x->morphIndex = f;
//	post("index now %f", x->morphIndex);
}
#endif

#if PD
void morphine_tilde_setup(void)
{
  morphine_class = class_new(gensym("morphine~"), (t_newmethod)morphine_new, 
			     (t_method)morphine_free ,sizeof(t_morphine), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(morphine_class, t_morphine, x_f);
  class_addmethod(morphine_class, (t_method)morphine_dsp, gensym("dsp"), 0);
  class_addmethod(morphine_class, (t_method)morphine_assist, gensym("assist"), 0);
  class_addmethod(morphine_class, (t_method)morphine_overlap, gensym("overlap"), A_FLOAT,0);
  class_addmethod(morphine_class, (t_method)morphine_winfac, gensym("winfac"), A_FLOAT,0);
  class_addmethod(morphine_class, (t_method)morphine_mute, gensym("mute"), A_FLOAT,0);
  class_addmethod(morphine_class, (t_method)morphine_transition, gensym("transition"), A_FLOAT,0);
  class_addmethod(morphine_class, (t_method)morphine_fftinfo, gensym("fftinfo"), A_CANT,0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

/* diagnostic messages for Max */

void morphine_assist (t_morphine *x, void *b, long msg, long arg, char *dst)
{

  if (msg == 1) {

    switch (arg) {

    	case 0:		sprintf(dst,"(signal) Input One"); break;
    	case 1:		sprintf(dst,"(signal) Input Two"); break;
    	case 2:		sprintf(dst,"(signal/float) Morph Index"); break;
    }
  }

  else {

    if (msg == 2)
      sprintf(dst,"(signal) output");

  }
}

void morphine_transition(t_morphine *x, t_floatarg f)
{	
	x->exponScale = (float)f;
}


void *morphine_new(t_symbol *s, int argc, t_atom *argv)
{

#if MSP
  t_morphine 	*x = (t_morphine *) newobject(morphine_class);
  dsp_setup((t_pxobject *)x,3);
  outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
  t_morphine *x = (t_morphine *)pd_new(morphine_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif

/* optional arguments: exponent scale, overlap, window factor */

  x->exponScale = atom_getfloatarg(0,argc,argv);
  x->overlap = atom_getfloatarg(1,argc,argv);
  x->winfac = atom_getfloatarg(2,argc,argv);

  if(!power_of_two(x->overlap)){
    x->overlap = 4;
  }
  if(!power_of_two(x->winfac)){
    x->winfac = 1;
  }
  		
  x->vs = sys_getblksize();
  x->R = sys_getsr();
 
  morphine_init(x,0);

  return (x);
}

void morphine_init(t_morphine *x, short initialized)
{
  int i;
int BIGGIE = 32768;
  
  x->D = x->vs;
  x->N = x->D * x->overlap;
  x->Nw = x->N * x->winfac;
  limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
  x->N2 = (x->N)>>1;
  x->Nw2 = (x->Nw)>>1;
  x->inCount = -(x->Nw);
  x->mult = 1. / (float) x->N;
  if(!initialized){
    x->morphIndex = 0.;
    x->mute = 0;
    x->Wanal = (float *) getbytes( (MAX_Nw) * sizeof(float));	
    x->Wsyn = (float *) getbytes( (MAX_Nw) * sizeof(float));	
    x->Hwin = (float *) getbytes( (MAX_Nw) * sizeof(float));
    x->inputOne = (float *) getbytes(MAX_Nw * sizeof(float));	
    x->inputTwo = (float *) getbytes(MAX_Nw * sizeof(float));
    x->bufferOne = (float *) getbytes(MAX_N * sizeof(float));
    x->bufferTwo = (float *) getbytes(MAX_N * sizeof(float));
    x->channelOne = (float *) getbytes(MAX_N+2 * sizeof(float));
    x->channelTwo = (float *) getbytes(MAX_N+2 * sizeof(float));
    x->output = (float *) getbytes(MAX_Nw * sizeof(float));
    x->bitshuffle = (int *) getbytes(MAX_N * 2 * sizeof(int));
    x->trigland = (float *) getbytes(MAX_N * 2 * sizeof(float));
    x->picks = (pickme *) getbytes(((MAX_N2)+1) * sizeof(pickme));
    x->mirror = (pickme *) getbytes(((MAX_N2)+1) * sizeof(pickme));

  } 
		memset((char *)x->inputOne,0,x->Nw * sizeof(float));
		memset((char *)x->inputTwo,0,x->Nw * sizeof(float));
		memset((char *)x->output,0,x->Nw * sizeof(float));
 
  init_rdft( x->N, x->bitshuffle, x->trigland);
  makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
  
}

t_int *morphine_perform(t_int *w)
{

  int		
			i,j,
			inCount,
			R,
			N,
			N2,
			D,
			Nw,
			lookupIndex,
  			even, odd,
  	 		*bitshuffle;

  float		maxamp,	
  			threshMult = 1.,
			mult,
			morphIndex,
			exponScale,
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

  pickme	*picks,
  			*mirror;

/* get our inlets and outlets */
	
  t_morphine *x = (t_morphine *) (w[1]);
  t_float *inOne = (t_float *)(w[2]);
  t_float *inTwo = (t_float *)(w[3]);
  t_float *vec_morphIndex = (t_float *)(w[4]);
  t_float *out = (t_float *)(w[5]);
  t_int n = w[6];

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
  picks = x->picks;
  mirror = x->mirror;
  morphIndex = x->morphIndex;
  exponScale = x->exponScale;
  
  morphIndex = connected[2] ? *vec_morphIndex : x->morphIndex;
//  post("connected %d index %f stored index %f",connected[2],morphIndex, x->morphIndex);
  if ( morphIndex < 0 )
  	morphIndex = 0.;
  else {
    if ( morphIndex > 1. )
    	morphIndex = 1.;
  }
  
  
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


/* find amplitude differences between home and visitors */

	  (picks+i)->value = fabs( *(channelOne+even) - 
	  						*(channelTwo+even) );
	  (picks+i)->bin = i;  
    }

/* sort our differences in ascending order */


	qsortE( (char *) picks, (int) N2+1, (int) sizeof(pickme),
		 sortIncreasing );

      /* now we create an effective mirror of the sorted distribution.
         we will assure that the initial transition will be made from
         small spectral differences (when the sort behavior is increasing)
         and the ending transition will also be made from small spectral
         differences */

      for ( i=0; i <= N2; i += 2 ) {
        (mirror+(i/2))->bin = (picks+i)->bin;
        (mirror+(i/2))->value = (picks+i)->value;
      }

      for ( i=1; i <= N2; i += 2 ) {
        (mirror+(N2-(i/2)))->bin = (picks+i)->bin;
        (mirror+(N2-(i/2)))->value = (picks+i)->value;
      }


/* calculate our morphIndex from an exponential function based on exponScale */
      
     if (exponScale == 0.) 
       lookupIndex = (int) (( (float) N2 ) * morphIndex);
       
     else {
     
     	if ( morphIndex < .5 ) {
     
          lookupIndex = (int) ( ((float) N2) * ((
          			 (1. - exp( exponScale * morphIndex * 2. )) /
    				 (1. - exp( exponScale )) ) * .5) );
    	}
    				 
    	else {
    	
    	  lookupIndex = (int) ( ((float) N2) * ( .5 +  
          			 (( (1. - exp( -exponScale * (morphIndex - .5) * 2. )) /
    				 (1. - exp( -exponScale )) ) * .5) ) );
    	}			 
    				 
      }
      
      
//      post("%d", lookupIndex);
      
/* choose the bins that are least different first */

    for ( i=0; i <= lookupIndex; i++ ) {

	  even = ((mirror+i)->bin)<<1,
	  odd = (((mirror+i)->bin)<<1) + 1;	

	  *(channelOne+even) = *(channelTwo+even);
	  *(channelOne+odd) = *(channelTwo+odd);
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
void morphine_free(t_morphine *x)
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
  freebytes(x->picks,0);
  freebytes(x->mirror,0);
  freebytes(x->output,0);
}

void morphine_fftinfo( t_morphine *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void morphine_overlap(t_morphine *x, t_floatarg df)
{
int o = (int)df;

  if(!power_of_two(o)){
    error("%d is not a power of two",o);
    return;
  }
  x->overlap = (int)o;
  morphine_init(x,1);
}

void morphine_winfac(t_morphine *x, t_floatarg df)
{
int wf = (int) df;
  if(!power_of_two(wf)){
    error("%d is not a power of two",wf);
    return;
  }
  x->winfac = wf;
  morphine_init(x,1);
}

void morphine_mute(t_morphine *x, t_floatarg toggle)
{
  x->mute = (short)toggle;
}

void morphine_dsp(t_morphine *x, t_signal **sp, short *count)
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
    morphine_init(x,1);
  }	

	dsp_add(morphine_perform, 6, x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[2]->s_vec,
		sp[3]->s_vec,
		sp[0]->s_n);
}



