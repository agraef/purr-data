#include "MSPd.h"
#include "fftease.h"

#if MSP
void *multyq_class;
#endif 

#if PD
static t_class *multyq_class;
#endif

#define OBJECT_NAME "multyq~"

typedef struct _multyq
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
  //
  float mult; 
  float *trigland;
  int *bitshuffle;

  // for multyQ
  float cf1;
  float gainfac1;
  float bw1;
  float cf2;
  float gainfac2;
  float bw2;
  float cf3;
  float gainfac3;
  float bw3;
  float cf4;
  float gainfac4;
  float bw4;

  float *rcos;
  float *filt;
  float *freqs;
  int rcoslen;
  //
  short in2_connected;
  short in3_connected;
  short in4_connected;
  short in5_connected;
  short in6_connected;
  short in7_connected;
  short in8_connected;
  short in9_connected;
  short in10_connected;
  short in11_connected;
  short in12_connected;
  short in13_connected;
  short please_update;
  short always_update;
  short mute;
  short bypass;
  int overlap;
  int winfac;
} t_multyq;

void *multyq_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *multyq_perform(t_int *w);
void multyq_dsp(t_multyq *x, t_signal **sp, short *count);
void multyq_assist(t_multyq *x, void *b, long m, long a, char *s);
void multyq_bypass(t_multyq *x, t_floatarg state);
void multyq_mute(t_multyq *x, t_floatarg state);
void update_filter_function(t_multyq *x);
void multyq_float(t_multyq *x, double f);
void filtyQ( float *S, float *C, float *filtfunc, int N2 );
void multyq_init(t_multyq *x, short initialized);
void multyq_free(t_multyq *x);
void multyq_fftinfo(t_multyq *x);
void multyq_overlap(t_multyq *x, t_floatarg f);
void multyq_winfac(t_multyq *x, t_floatarg f);

