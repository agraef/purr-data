#include "MSPd.h"

#include "fftease.h"

#if MSP
void *crossx_class;
#endif 

#if PD
static t_class *crossx_class;
#endif

#define OBJECT_NAME "crossx~"

typedef struct _crossx
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
  float *last_channel;
  //
  int	inCount;
  float *Hwin;
  float *Wanal;	
  float *Wsyn;	
  float *output;
  /* crossx vars */

  float *c_lastphase_in1;
  float *c_lastphase_in2;
  float *c_lastphase_out;
  float c_fundamental;
  float c_factor_in;
  float c_factor_out;

  float threshie;
  short thresh_connected;
  // for fast fft
  float mult; 
  float *trigland;
  int *bitshuffle;
  int overlap;//overlap factor
  int winfac;//window factor
  int vs;//vector size
  short mute;//flag
  short autonorm;// for self gain regulation
} t_crossx;

void *crossx_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *crossx_perform(t_int *w);
void crossx_dsp(t_crossx *x, t_signal **sp, short *count);
void crossx_assist(t_crossx *x, void *b, long m, long a, char *s);
void crossx_float(t_crossx *x, double f);
void *crossx_new(t_symbol *s, int argc, t_atom *argv);
void crossx_init(t_crossx *x, short initialized);
void crossx_overlap(t_crossx *x, t_floatarg o);
void crossx_winfac(t_crossx *x, t_floatarg o);
void crossx_fftinfo(t_crossx *x);
void crossx_mute(t_crossx *x, t_floatarg toggle);
void crossx_autonorm(t_crossx *x, t_floatarg toggle);
void crossx_free(t_crossx *x);

#if MSP
void main(void)
{
  setup((t_messlist **)&crossx_class, (method)crossx_new, (method)dsp_free, 
  (short)sizeof(t_crossx), 0L, A_GIMME, 0);
  addmess((method)crossx_dsp, "dsp", A_CANT, 0);
  addmess((method)crossx_assist,"assist",A_CANT,0);
  addmess((method)crossx_mute,"mute",A_DEFFLOAT,0);
  addmess((method)crossx_overlap, "overlap",  A_DEFFLOAT, 0);
  addmess((method)crossx_winfac, "winfac",  A_DEFFLOAT, 0);
  addmess((method)crossx_fftinfo, "fftinfo", 0);
  addmess((method)crossx_autonorm, "autonorm",  A_DEFFLOAT, 0);
  addfloat((method)crossx_float);
  dsp_initclass();
   	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif
#if PD
void crossx_tilde_setup(void)
{
  crossx_class = class_new(gensym("crossx~"), (t_newmethod)crossx_new, 
			 (t_method)crossx_free ,sizeof(t_crossx), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(crossx_class, t_crossx, x_f);
  class_addmethod(crossx_class, (t_method)crossx_dsp, gensym("dsp"), 0);
  class_addmethod(crossx_class, (t_method)crossx_assist, gensym("assist"), 0);
  class_addmethod(crossx_class, (t_method)crossx_mute, gensym("mute"), A_DEFFLOAT,0);
  class_addmethod(crossx_class, (t_method)crossx_overlap, gensym("overlap"), A_DEFFLOAT,0);
  class_addmethod(crossx_class, (t_method)crossx_winfac, gensym("winfac"), A_DEFFLOAT,0);
  class_addmethod(crossx_class, (t_method)crossx_fftinfo, gensym("fftinfo"), 0);
  class_addmethod(crossx_class, (t_method)crossx_autonorm, gensym("autonorm"), A_DEFFLOAT,0);
   	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
post("padded memory");
}
#endif


void crossx_autonorm(t_crossx *x, t_floatarg toggle)
{
	x->autonorm = (short) toggle;
}
void crossx_assist (t_crossx *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:
      sprintf(dst,"(signal) Driver Sound");
      break;
    case 1:
      sprintf(dst,"(signal) Filter Sound");
      break;
    case 2:
      sprintf(dst,"(float/signal) Cross Synthesis Threshold");
      break;

    }
  } else if (msg==2) {
    sprintf(dst,"(signal) Output");
  }
}

void crossx_overlap(t_crossx *x, t_floatarg o)
{
  if(!power_of_two((int)o)){
    error("%f is not a power of two",o);
    return;
  }
  x->overlap = (int)o;
  crossx_init(x,1);
}

