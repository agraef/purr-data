#include "MSPd.h"
#include "fftease.h"

#if MSP
void *ether_class;
#endif
#if PD
static t_class *ether_class;
#endif

#define OBJECT_NAME "ether~"


/* Added a new inlet for the composite index */

typedef struct _ether
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
    
    float threshMult;
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
    	
} t_ether;


void *ether_new(t_symbol *s, int argc, t_atom *argv);
t_int *ether_perform(t_int *w);
void ether_dsp(t_ether *x, t_signal **sp, short *count);
void ether_assist(t_ether *x, void *b, long m, long a, char *s);
void ether_dest(t_ether *x, double f);
void ether_invert(t_ether *x, t_floatarg toggle);
void ether_init(t_ether *x, short initialized);
void ether_free(t_ether *x);
void ether_mute(t_ether *x, t_floatarg toggle);
void ether_fftinfo(t_ether *x);
void ether_winfac(t_ether *x, t_floatarg f);
void ether_overlap(t_ether *x, t_floatarg o);
void ether_tilde_setup(void);


#if MSP
void main(void)
{
    setup((t_messlist **)&ether_class, (method) ether_new, (method)ether_free, (short) sizeof(t_ether),
    		0, A_GIMME, 0);
  
    addmess((method)ether_dsp, "dsp", A_CANT, 0);
    addmess((method)ether_assist,"assist",A_CANT,0);    
    addfloat((method)ether_dest);
    addmess((method)ether_invert,"invert", A_FLOAT, 0);
    addmess((method)ether_mute,"mute", A_FLOAT, 0);
     addmess((method)ether_overlap,"overlap", A_FLOAT, 0);   
     addmess((method)ether_winfac,"winfac", A_FLOAT, 0);
     addmess((method)ether_fftinfo,"fftinfo",  0);
    dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);

}

void ether_dest(t_ether *x, double f)
{
	x->threshMult = (float) f;	
}
#endif

#if PD
void ether_tilde_setup(void)
{
  ether_class = class_new(gensym("ether~"), (t_newmethod)ether_new, 
			     (t_method)ether_free ,sizeof(t_ether), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(ether_class, t_ether, x_f);
  class_addmethod(ether_class, (t_method)ether_dsp, gensym("dsp"), 0);
  class_addmethod(ether_class, (t_method)ether_assist, gensym("assist"), 0);
  class_addmethod(ether_class, (t_method)ether_invert, gensym("invert"), A_FLOAT,0);
  class_addmethod(ether_class, (t_method)ether_overlap, gensym("overlap"), A_FLOAT,0);
  class_addmethod(ether_class, (t_method)ether_mute, gensym("mute"), A_FLOAT,0);
  class_addmethod(ether_class, (t_method)ether_winfac, gensym("winfac"), A_FLOAT,0);
  class_addmethod(ether_class, (t_method)ether_fftinfo, gensym("fftinfo"), A_CANT,0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif



/* diagnostic messages for Max */


void ether_assist (t_ether *x, void *b, long msg, long arg, char *dst)
{

  if (msg == 1) {

    switch (arg) {

    	case 0:		sprintf(dst,"(signal) Input One");break;
    	case 1:		sprintf(dst,"(signal) Input Two"); break;
    	case 2:		sprintf(dst,"(signal) Composite Index"); break;
    }
  }

  else {

    if (msg == 2)
      sprintf(dst,"(signal) Output");

  }
}

void ether_free(t_ether *x)
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

void ether_overlap(t_ether *x, t_floatarg df)
{
int o = (int)df;
  if(!power_of_two(o)){
    error("%d is not a power of two",o);
    return;
  }
  x->overlap = o;
  ether_init(x,1);
}

void ether_winfac(t_ether *x, t_floatarg f)
{
int wf = (int)f;
  if(!power_of_two(wf)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = wf;
  ether_init(x,1);
}

void ether_fftinfo( t_ether *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void *ether_new(t_symbol *s, int argc, t_atom *argv)
{

#if MSP
  t_ether 	*x = (t_ether *) newobject(ether_class);
  dsp_setup((t_pxobject *)x,3);
  outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
  t_ether *x = (t_ether *)pd_new(ether_class);
  /* add two additional signal inlets */
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif

/* optional arguments: overlap winfac */
	
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
  ether_init(x,0);
  
  return (x);
}

void ether_init(t_ether *x, short initialized)
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
		x->threshMult = 0.;
		x->Wanal = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->Wsyn = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->Hwin = (float *) getbytes( (MAX_Nw) * sizeof(float));
		x->inputOne = (float *) getbytes(MAX_Nw * sizeof(float));	
		x->inputTwo = (float *) getbytes(MAX_Nw  * sizeof(float));
		x->bufferOne = (float *) getbytes(MAX_N  * sizeof(float));
		x->bufferTwo = (float *) getbytes(MAX_N  * sizeof(float));
		x->channelOne = (float *) getbytes(MAX_N+2  * sizeof(float));
		x->channelTwo = (float *) getbytes(MAX_N+2  * sizeof(float));
		x->output = (float *) getbytes(MAX_Nw * sizeof(float));
		x->bitshuffle = (int *) getbytes(MAX_N * 2 * sizeof(int));
		x->trigland = (float *) getbytes(MAX_N * 2 * sizeof(float));
	} 
		memset((char *)x->inputOne,0,x->Nw * sizeof(float));
		memset((char *)x->inputTwo,0,x->Nw * sizeof(float));
		memset((char *)x->output,0,x->Nw * sizeof(float));

	init_rdft( x->N, x->bitshuffle, x->trigland);
	makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 1);
	
}


t_int *ether_perform(t_int *w)
{

  int		i,j,
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

  
/* get our inlets and outlets */
	
  t_ether *x = (t_ether *) (w[1]);
  t_float *inOne = (t_float *)(w[2]);
  t_float *inTwo = (t_float *)(w[3]);
  t_float *vec_threshMult = (t_float *)(w[4]);
  t_float *out = (t_float *)(w[5]);
  t_int n = w[6];
	
  short *connected = x->connected;
  
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
  
  if(connected[2]){
  	threshMult = *vec_threshMult;
  }
  else if ( x->threshMult != 0. ){
  	threshMult = x->threshMult;
  }
  else { 
	threshMult = 1.0;
  }
  
  if(x->mute){
	while(n--)
		*out++ = 0.0;
	return w+7;
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

/* use slow fft */


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
      *(channelTwo+odd) = -atan2( b2, a2 );

/* use simple threshold for inverse compositing */
    
      if ( *(channelOne+even) > *(channelTwo+even) * threshMult )
      	*(channelOne+even) = *(channelTwo+even);
    	
      if ( *(channelOne+odd) == 0. )
    	*(channelOne+odd) = *(channelTwo+odd);	 
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
      *(channelTwo+odd) = -atan2( b2, a2 );

/* use simple threshold for compositing */
    
      if ( *(channelOne+even) < *(channelTwo+even) * threshMult )
      	*(channelOne+even) = *(channelTwo+even);
    	
      if ( *(channelOne+odd) == 0. )
    	*(channelOne+odd) = *(channelTwo+odd);	 
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

void ether_mute(t_ether *x, t_floatarg toggle)
{
  x->mute = (short)toggle;
}

void ether_invert(t_ether *x, t_floatarg toggle)
{
  x->invert = (int)toggle;
}

void ether_dsp(t_ether *x, t_signal **sp, short *count)
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
    ether_init(x,1);
  }	
	dsp_add(ether_perform, 6, x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[2]->s_vec,
		sp[3]->s_vec,
		sp[0]->s_n);
}