#if MSP
void main(void)
{
  setup((t_messlist **)&multyq_class, (method)multyq_new, (method)multyq_free, 
  (short)sizeof(t_multyq), 0, A_GIMME, 0);
  addmess((method)multyq_dsp, "dsp", A_CANT, 0);
  addmess((method)multyq_assist,"assist",A_CANT,0);
  addmess((method)multyq_bypass,"bypass",A_DEFFLOAT,0);
  addmess((method)multyq_mute,"mute",A_DEFFLOAT,0);
  addmess((method)multyq_overlap,"overlap",A_DEFFLOAT,0);
  addmess((method)multyq_winfac,"winfac",A_DEFFLOAT,0);
  addmess((method)multyq_fftinfo,"fftinfo",0);
  addfloat((method)multyq_float);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif 

#if PD
void multyq_tilde_setup(void)
{
  multyq_class = class_new(gensym("multyq~"), (t_newmethod)multyq_new, 
			 (t_method)multyq_free ,sizeof(t_multyq), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(multyq_class, t_multyq, x_f);
  class_addmethod(multyq_class,(t_method)multyq_dsp,gensym("dsp"),0);
  class_addmethod(multyq_class,(t_method)multyq_mute,gensym("mute"),A_FLOAT,0);
  class_addmethod(multyq_class,(t_method)multyq_bypass,gensym("bypass"),A_FLOAT,0);
  class_addmethod(multyq_class,(t_method)multyq_overlap,gensym("overlap"),A_FLOAT,0);
  class_addmethod(multyq_class,(t_method)multyq_winfac,gensym("winfac"),A_FLOAT,0);
  class_addmethod(multyq_class,(t_method)multyq_fftinfo,gensym("fftinfo"),0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif


void multyq_free(t_multyq *x)
{
#if MSP
  dsp_free((t_pxobject *)x);
#endif
  freebytes(x->Wanal,0);
  freebytes(x->Wsyn,0);
  freebytes(x->input,0);
  freebytes(x->Hwin,0);
  freebytes(x->buffer,0);
  freebytes(x->channel,0);
  freebytes(x->output,0);
  freebytes(x->bitshuffle,0);
  freebytes(x->trigland,0);
  freebytes(x->rcos,0);
  freebytes(x->freqs,0);
  freebytes(x->filt,0);
}

void multyq_overlap(t_multyq *x, t_floatarg f)
{
int i = (int) f;
  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
	x->overlap = i;
	multyq_init(x,1);
}

void multyq_winfac(t_multyq *x, t_floatarg f)
{
int i = (int)f;

  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = i;
	multyq_init(x,2);
}

void multyq_fftinfo(t_multyq *x)
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void *multyq_new(t_symbol *s, int argc, t_atom *argv)
{
int i;
#if MSP
  t_multyq *x = (t_multyq *)newobject(multyq_class);
  dsp_setup((t_pxobject *)x,13);
  outlet_new((t_pxobject *)x, "signal");
  x->x_obj.z_misc |= Z_NO_INPLACE;
#endif

#if PD
  t_multyq *x = (t_multyq *)pd_new(multyq_class);
  for(i=0; i<12; i++){
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal"));
  }
  outlet_new(&x->x_obj, gensym("signal"));
#endif
  
  x->overlap = atom_getfloatarg(0,argc,argv);
  x->winfac = atom_getfloatarg(1,argc,argv);
  if(!power_of_two(x->overlap))
    x->overlap = 4;
  if(!power_of_two(x->winfac))
    x->winfac = 2;

  x->D = sys_getblksize();
  x->R = sys_getsr();
  if(!x->R)
    x->R = 44100;
  if(!x->D)
    x->D = 256;
	  	    		

  multyq_init(x,0);

  return (x);
}

void multyq_init(t_multyq *x, short initialized)
{
	int i;
	float funda, base;
	
	x->N = x->D * x->overlap; 
	x->Nw = x->N * x->winfac;	
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->mult = 1. / (float) x->N; 
	x->inCount = -(x->Nw);
	
	if(!initialized){
		x->please_update = 0;
		x->always_update = 0;
		x->rcoslen = 8192 ;		
		x->Wanal = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->Wsyn = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->Hwin = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->input = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->output = (float *) getbytes( MAX_Nw * sizeof(float) );
		x->buffer = (float *) getbytes( MAX_N * sizeof(float) );
		x->channel = (float *) getbytes( (MAX_N+2) * sizeof(float) );
		x->bitshuffle = (int *) getbytes( MAX_N * 2 * sizeof( int ) );
		x->trigland = (float *) getbytes( MAX_N * 2 * sizeof( float ) );
		x->rcos = (float *) getbytes( x->rcoslen * sizeof( float ) );
		x->freqs = (float *) getbytes( MAX_N2 * sizeof( float ) );	
		x->filt = (float *) getbytes( (MAX_N2 + 1) * sizeof( float ) );	
		
		x->cf1  = 200.;
		x->gainfac1  = 0.0;
		x->bw1 = .15;
		
		x->cf2  = 700.;
		x->gainfac2  = 0.0;
		x->bw2  = .1; 
		
		x->cf3  = 3000.;
		x->gainfac3  = 0.0;
		x->bw3  = .15;
		
		x->cf4  = 12000.;
		x->gainfac4 = 0.0;
		x->bw4 = .15;
		
		x->mute = 0;
		x->bypass = 0;
		for (i = 0; i < x->rcoslen; i++){
			x->rcos[i] =  .5 - .5 * cos(((float)i/(float)x->rcoslen) * TWOPI);
		}	
		
	} 
		memset((char *)x->input,0,x->Nw * sizeof(float));
		memset((char *)x->output,0,x->Nw * sizeof(float));		

    init_rdft( x->N, x->bitshuffle, x->trigland);
    makehanning(x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
	funda = base = (float)x->R /(float)x->N ;
	for(i = 0; i < x->N2; i++){
		x->freqs[i] = base;
		base += funda;
	}
	update_filter_function(x);
	
}

t_int *multyq_perform(t_int *w)
{

  int i, j;
  t_multyq *x = (t_multyq *) (w[1]);
  short please_update = x->please_update;
  float	*inbuf = (t_float *)(w[2]);
  float	*in2 = (t_float *)(w[3]);
  float *in3 = (t_float *)(w[4]);
  float	*in4 = (t_float *)(w[5]);
  float	*in5 = (t_float *)(w[6]);
  float	*in6 = (t_float *)(w[7]);
  float	*in7 = (t_float *)(w[8]);
  float	*in8 = (t_float *)(w[9]);
  float	*in9 = (t_float *)(w[10]);
  float	*in10 = (t_float *)(w[11]);
  float	*in11 = (t_float *)(w[12]);
  float	*in12 = (t_float *)(w[13]);
  float	*in13 = (t_float *)(w[14]);
  float	*outbuf = (t_float *)(w[15]);
  t_int	n = w[16];
  int  inCount = x->inCount;
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
  float *filt = x->filt;
  float mult = x->mult;
  int in = x->inCount ;  
  int on = in;
  
  if(x->mute) {
    while (n--){
      *outbuf++ = 0.;
    }
    return (w+17);
  }	
  if(x->bypass) {
    while (n--){
      *outbuf++ = *inbuf++;
    }
    return (w+17);
  }
#if MSP
  if( x->in2_connected ){
    x->cf1 = *in2++ ;
  }
  if( x->in3_connected ){
    x->bw1 = *in3++ ;
  }
  if( x->in4_connected ){
    x->gainfac1 = *in4++ ;
  }
  if( x->in5_connected ){
    x->cf2 = *in5++ ;
  }
  if( x->in6_connected ){
    x->bw2 = *in6++ ;
  }
  if( x->in7_connected ){
    x->gainfac2 = *in7++ ;
  }  
  if( x->in8_connected ){
    x->cf3 = *in8++ ;
  }
  if( x->in9_connected ){
    x->bw3 = *in9++ ;
  }
  if( x->in10_connected ){
    x->gainfac3 = *in10++ ;
  }  
  if( x->in11_connected ){
    x->cf4 = *in11++ ;
  }
  if( x->in12_connected ){
    x->bw4 = *in12++ ;
  }
  if( x->in13_connected ){
    x->gainfac4 = *in13++;
  }  
#endif

#if PD
    x->cf1 = *in2++;
    x->bw1 = *in3++;
    x->gainfac1 = *in4++;
    x->cf2 = *in5++;
    x->bw2 = *in6++;
    x->gainfac2 = *in7++;
    x->cf3 = *in8++;
    x->bw3 = *in9++;
    x->gainfac3 = *in10++;
    x->cf4 = *in11++;
    x->bw4 = *in12++;
    x->gainfac4 = *in13++;
#endif
  
  if(x->always_update) {
    update_filter_function(x);
  }
  else if(please_update) {
    update_filter_function(x);	
    please_update = 0;
  }

  in += D;
  on += I;

  for ( j = 0 ; j < (Nw - D) ; j++ ){
    input[j] = input[j+D];
  }
  for ( j = (Nw-D), i = 0 ; j < Nw; j++, i++ ) {
    input[j] = *inbuf++;
  }

  fold(input, Wanal, Nw, buffer, N, in);   
  rdft(N, 1, buffer, bitshuffle, trigland);

  filtyQ(buffer, channel,filt, N2);

  rdft(N, -1, buffer, bitshuffle, trigland);
  overlapadd( buffer, N, Wsyn, output, Nw, inCount);
    
   
  for (j = 0; j < D; j++){
    *outbuf++ = output[j] * mult;
  }
  for (j = 0; j < Nw - D; j++){
    output[j] = output[j+D];
  }

  for (j = Nw - D; j < Nw; j++){
    output[j] = 0.;
  }	

  x->inCount = in;
  x->please_update = please_update;
  
  return (w+17);
}		

void multyq_bypass(t_multyq *x, t_floatarg state)
{
  x->bypass = (short)state;	
}

void multyq_mute(t_multyq *x, t_floatarg state)
{
  x->mute = (short)state;	
}

void multyq_dsp(t_multyq *x, t_signal **sp, short *count)
{
  int i;

  if(x->R != sp[0]->s_sr||x->D != sp[0]->s_n){
    x->R = sp[0]->s_sr;
    x->D = sp[0]->s_n;
    multyq_init(x,1);
  }

#if MSP
  x->in2_connected = count[1];
  x->in3_connected = count[2];
  x->in4_connected = count[3];
  x->in5_connected = count[4];
  x->in6_connected = count[5];
  x->in7_connected = count[6];
  x->in8_connected = count[7];
  x->in9_connected = count[8];
  x->in10_connected = count[9];
  x->in11_connected = count[10];
  x->in12_connected = count[11];
  x->in13_connected = count[12];
  x->always_update = 0;
  for(i = 1; i < 13; i++) {
    x->always_update += count[i];
  }
#endif

#if PD
    x->always_update = 1;
#endif		

  dsp_add(multyq_perform, 16, x, 
	  sp[0]->s_vec,sp[1]->s_vec,sp[2]->s_vec,sp[3]->s_vec,sp[4]->s_vec,  
	  sp[5]->s_vec,sp[6]->s_vec,sp[7]->s_vec,sp[8]->s_vec,sp[9]->s_vec, 
	  sp[10]->s_vec,sp[11]->s_vec,sp[12]->s_vec,sp[13]->s_vec,sp[0]->s_n);
}


void update_filter_function(t_multyq *x) 
{
  float funda, curfreq, m1, m2;
  float lo, hi ;
  float ploc, gainer;
  int i;
  float nyquist = (float)x->R / 2.0;
  float *filt = x->filt;
  float *rcos = x->rcos;
  float *freqs = x->freqs;
  int rcoslen = x->rcoslen;
  
  // sanity
  if( x->cf1 < 0 ){
    x->cf1 = 0;
  } 
  else if( x->cf1 > nyquist){
    x->cf1 = nyquist ;
  } 
  if( x->bw1 <= .05 ){
    x->bw1 = .05;
  }
  else if( x->bw1 > 1. ){
    x->bw1 = 1.;
  }
  if( x->gainfac1 < -1. ){
    x->gainfac1 = -1;
  }
  if( x->cf2 < 0 ){
    x->cf2 = 0;
  } 
  else if( x->cf2> nyquist){
    x->cf2 = nyquist ;
  } 
  if( x->bw2 <= .05 ){
    x->bw2 = .05;
  }
  else if( x->bw2 > 1. ){
    x->bw2 = 1.;
  }
  if( x->gainfac2 < -1. ){
    x->gainfac2 = -1;
  }
  if( x->cf3 < 0 ){
    x->cf3 = 0;
  } 
  else if( x->cf3 > nyquist){
    x->cf3 = nyquist ;
  } 
  if( x->bw3 <= .05 ){
    x->bw3 = .05;
  }
  else if( x->bw3 > 1. ){
    x->bw3 = 1.;
  }
  if( x->gainfac3 < -1. ){
    x->gainfac3 = -1;
  }
  if( x->cf4 < 0 ){
    x->cf4 = 0;
  } 
  else if( x->cf4 > nyquist){
    x->cf4 = nyquist ;
  } 
  if( x->bw4 <= .05 ){
    x->bw4 = .05;
  }
  else if( x->bw4 > 1. ){
    x->bw4 = 1.;
  }
  if( x->gainfac4 < -1. ){
    x->gainfac4 = -1;
  }
  for( i = 0; i < x->N2; i++ ) {
    x->filt[i] = 1.0 ;
  }
  // filt 1
  lo = x->cf1 * (1.0 - x->bw1 );
  hi = x->cf1 * (1.0 + x->bw1 );
  for( i = 0; i < x->N2; i++ ) {
    if(freqs[i] >= lo && freqs[i] <= hi){
      ploc = (freqs[i] - lo) / (hi - lo);
      gainer = 1 + x->gainfac1 * rcos[ (int) (ploc * rcoslen) ] ;
      if( gainer < 0 ){
	gainer = 0;
      }
      filt[i] *= gainer ;
		
    }
  }
  // filt 2
  lo = x->cf2 * (1.0 - x->bw2 );
  hi = x->cf2 * (1.0 + x->bw2 );
  for( i = 0; i < x->N2; i++ ) {
    if( freqs[i] >= lo && freqs[i] <= hi){
      ploc = (freqs[i] - lo) / (hi - lo);
      gainer = 1 + x->gainfac2 * rcos[ (int) (ploc * rcoslen) ] ;
      if( gainer < 0 ){
	gainer = 0;
      }
      filt[i] *= gainer ;
		
    }
  }
  // filt 3
  lo = x->cf3 * (1.0 - x->bw3 );
  hi = x->cf3 * (1.0 + x->bw3 );
  for( i = 0; i < x->N2; i++ ) {
    if(freqs[i] >= lo && freqs[i] <= hi){
      ploc = (freqs[i] - lo) / (hi - lo);
      gainer = 1 + x->gainfac3 * rcos[ (int) (ploc * rcoslen) ] ;
      if( gainer < 0 ){
	gainer = 0;
      }
      filt[i] *= gainer ;
		
    }
  }
  // filt 4
  lo = x->cf4 * (1.0 - x->bw4 );
  hi = x->cf4 * (1.0 + x->bw4 );
  for( i = 0; i < x->N2; i++ ) {
    if(freqs[i] >= lo && freqs[i] <= hi){
      ploc = (freqs[i] - lo) / (hi - lo);
      gainer = 1 + x->gainfac4 * rcos[ (int) (ploc * rcoslen) ] ;
      if( gainer < 0 ){
	gainer = 0;
      }
      filt[i] *= gainer ;
		
    }
  }

}
#if MSP
void multyq_float(t_multyq *x, double f) // Look at floats at inlets
{
int inlet = x->x_obj.z_in;
	
  if (inlet == 1)
    {
      x->cf1 = f;
    }
  else if (inlet == 2)
    {
      x->bw1 = f;
    }
  else if (inlet == 3)
    {
      x->gainfac1 = f;
    }
  else if (inlet == 4)
    {
      x->cf2 = f;
    }
  else if (inlet == 5)
    {
      x->bw2 = f;
    }
  else if (inlet == 6)
    {
      x->gainfac2 = f;
    }
  else if (inlet == 7)
    {
      x->cf3 = f;
    }
  else if (inlet == 8)
    {
      x->bw3 = f;
    }
  else if (inlet == 9)
    {
      x->gainfac3 = f;
    }	
  else if (inlet == 10)
    {
      x->cf4 = f;
    }
  else if (inlet == 11)
    {
      x->bw4 = f;
    }
  else if (inlet == 12)
    {
      x->gainfac4 = f;
    }		
  x->please_update = 1;
}
#endif

void multyq_assist (t_multyq *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0: sprintf(dst,"(signal) Input"); break;
    case 1: sprintf(dst,"(signal/float) Cf1");break;
    case 2: sprintf(dst,"(signal/float) Bw1"); break;
    case 3: sprintf(dst,"(signal/float) Gain1"); break;
    case 4: sprintf(dst,"(signal/float) Cf2"); break;
    case 5: sprintf(dst,"(signal/float) Bw2"); break;
    case 6: sprintf(dst,"(signal/float) Gain2"); break;
    case 7: sprintf(dst,"(signal/float) Cf3"); break;
    case 8: sprintf(dst,"(signal/float) Bw3"); break;
    case 9: sprintf(dst,"(signal/float) Gain3"); break;
    case 10: sprintf(dst,"(signal/float) Cf4"); break;
    case 11: sprintf(dst,"(signal/float) Bw4"); break;
    case 12: sprintf(dst,"(signal/float) Gain4"); break;
    }
  } else if (msg==2) {
    sprintf(dst,"(signal) Output");
  }
}


void filtyQ( float *S, float *C, float *filtfunc, int N2 )
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
    C[amp] *= filtfunc[ i ];
    C[phase] = -atan2( b, a );
  }

  for ( i = 0; i <= N2; i++ ) {
    imag = phase = ( real = amp = i<<1 ) + 1;
    S[real] = *(C+amp) * cos( *(C+phase) );
    if ( i != N2 )
      S[imag] = -*(C+amp) * sin( *(C+phase) );
  }
}
