#include "MSPd.h"
#include "fftease.h"

#if MSP
void *pvcompand_class;
#endif 

#if PD
static t_class *pvcompand_class;
#endif

#define OBJECT_NAME "pvcompand~"

#define MAX_N (16384)
#define MAX_Nw (MAX_N * 4)

typedef struct _pvcompand
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
  //
  float *curthresh ;
  float *atten;
  float *thresh ;
  int count;
  float thresh_interval;
  float max_atten; 
  float  atten_interval ; 
  float tstep;
  float gstep;
  float last_max_atten;
  short norml;
  short mute;
  short bypass;
  short connected[2];
  int overlap;
  int winfac;
} t_pvcompand;

void *pvcompand_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *pvcompand_perform(t_int *w);
void pvcompand_dsp(t_pvcompand *x, t_signal **sp, short *count);
void pvcompand_assist(t_pvcompand *x, void *b, long m, long a, char *s);
void update_thresholds(t_pvcompand *x);
void pvcompand_normalize(t_pvcompand *x, t_floatarg val);
void pvcompand_float(t_pvcompand *x, double f);
void pvcompand_free(t_pvcompand *x);
float pvcompand_ampdb(float db);
void pvcompand_init(t_pvcompand *x,short initialized);
void pvcompand_fftinfo(t_pvcompand *x);
void pvcompand_overlap(t_pvcompand *x, t_floatarg f);
void pvcompand_winfac(t_pvcompand *x, t_floatarg f);
void pvcompand_bypass(t_pvcompand *x, t_floatarg f);
void pvcompand_mute(t_pvcompand *x, t_floatarg f);

