#include "MSPd.h"
#include "fftease.h"

#if MSP
void *drown_class;
#endif 

#if PD
static t_class *drown_class;
#endif

#define OBJECT_NAME "drown~"

typedef struct _drown
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
  float *Wanal;	
  float *Wsyn;	
  float *input;	
  float *Hwin;
  float *buffer;
  float *channel;
  float *output;
  float drownmult;
  // float threshgen;
  short mute;
  float threshold;
  // faster FFT
  float mult; 
  float *trigland;
  int *bitshuffle;
      
  short connected[8];
  int overlap;//overlap factor
  int winfac;//window factor
  int vs;//vector size
  
  short peakflag;// means threshold is relative to peak amp
  //
} t_drown;

void *drown_new(t_symbol *s, int argc, t_atom *argv);
void drown_mute(t_drown *x, t_floatarg toggle);
void drown_rel2peak(t_drown *x, t_floatarg toggle);
//t_int *offset_perform(t_int *w);
t_int *drown_perform(t_int *w);
void drown_dsp(t_drown *x, t_signal **sp, short *count);
void drown_assist(t_drown *x, void *b, long m, long a, char *s);
void nudist( float *S, float *C, float threshold, float fmult, int N2 );
void drown_float(t_drown *x, double f);
void drown_overlap(t_drown *x, t_floatarg o);
void drown_free(t_drown *x);
void drown_init(t_drown *x, short initialized);
void drown_overlap(t_drown *x, t_floatarg f);
void drown_winfac(t_drown *x, t_floatarg f);
void drown_fftinfo(t_drown *x);

