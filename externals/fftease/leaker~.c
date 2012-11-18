#include "MSPd.h"
#include "fftease.h"

#if MSP
void *leaker_class;
#endif 

#if PD
static t_class *leaker_class;
#endif

#define OBJECT_NAME "leaker~"


typedef struct _leaker
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

  int *sieve ;
  //
  int	inCount;
  float *Hwin;
  float *Wanal;	
  float *Wsyn;	
  float *output;
  /* leaker vars */

  float *c_lastphase_in1;
  float *c_lastphase_in2;
  float *c_lastphase_out;
  float c_fundamental;
  float c_factor_in;
  float c_factor_out;

  float mult; 
  float *trigland;
  int *bitshuffle;
  short mute;
  short bypass;
  short fade_connected;
  float fade_value;
  int overlap;
  int winfac;
} t_leaker;

static void leaker_free(t_leaker *x);
void *leaker_new(t_symbol *msg, short argc, t_atom *argv);
//t_int *offset_perform(t_int *w);
t_int *leaker_perform(t_int *w);
void leaker_dsp(t_leaker *x, t_signal **sp, short *count);
void leaker_assist(t_leaker *x, void *b, long m, long a, char *s);
void leaker_upsieve(t_leaker *x) ;
void leaker_downsieve(t_leaker *x) ;
void leaker_randsieve(t_leaker *x) ;
void leaker_bypass(t_leaker *x, t_floatarg state);
void leaker_mute(t_leaker *x, t_floatarg state);
void leaker_float(t_leaker *x, double f);
void leaker_init(t_leaker *x, short initialized);
void leaker_overlap(t_leaker *x, t_floatarg f);
void leaker_winfac(t_leaker *x, t_floatarg f);
void leaker_fftinfo(t_leaker *x);