void crossx_winfac(t_crossx *x, t_floatarg f)
{
  if(!power_of_two((int)f)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = (int)f;
  crossx_init(x,1);
}

void crossx_fftinfo( t_crossx *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void crossx_mute(t_crossx *x, t_floatarg toggle)
{
  x->mute = (short)toggle;
}

void crossx_free(t_crossx *x)
{
#if MSP
  dsp_free((t_pxobject *) x);
#endif
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
  /* these last are extra - kill if we clean up - upstairs
  free(x->c_lastphase_in1);
  free(x->c_lastphase_in2);
  free(x->c_lastphase_out);
  free(x->last_channel);*/
}

#if MSP
void crossx_float(t_crossx *x, double f) // Look at floats at inlets
{
int inlet = x->x_obj.z_in;
	
  if (inlet == 2)
    {
      x->threshie = f;
    }
}
#endif

void *crossx_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_crossx *x = (t_crossx *)newobject(crossx_class);
  dsp_setup((t_pxobject *)x,3);
  outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
  t_crossx *x = (t_crossx *)pd_new(crossx_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif


  x->overlap = atom_getfloatarg(0,argc,argv);
  x->winfac = atom_getfloatarg(1,argc,argv);
  if(x->overlap <= 0)
    x->overlap = 4;

  x->winfac = 1;
    
  x->R = sys_getsr();
  x->vs = sys_getblksize();

  crossx_init(x,0);
  return (x);
}

void crossx_init(t_crossx *x, short initialized)
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

  x->c_fundamental =  (float) x->R/( (x->N2)<<1 );
  x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
  x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
	if(!initialized){
      x->threshie = .001 ;
      x->autonorm = 0;
        x->mute = 0;
	  x->Wanal = (float *) getbytes((MAX_Nw) * sizeof(float));	
	  x->Wsyn = (float *) getbytes((MAX_Nw) * sizeof(float));	
	  x->Hwin = (float *) getbytes((MAX_Nw) * sizeof(float));
	  x->output = (float *) getbytes((MAX_Nw) * sizeof(float));
	  x->bitshuffle = (int *) getbytes((MAX_N * 2)* sizeof(int));
	  x->trigland = (float *) getbytes((MAX_N * 2)* sizeof(float));

	  x->input1 = (float *) getbytes(MAX_Nw * sizeof(float));	
	  x->buffer1 = (float *) getbytes(MAX_N * sizeof(float));
	  x->channel1 = (float *) getbytes((MAX_N+2) * sizeof(float));
	  x->input2 = (float *) getbytes(MAX_Nw * sizeof(float));	
	  x->buffer2 = (float *) getbytes(MAX_N * sizeof(float));
	  x->channel2 = (float *) getbytes((MAX_N+2) * sizeof(float));
	  x->last_channel = (float *) getbytes((MAX_N+2) * sizeof(float));
	  x->c_lastphase_in1 = (float *) getbytes((MAX_N2+1) * sizeof(float));
	  x->c_lastphase_in2 = (float *) getbytes((MAX_N2+1) * sizeof(float));
	  x->c_lastphase_out = (float *) getbytes((MAX_N2+1) * sizeof(float));
	} 
		memset((char *)x->input1,0,x->Nw * sizeof(float));
		memset((char *)x->input2,0,x->Nw * sizeof(float));
		memset((char *)x->output,0,x->Nw * sizeof(float));
		memset((char *)x->buffer1,0,x->N * sizeof(float));
		memset((char *)x->buffer2,0,x->N * sizeof(float));
		memset((char *)x->channel1,0,(x->N+2) * sizeof(float));
		memset((char *)x->channel2,0,(x->N+2) * sizeof(float));
		memset((char *)x->c_lastphase_in1,0,(x->N2+1) * sizeof(float));
		memset((char *)x->c_lastphase_in2,0,(x->N2+1) * sizeof(float));
		memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));
	
  init_rdft( x->N, x->bitshuffle, x->trigland);
  makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
}