#if MSP
void main(void)
{
  setup((t_messlist **)&drown_class, (method)drown_new, (method)drown_free, 
	(short)sizeof(t_drown), 0, A_GIMME, 0);
  addmess((method)drown_dsp, "dsp", A_CANT, 0);
  addmess((method)drown_assist,"assist",A_CANT,0);
  addmess((method)drown_mute,"mute",A_FLOAT,0);
  addmess((method)drown_rel2peak,"rel2peak",A_FLOAT,0);
  addmess((method)drown_overlap,"overlap",A_FLOAT,0);
  addmess((method)drown_winfac,"winfac",A_FLOAT,0);
  addmess((method)drown_fftinfo,"fftinfo",0);
  addfloat((method)drown_float);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if MSP
void drown_float(t_drown *x, double f) // Look at floats at inlets
{

  int inlet = x->x_obj.z_in;
//  post("drown: incoming float: %f",f);
//  post("inlet %d",inlet);
  if (inlet == 1)
    {
      x->threshold = f;
    }
  if (inlet == 2)
    {
      x->drownmult = f;
    }
}
#endif
#if PD
void drown_tilde_setup(void)
{
  drown_class = class_new(gensym("drown~"), (t_newmethod)drown_new, 
			  (t_method)drown_free ,sizeof(t_drown), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(drown_class, t_drown, x_f);
  class_addmethod(drown_class, (t_method)drown_dsp, gensym("dsp"), 0);
  class_addmethod(drown_class, (t_method)drown_assist, gensym("assist"), 0);
  class_addmethod(drown_class, (t_method)drown_mute, gensym("mute"), A_FLOAT,0);
  class_addmethod(drown_class, (t_method)drown_overlap, gensym("overlap"), A_FLOAT,0);
  class_addmethod(drown_class, (t_method)drown_rel2peak, gensym("rel2peak"), A_FLOAT,0);
  class_addmethod(drown_class, (t_method)drown_overlap, gensym("overlap"), A_FLOAT,0);
  class_addmethod(drown_class, (t_method)drown_winfac, gensym("winfac"), A_FLOAT,0);
  class_addmethod(drown_class, (t_method)drown_fftinfo, gensym("fftinfo"), 0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif



void drown_overlap(t_drown *x, t_floatarg f)
{
int i = (int) f;
  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
	x->overlap = i;
	drown_init(x,1);
}

void drown_winfac(t_drown *x, t_floatarg f)
{
int i = (int)f;

  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = i;
	drown_init(x,2);
}

void drown_fftinfo(t_drown *x)
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void drown_rel2peak(t_drown *x, t_floatarg toggle)
{
  x->peakflag = (short)toggle;
}

void drown_mute(t_drown *x, t_floatarg toggle)
{
  x->mute = (short)toggle;
}

void drown_assist (t_drown *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0: sprintf(dst,"(signal) Input"); break;
    case 1: sprintf(dst,"(signal/float) Threshold Generator"); break;
    case 2: sprintf(dst,"(signal/float) Multiplier for Weak Bins"); break;
    }
  } else if (msg==2) {
    sprintf(dst,"(signal) Output");
  }
}

void *drown_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_drown *x = (t_drown *)newobject(drown_class);
  dsp_setup((t_pxobject *)x,3);
  outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
  t_drown *x = (t_drown *)pd_new(drown_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif

  x->threshold = atom_getfloatarg(0,argc,argv);
  x->drownmult = atom_getfloatarg(1,argc,argv);
  x->overlap = atom_getfloatarg(2,argc,argv);
  x->winfac = atom_getfloatarg(3,argc,argv);

  if(x->threshold <= 0)
    x->threshold = .0001;
  if(x->drownmult <= 0)
    x->drownmult = 0.1;
  if(!power_of_two(x->overlap))
    x->overlap = 4;
  if(!power_of_two(x->winfac))
    x->winfac = 1;
  	
  x->vs = sys_getblksize();
  x->R = sys_getsr();
  
  drown_init(x,0);
  return (x);
}

void drown_init(t_drown *x, short initialized)
{
	int i;
	int mem;
	
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
		x->peakflag = 0;
		mem = (MAX_Nw) * sizeof(float);
		x->input = (float *) getbytes(mem);	
		x->output = (float *) getbytes(mem);
		x->Wanal = (float *) getbytes(mem);	
		x->Wsyn = (float *) getbytes(mem);	
		x->Hwin = (float *) getbytes(mem);
		mem = (MAX_N) * sizeof(float);
		x->buffer = (float *) getbytes(mem); 
		mem = (MAX_N+2) * sizeof(float);
		x->channel = (float *) getbytes(mem);
		mem = (MAX_N) * sizeof(int);
		x->bitshuffle = (int *) getbytes(mem);
		mem = (MAX_N) * sizeof(float);
		x->trigland = (float *) getbytes(mem);
	} 
		memset((char *)x->input,0,x->Nw * sizeof(float));
		memset((char *)x->output,0,x->Nw * sizeof(float));

	makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
	init_rdft( x->N, x->bitshuffle, x->trigland);
}

void drown_free(t_drown *x)
{
#if MSP
  dsp_free((t_pxobject *) x);
#endif
  freebytes(x->trigland,0);
  freebytes(x->bitshuffle,0);
  freebytes(x->Wanal,0);
  freebytes(x->Wsyn,0);
  freebytes(x->input,0);
  freebytes(x->Hwin,0);
  freebytes(x->buffer,0);
  freebytes(x->channel,0);
  freebytes(x->output,0);
}

t_int *drown_perform(t_int *w)
{
  int	i,j;
  float frame_peak = 0.0;
  float local_thresh;
	
  t_drown *x = (t_drown *) (w[1]);
  t_float *in = (t_float *)(w[2]);
  t_float *in2 = (t_float *)(w[3]);
  t_float *in3 = (t_float *)(w[4]);
  t_float *out = (t_float *)(w[5]);
  t_int n = w[6];

  float *Wanal = x->Wanal;
  float *Wsyn = x->Wsyn;
  float *input = x->input;
  float *Hwin = x->Hwin;
  float *buffer = x->buffer;
  float *channel = x->channel;
  float *output = x->output;

  int D = x->D;
  int I = D;
  int R = x->R;
  int Nw = x->Nw;
  int N = x->N ;
  int N2 = x-> N2;
  int Nw2 = x->Nw2;
  int *bitshuffle = x->bitshuffle;
  float *trigland = x->trigland;
  float mult = x->mult;
  int inCount = x->inCount ;  
  int on = inCount;
  float threshold = x->threshold;
  float drownmult = x->drownmult;
  short *connected = x->connected;
  // get multiplier from sig inlet

  /* dereference struncture  */	
	
	
  if( x->mute ){
    while( n-- ){
      *out++ = 0.0;
    }
    return (w+7);
  }
	
  if( connected[1] )
    threshold = *in2++ ;
		
  if( connected[2] )
    drownmult = *in3++ ;
	
  inCount += D;


  for ( j = 0 ; j < Nw - D ; j++ ){
    input[j] = input[j+D];
  }
  for ( j = Nw - D; j < Nw; j++ ) {
    input[j] = *in++;
  }

  fold( input, Wanal, Nw, buffer, N, inCount );	

  rdft(N, 1, buffer, bitshuffle, trigland);
  
  if( x->peakflag ){
  	leanconvert( buffer, channel, N2);
  	for(i = 0; i <N; i+= 2){	
  		if(frame_peak < channel[i])
  			frame_peak = channel[i];
  	}
  	local_thresh = frame_peak * threshold;
  	for(i = 0; i <N; i+= 2){	
  		if(channel[i] < local_thresh)
  			channel[i]  *= drownmult;
  	}  	
  	leanunconvert( channel, buffer, N2);
  } else {
 	 nudist( buffer, channel, threshold, drownmult, N2 );
  }
  rdft( N, -1, buffer, bitshuffle, trigland );

  overlapadd( buffer, N, Wsyn, output, Nw, inCount);

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

void nudist( float *S, float *C, float threshold, float fmult, int N2 )

{
  int real, imag, amp, phase;
  float a, b;
  int i;
  float maxamp = 1.;
  for ( i = 0; i <= N2; i++ ) {
    imag = phase = ( real = amp = i<<1 ) + 1;
    a = ( i == N2 ? S[1] : S[real] );
    b = ( i == 0 || i == N2 ? 0. : S[imag] );
    C[amp] = hypot( a, b );

    if ( (C[amp]) < threshold  ){
      C[amp] *= fmult;
    }
    C[phase] = -atan2( b, a );
  }

  for ( i = 0; i <= N2; i++ ) {
    imag = phase = ( real = amp = i<<1 ) + 1;
    S[real] = *(C+amp) * cos( *(C+phase) );
    if ( i != N2 )
      S[imag] = -*(C+amp) * sin( *(C+phase) );
  }
}

void drown_dsp(t_drown *x, t_signal **sp, short *count)
{
  long i;
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
    drown_init(x,1);
  }
  dsp_add(drown_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
	  sp[3]->s_vec, sp[0]->s_n);
}