#if MSP
void main(void)
{
  setup((t_messlist **)&pvcompand_class, (method)pvcompand_new, 
  (method)pvcompand_free, (short)sizeof(t_pvcompand), 0, A_GIMME, 0);
  addmess((method)pvcompand_dsp, "dsp", A_CANT, 0);
  addmess((method)pvcompand_normalize, "normalize", A_LONG, 0);
  addmess((method)pvcompand_winfac,"winfac", A_FLOAT, 0);
  addmess((method)pvcompand_overlap,"overlap", A_FLOAT, 0);
  addmess((method)pvcompand_fftinfo,"fftinfo", 0);
  addmess((method)pvcompand_bypass,"bypass", A_FLOAT, 0);
  addmess((method)pvcompand_mute,"mute", A_FLOAT, 0);
  addmess((method)pvcompand_assist,"assist",A_CANT,0);
  addfloat((method)pvcompand_float);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void pvcompand_tilde_setup(void)
{
  pvcompand_class = class_new(gensym("pvcompand~"), (t_newmethod)pvcompand_new, 
			 (t_method)pvcompand_free ,sizeof(t_pvcompand), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(pvcompand_class, t_pvcompand, x_f);
  class_addmethod(pvcompand_class,(t_method)pvcompand_dsp,gensym("dsp"),0);
  class_addmethod(pvcompand_class,(t_method)pvcompand_mute,gensym("mute"),A_FLOAT,0);
  class_addmethod(pvcompand_class,(t_method)pvcompand_bypass,gensym("bypass"),A_FLOAT,0);
  class_addmethod(pvcompand_class,(t_method)pvcompand_overlap,gensym("overlap"),A_FLOAT,0);
  class_addmethod(pvcompand_class,(t_method)pvcompand_winfac,gensym("winfac"),A_FLOAT,0);
  class_addmethod(pvcompand_class,(t_method)pvcompand_fftinfo,gensym("fftinfo"),0);
  class_addmethod(pvcompand_class,(t_method)pvcompand_normalize,gensym("normalize"),A_FLOAT,0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}

#endif

void pvcompand_bypass(t_pvcompand *x, t_floatarg f)
{
	x->bypass = (short)f;
}

void pvcompand_mute(t_pvcompand *x, t_floatarg f)
{
	x->mute = (short)f;
}


void pvcompand_free( t_pvcompand *x ){
#if MSP
	dsp_free( (t_pxobject *) x);
#endif
	freebytes(x->curthresh,0);
	freebytes(x->atten,0);
	freebytes(x->thresh,0);
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

void pvcompand_assist (t_pvcompand *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:
      sprintf(dst,"(signal) Input");
      break;
    case 1:
      sprintf(dst,"(float/signal) Threshold");
      break;

    }
  } else if (msg==2) {
    sprintf(dst,"(signal) Output");
  }
}

#if MSP
void pvcompand_float(t_pvcompand *x, double f) // Look at floats at inlets
{
int inlet = x->x_obj.z_in;
	
    if (inlet == 1)
	{
		x->last_max_atten = x->max_atten = f;
		update_thresholds(x);	
		
	}
}
#endif
  
void *pvcompand_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_pvcompand *x = (t_pvcompand *)newobject(pvcompand_class);
  dsp_setup((t_pxobject *)x,2);
  outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
    t_pvcompand *x = (t_pvcompand *)pd_new(pvcompand_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
#endif

  // INITIALIZATIONS
  x->D  = sys_getblksize();
  x->R = sys_getsr();
  x->max_atten = atom_getfloatarg(0,argc,argv);
  x->overlap = atom_getfloatarg(1,argc,argv);
  x->winfac = atom_getfloatarg(2,argc,argv);
  if(!x->max_atten)
  	x->max_atten = -6;

  if(x->D <= 0)
  	x->D = 256;
  if(x->R <= 0)
  	x->R = 44100;

   pvcompand_init(x,0);
  return (x);
}

void pvcompand_init(t_pvcompand *x,short initialized)
{
int i;

  if(!power_of_two(x->overlap))
  	x->overlap = 4;
  if(!power_of_two(x->winfac))
  	x->winfac = 1;
  	
  x->N = x->D * x->overlap;
  x->Nw = x->N * x->winfac;	
  limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
  x->N2 = (x->N)>>1;
  x->Nw2 = (x->Nw)>>1;
  x->inCount = -(x->Nw);
  x->mult = 1. / (float) x->N;

	if(!initialized){
	  x->norml = 0;
	  x->mute = 0;
	  x->bypass = 0;
	  x->thresh_interval = 1.0;
	  x->last_max_atten =  x->max_atten; 
	  x->atten_interval = 2.0 ; 
	  x->tstep = 1.0 ;
	  x->gstep = 2.0 ;	
    x->Wanal = (float *) getbytes(MAX_Nw * sizeof(float));	
    x->Wsyn = (float *) getbytes(MAX_Nw * sizeof(float));	
    x->Hwin = (float *) getbytes(MAX_Nw * sizeof(float));	
    x->input = (float *) getbytes(MAX_Nw * sizeof(float) );	
    x->buffer = (float *) getbytes(MAX_N * sizeof(float) );
    x->channel = (float *) getbytes( (MAX_N+2) * sizeof(float) );
    x->output = (float *) getbytes(MAX_N * sizeof(float) );
    x->bitshuffle = (int *) getbytes(MAX_N * 2 * sizeof( int ) );
    x->trigland = (float *) getbytes(MAX_N * 2 * sizeof( float ) );
    x->thresh = (float *) getbytes(MAX_N * sizeof(float) );
	  x->atten = (float *) getbytes(MAX_N * sizeof(float) );
	  x->curthresh = (float *) getbytes(MAX_N * sizeof(float) );
	} 
		
		memset((char *)x->input,0,x->Nw * sizeof(float));
		memset((char *)x->output,0,x->Nw * sizeof(float));

	  init_rdft( x->N, x->bitshuffle, x->trigland);
	  makewindows( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D);
	  update_thresholds(x); 

}

void update_thresholds( t_pvcompand *x ) {
  int i;

  float nowamp = x->max_atten ;
  float nowthresh = 0.0 ;

  x->count = 0;
  if( nowamp < 0.0 ) 
    while( nowamp < 0.0 ){
      x->atten[x->count] = pvcompand_ampdb( nowamp );
      nowamp += x->gstep ;
      ++(x->count);
 	if(x->count >= x->N){
		error("count exceeds %d",x->N);
		x->count = x->N - 1;
		break;
	}
    }
  else if( nowamp > 0.0 ) 
    while( nowamp > 0.0 ){
      x->atten[x->count] = pvcompand_ampdb( nowamp );
      nowamp -= x->gstep ;
      ++(x->count);
 	if(x->count >= x->N){
		error("count exceeds %d",x->N);
		x->count = x->N - 1;
		break;
	}
    }

  for( i = 0; i < x->count; i++){
    x->thresh[i] = pvcompand_ampdb( nowthresh );
    nowthresh -= x->tstep ;
  }
  /*   
       for( i = 0; i < count; i++)
       fprintf(stderr,"thresh %f gain %f\n",thresh[i], atten[i]);
  */
}

void pvcompand_normalize(t_pvcompand *x, t_floatarg val) 
{
  x->norml = val;
}

t_int *pvcompand_perform(t_int *w)
{
  float sample, outsamp ;	
  int i,j;	
  float maxamp ;	
  float fmult;
  float cutoff;
  float avr, new_avr, rescale;

  t_pvcompand *x = (t_pvcompand *) (w[1]);
  t_float *in = (t_float *)(w[2]);
  t_float *in2 = (t_float *)(w[3]);
  t_float *out = (t_float *)(w[4]);
  int n = (int)(w[5]);

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
  float mult = x->mult;
  int count = x->count;
  float *atten = x->atten;
  float *curthresh = x->curthresh;
  float *thresh = x->thresh;
  float max_atten = x->max_atten;		

if( x->mute ){
	while( n-- ){
		*out++ = 0.0;
	}
	return (w+6); 
}
if( x->bypass ){
	while( n-- ){
		*out++ = *in++ * 0.5; // gain compensation
	}
	return (w+6); 
}

#if MSP
  if( x->connected[1] ){
	  max_atten = *in2++ ;
	  if(max_atten != x->last_max_atten) {
	    x->last_max_atten = x->max_atten = max_atten;
	    update_thresholds(x);
	  }
  } 
#endif

#if PD
  max_atten = *in2++ ;
  if(max_atten != x->last_max_atten) {
    x->last_max_atten = x->max_atten = max_atten;
    update_thresholds(x);
  }
#endif

  inCount += D;

  for ( j = 0 ; j < Nw - D ; j++ ){
    input[j] = input[j+D];
  }
  for ( j = Nw - D; j < Nw; j++ ) {
    input[j] = *in++;
  }

  fold( input, Wanal, Nw, buffer, N, inCount );	
  rdft( N, 1, buffer, bitshuffle, trigland );

  leanconvert(buffer, channel, N2);

  maxamp = 0.;
  avr = 0;
  for( i = 0; i < N; i+= 2 ){
    avr += channel[i];
    if( maxamp < channel[i] ){
      maxamp = channel[i] ;
    }
  }
   if(count <= 1){
	//	post("count too low!"); 
		count = 1;
	}
  for( i = 0; i < count; i++ ){
    curthresh[i] = thresh[i]*maxamp ;
  }
  cutoff = curthresh[count-1];
  new_avr = 0;
  for( i = 0; i < N; i += 2){
    if( channel[i] > cutoff ){
      j = count-1;
      while( channel[i] > curthresh[j] ){
				j--;
				if( j < 0 ){
				  j = 0;
				  break;
				}
      }
      channel[i] *= atten[j];
    }
    new_avr += channel[i] ;
  }

  leanunconvert( channel,buffer, N2);

  rdft( N, -1, buffer, bitshuffle, trigland );

  overlapadd( buffer, N, Wsyn, output, Nw, inCount);
  if( x->norml ) {
    if( new_avr <= 0 ){
      new_avr = .0001;
    }
    rescale =  avr / new_avr ;
    mult *= rescale ;

  } else {
    mult *= pvcompand_ampdb( max_atten * -.5); ;
  }

  for ( j = 0; j < D; j++ ){
    *out++ = output[j] * mult;

  }
  for ( j = 0; j < Nw - D; j++ )
    output[j] = output[j+D];
			
  for ( j = Nw - D; j < Nw; j++ )
    output[j] = 0.;

	

  /* restore state variables */
  x->inCount = inCount % Nw;
  return (w+6);
}	
	
float pvcompand_ampdb(float db) 
{
  float amp;
  amp = pow((double)10.0, (double)(db/20.0)) ;
  return(amp);
}

void pvcompand_overlap(t_pvcompand *x, t_floatarg f)
{
int i = (int) f;
  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
	x->overlap = i;
	pvcompand_init(x,1);
}

void pvcompand_winfac(t_pvcompand *x, t_floatarg f)
{
int i = (int)f;

  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = i;
	pvcompand_init(x,2);
}

void pvcompand_fftinfo(t_pvcompand *x)
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void pvcompand_dsp(t_pvcompand *x, t_signal **sp, short *count)
{
  long i;

#if MSP
  x->connected[1] = count[1];
#endif

  if(x->D != sp[0]->s_n || x->R != sp[0]->s_sr ){
  	x->D = sp[0]->s_n;
  	x->R = sp[0]->s_sr;
    pvcompand_init(x,1);
  }
  
  dsp_add(pvcompand_perform, 5,x,sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