t_int *crossx_perform(t_int *w)
{
	int i, j;

  float a1, a2, b1, b2;
  int even, odd;
  int amp, freq;
  float gainer, threshie;
  float ingain = 0;
  float outgain, rescale;
  float mymult;

  t_crossx *x = (t_crossx *) (w[1]);
  t_float *in1 = (t_float *)(w[2]);
  t_float *in2 = (t_float *)(w[3]);
  t_float *in3 = (t_float *)(w[4]);
  t_float *out = (t_float *)(w[5]);
  t_int n = w[6];
	
  /* dereference struncture  */	
  float *input1 = x->input1;
  float *input2 = x->input2;
  float *buffer1 = x->buffer1;
  float *buffer2 = x->buffer2;
  int inCount = x->inCount;
  int R = x->R;
  int N = x->N;
  int N2 = x->N2;
  int D = x->D;
  int Nw = x->Nw;
  float *Wanal = x->Wanal;
  float *Wsyn = x->Wsyn;
  float *output = x->output;
  float *channel1 = x->channel1;
  float *channel2 = x->channel2;
  float *last_channel = x->last_channel;
  int *bitshuffle = x->bitshuffle;
  float *trigland = x->trigland;
  float mult = x->mult;
  float *c_lastphase_in1 = x->c_lastphase_in1;
  float *c_lastphase_in2 = x->c_lastphase_in2;
float *c_lastphase_out = x->c_lastphase_out;
float c_fundamental = x->c_fundamental;
	float c_factor_in = x->c_factor_in;
	float c_factor_out = x->c_factor_out;
	short autonorm = x->autonorm;

  if(x->mute){
    while(n--){
      *out++ = 0;
    }
    return w+7;
  }
  
  if( x->thresh_connected ){	
  	threshie = *in3++;
  } else {
  	threshie = x->threshie;
  }
  
  inCount += D;

  for ( j = 0 ; j < Nw - D ; j++ ){
    input1[j] = input1[j+D];
    input2[j] = input2[j+D];
  }
  for ( j = Nw - D; j < Nw; j++ ) {
    input1[j] = *in1++;
    input2[j] = *in2++;
  }

  fold( input1, Wanal, Nw, buffer1, N, inCount );		
  fold( input2, Wanal, Nw, buffer2, N, inCount );	
  rdft( N, 1, buffer1, bitshuffle, trigland );
  rdft( N, 1, buffer2, bitshuffle, trigland );

/* changing algorithm for window flexibility */
  if(autonorm){
    ingain = 0;
  	for(i = 0; i < N; i+=2){
  		ingain += hypot(buffer1[i], buffer1[i+1]);
  	}
  }

  for ( i = 0; i <= N2; i++ ) {
    odd = ( even = i<<1 ) + 1;

    a1 = ( i == N2 ? *(buffer1+1) : *(buffer1+even) );
    b1 = ( i == 0 || i == N2 ? 0. : *(buffer1+odd) );
    a2 = ( i == N2 ? *(buffer2+1) : *(buffer2+even) );
    b2 = ( i == 0 || i == N2 ? 0. : *(buffer2+odd) );
    gainer = hypot(a2, b2);
    if( gainer > threshie ) 
    	*(channel1+even) = hypot( a1, b1 ) * gainer;
    *(channel1+odd) = -atan2( b1, a1 );
    *(buffer1+even) = *(channel1+even) * cos( *(channel1+odd) );
    if ( i != N2 )
      *(buffer1+odd) = -(*(channel1+even)) * sin( *(channel1+odd) );

  }
  if(autonorm){
    outgain = 0;
  	for(i = 0; i < N; i+=2){
  		outgain += hypot(buffer1[i], buffer1[i+1]);
  	}
	if(ingain <= .0000001){
  		// post("gain emergency!");
  		rescale = 1.0;
  	} else {
  		rescale = ingain / outgain;
  	} 
  	// post("ingain %f outgain %f rescale %f",ingain, outgain, rescale);
  	mymult = mult * rescale;
  }  else {
  	mymult = mult;
  }

  rdft( N, -1, buffer1, bitshuffle, trigland );
  overlapadd( buffer1, N, Wsyn, output, Nw, inCount);

  for ( j = 0; j < D; j++ )
    *out++ = output[j] * mymult;

  for ( j = 0; j < Nw - D; j++ )
    output[j] = output[j+D];
			
  for ( j = Nw - D; j < Nw; j++ )
    output[j] = 0.;

	

  /* restore state variables */
  x->inCount = inCount % Nw;
  return (w+7);
}		

void crossx_dsp(t_crossx *x, t_signal **sp, short *count)
{
#if MSP
  x->thresh_connected = count[2];
#endif
#if PD
  x->thresh_connected = 1;
#endif

  if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr ){
  	x->vs = sp[0]->s_n;
  	x->R = sp[0]->s_sr;
    crossx_init(x,1);
  }
  
  dsp_add(crossx_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
	  sp[3]->s_vec, sp[0]->s_n);
}