#if MSP
void main(void)
{
  setup((t_messlist **)&leaker_class, (method)leaker_new, (method)leaker_free, 
	(short)sizeof(t_leaker), 0, A_GIMME, 0);
  addmess((method)leaker_dsp, "dsp", A_CANT, 0);
  addmess((method)leaker_assist,"assist",A_CANT,0);
  addmess((method)leaker_upsieve, "upsieve", 0);
  addmess((method)leaker_downsieve, "downsieve", 0);
  addmess((method)leaker_randsieve, "randsieve", 0);
  addmess((method)leaker_bypass,"bypass",A_DEFFLOAT,0);
  addmess((method)leaker_mute,"mute",A_DEFFLOAT,0);
  addmess((method)leaker_overlap,"overlap",A_DEFFLOAT,0);
  addmess((method)leaker_winfac,"winfac",A_DEFFLOAT,0);
  addmess((method)leaker_fftinfo,"fftinfo",0);
  addfloat((method)leaker_float);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void leaker_tilde_setup(void)
{
  leaker_class = class_new(gensym("leaker~"), (t_newmethod)leaker_new, 
    (t_method)leaker_free ,sizeof(t_leaker), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(leaker_class, t_leaker, x_f);
  class_addmethod(leaker_class, (t_method)leaker_dsp, gensym("dsp"), 0);
  class_addmethod(leaker_class, (t_method)leaker_mute, gensym("mute"), A_DEFFLOAT,0);
  class_addmethod(leaker_class, (t_method)leaker_bypass, gensym("bypass"), A_DEFFLOAT,0);
  class_addmethod(leaker_class, (t_method)leaker_mute, gensym("mute"), A_DEFFLOAT,0);
  class_addmethod(leaker_class, (t_method)leaker_overlap, gensym("overlap"), A_DEFFLOAT,0);
  class_addmethod(leaker_class, (t_method)leaker_winfac, gensym("winfac"), A_DEFFLOAT,0);
  class_addmethod(leaker_class, (t_method)leaker_fftinfo, gensym("fftinfo"),0);
  class_addmethod(leaker_class, (t_method)leaker_upsieve, gensym("upsieve"), 0);
  class_addmethod(leaker_class, (t_method)leaker_downsieve, gensym("downsieve"),0);
  class_addmethod(leaker_class, (t_method)leaker_randsieve, gensym("randsieve"),0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void leaker_overlap(t_leaker *x, t_floatarg f)
{
int i = (int) f;
  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
	x->overlap = i;
	leaker_init(x,1);
}

void leaker_winfac(t_leaker *x, t_floatarg f)
{
int i = (int)f;

  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = i;
	leaker_init(x,2);
}

void leaker_fftinfo(t_leaker *x)
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void leaker_free( t_leaker *x ){
#if MSP
  dsp_free((t_pxobject *)x);
#endif
  freebytes(x->c_lastphase_in1,0); 
  freebytes(x->c_lastphase_in2,0); 
  freebytes(x->c_lastphase_out,0); 
  freebytes(x->trigland,0); 
  freebytes(x->bitshuffle,0); 
  freebytes(x->Wanal,0); 
  freebytes(x->Wsyn,0);
  freebytes(x->input1,0);
  freebytes(x->input2,0);
  freebytes(x->Hwin,0);
  freebytes(x->buffer1,0);
  freebytes(x->buffer2,0);
  freebytes(x->channel1,0);
  freebytes(x->channel2,0);
  freebytes(x->output,0);
  freebytes(x->sieve,0);
}

void leaker_upsieve(t_leaker *x) {
  int i;
  for( i = 0; i < x->N2; i++ ){
    x->sieve[i] = i + 1;
  }
}

void leaker_downsieve(t_leaker *x) {
  int i;
  for( i = 0; i < x->N2; i++ ){
    x->sieve[i] = x->N2  - i;
  }
}

void leaker_randsieve(t_leaker *x) {
  int i,j;
  int NSwitch = 100000 ;
  int temp ;
  int pos1, pos2;

  // use better algorithm

  for( i = 0; i < x->N2; i++ ){
    x->sieve[i] = i + 1;
  }
  for( i = 0; i < NSwitch; i++ ){
    pos1 = rand() % x->N2;
    pos2 = rand() % x->N2;
    temp = x->sieve[pos2];
    x->sieve[pos2] = x->sieve[pos1];
    x->sieve[pos1] = temp ;
  }
}

void leaker_bypass(t_leaker *x, t_floatarg state)
{
  x->bypass = (short)state;	
}
void leaker_mute(t_leaker *x, t_floatarg state)
{
  x->mute = (short)state;	
}


void leaker_assist (t_leaker *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0: sprintf(dst,"(signal) Input 1");break;
    case 1: sprintf(dst,"(signal) Input 2");break;
    case 2: sprintf(dst,"(signal/float) Crossfade Position (0.0 - 1.0)");break;
    }
  } else if (msg==2) {
    sprintf(dst,"(signal) Output ");
  }
}

void *leaker_new(t_symbol *msg, short argc, t_atom *argv)
{
#if MSP
  t_leaker *x = (t_leaker *)newobject(leaker_class);
  dsp_setup((t_pxobject *)x,3);
  outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
  t_leaker *x = (t_leaker *)pd_new(leaker_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif

  x->D = sys_getblksize();
  x->R = sys_getsr();
  if(!x->D)
    x->D = 256;
  if(!x->R)
    x->R = 44100;

  x->overlap = atom_getfloatarg(0,argc,argv);
  x->winfac = atom_getfloatarg(1,argc,argv);
  if(!x->overlap)
    x->overlap = 4;
  if(!x->winfac)
    x->winfac = 1;

  leaker_init(x,0);	
  return (x);
}

void leaker_init(t_leaker *x, short initialized)
{
	int i;
	if(!power_of_two(x->overlap))
		x->overlap = 4;
	if(!power_of_two(x->winfac))
		x->winfac = 2;
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
	
	if(!initialized) {
		x->mute = 0;
		x->bypass = 0;
		x->fade_connected = 0;
		x->fade_value = 0;
		
		x->input1 = (float *) getbytes(MAX_Nw * sizeof(float));	
		x->buffer1 = (float *) getbytes(MAX_N * sizeof(float));
		x->channel1 = (float *) getbytes((MAX_N+2) * sizeof(float));
		x->input2 = (float *) getbytes(MAX_Nw * sizeof(float));	
		x->buffer2 = (float *) getbytes(MAX_N * sizeof(float));
		x->channel2 = (float *) getbytes((MAX_N+2) * sizeof(float));
		x->Wanal = (float *) getbytes(MAX_Nw * sizeof(float));	
		x->Wsyn = (float *) getbytes(MAX_Nw * sizeof(float));	
		x->Hwin = (float *) getbytes(MAX_Nw * sizeof(float));
		x->output = (float *) getbytes(MAX_Nw * sizeof(float));
		x->bitshuffle = (int *) getbytes (MAX_N * 2 * sizeof( int ));
		x->trigland = (float *) getbytes(MAX_N * 2 * sizeof( float ));	  
		x->sieve = (int *) getbytes((MAX_N2 + 1) * sizeof(int));
		x->c_lastphase_in1 = (float *) getbytes((MAX_N2+1) * sizeof(float));
		x->c_lastphase_in2 = (float *) getbytes((MAX_N2+1) * sizeof(float));
		x->c_lastphase_out = (float *) getbytes((MAX_N2+1) * sizeof(float));
		
	}  

		memset((char *)x->input1,0,x->Nw);
		memset((char *)x->input2,0,x->Nw);
		memset((char *)x->output,0,x->Nw);
		memset((char *)x->c_lastphase_in1,0,(x->N2+1) * sizeof(float));
		memset((char *)x->c_lastphase_in2,0,(x->N2+1) * sizeof(float));
		memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));

	init_rdft(x->N, x->bitshuffle, x->trigland);
	makehanning(x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
	if(initialized != 2){
		for(i = 0; i < x->N2; i++){
			x->sieve[i] = i;
		}
	}
}

t_int *leaker_perform(t_int *w)
{
  int i,j,odd,even;
  float a1,a2,b1,b2;

  t_leaker *x = (t_leaker *) (w[1]);
  t_float *in1 = (t_float *)(w[2]);
  t_float *in2 = (t_float *)(w[3]);
  t_float *in3 = (t_float *)(w[4]);
  t_float *out = (t_float *)(w[5]);
  t_int n = w[6];

  float fade_value = x->fade_value;	
  float *input1 = x->input1;
  float *input2 = x->input2;
  int inCount = x->inCount;
  int R = x->R;
  int N = x->N;
  int N2 = x->N2;
  int D = x->D;
  int Nw = x->Nw;
  float *Wanal = x->Wanal;
  float *Wsyn = x->Wsyn;
  float *output = x->output;
  float *buffer1 = x->buffer1;
  float *buffer2 = x->buffer2;
  float *channel1 = x->channel1;
  float *channel2 = x->channel2;
  int *sieve = x->sieve;
  int *bitshuffle = x->bitshuffle;
  float *trigland = x->trigland;
  float mult = x->mult;	

  /* dereference struncture  */	
  if( x->mute) {
    while(n--){
      *out++ = 0.;
    }
    return (w+7);
  }	
  if( x->bypass ) {
    while(n--){
      *out++ = *in1++;
    }
    return (w+7);
  } 

#if MSP
  if(x->fade_connected)
    fade_value = *in3++ * (float) N2;
#endif

#if PD
    fade_value = *in3++ * (float) N2;
#endif

  inCount += D;

  for ( j = 0 ; j < Nw - D ; j++ ){
    input1[j] = input1[j+D];
    input2[j] = input2[j+D];
  }
  for ( j = Nw - D; j < Nw; j++ ) {
    input1[j] = *in1++;
    input2[j] = *in2++;
  }

  fold(input1, Wanal, Nw, buffer1, N, inCount);		
  fold(input2, Wanal, Nw, buffer2, N, inCount);	
  rdft(N, 1, buffer1, bitshuffle, trigland);
  rdft(N, 1, buffer2, bitshuffle, trigland);


  for ( i = 0; i <= N2; i++ ) {
    odd = ( even = i<<1 ) + 1;
    if( fade_value <= 0 || fade_value < sieve[i]  ){
      a1 = ( i == N2 ? *(buffer1+1) : *(buffer1+even) );
      b1 = ( i == 0 || i == N2 ? 0. : *(buffer1+odd) );

      *(channel1+even) = hypot( a1, b1 ) ;
      *(channel1+odd) = -atan2( b1, a1 );
      *(buffer1+even) = *(channel1+even) * cos(*(channel1+odd));
      if ( i != N2 ){
	*(buffer1+odd) = -(*(channel1+even)) * sin(*(channel1+odd));
      }
    } else {
      a2 = ( i == N2 ? *(buffer2+1) : *(buffer2+even) );
      b2 = ( i == 0 || i == N2 ? 0. : *(buffer2+odd) );
      *(channel1+even) = hypot( a2, b2 ) ;
      *(channel1+odd) = -atan2( b2, a2 );
      *(buffer1+even) = *(channel1+even) * cos(*(channel1+odd) );
      if ( i != N2 ){
	*(buffer1+odd) = -(*(channel1+even)) * sin( *(channel1+odd) );
      }
    }
  }

  rdft( N, -1, buffer1, bitshuffle, trigland );
  overlapadd( buffer1, N, Wsyn, output, Nw, inCount);

  for ( j = 0; j < D; j++ )
    *out++ = output[j] * mult;

  for ( j = 0; j < Nw - D; j++ )
    output[j] = output[j+D];

  for ( j = Nw - D; j < Nw; j++ )
    output[j] = 0.;

  x->inCount = inCount % Nw;
  
  return (w+7);
}		

void leaker_dsp(t_leaker *x, t_signal **sp, short *count)
{
  long i;
#if MSP
  x->fade_connected = count[2];
#endif
  if(x->R != sp[0]->s_sr || x->D != sp[0]->s_n){
    x->R = sp[0]->s_sr;
    x->D = sp[0]->s_n;
    leaker_init(x,1);
  }
  
  dsp_add(leaker_perform, 6, x, 
	  sp[0]->s_vec,sp[1]->s_vec,sp[2]->s_vec,sp[3]->s_vec,sp[0]->s_n);
}
#if MSP
void leaker_float(t_leaker *x, double f)
{
  int inlet = x->x_obj.z_in;
  if( inlet == 2 && f >= 0 && f <= 1){
    x->fade_value = f * (float) x->N2;
  }
}
#endif

