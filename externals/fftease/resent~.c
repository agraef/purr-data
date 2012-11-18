#include "MSPd.h"
#include "fftease.h"

#if MSP
void *resent_class;
#endif 

#if PD
static t_class *resent_class;
#endif

#define OBJECT_NAME "resent~"

typedef struct _resent
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
  float *frame_incr;
  float *store_incr;
  float *frame_phase;
  float frameloc;
  float **loveboat;
  float current_frame;
  int framecount;
  //
  float frame_increment ;
  float *composite_frame ;
  float fpos;
  float last_fpos;
  float tadv;
  // for convert
  float *c_lastphase_in;
  float *c_lastphase_out;
  float c_fundamental;
  float c_factor_in;
  float c_factor_out;
  // faster fft
  float mult; 
  float *trigland;
  int *bitshuffle;
  int read_me;
  int frames_read;
//  int MAXFRAMES;
  short mute;
  void *m_clock;
  void *m_bang;
  short playthrough;
  int hopsize;
  int overlap;
  int winfac;
  short lock;
  float duration;
  short verbose;
  float sync;
} t_resent;

void *resent_new(t_symbol *msg, short argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *resent_perform(t_int *w);
void resent_dsp(t_resent *x, t_signal **sp, short *count);
void resent_assist(t_resent *x, void *b, long m, long a, char *s);
void resent_acquire_sample (t_resent *x) ;
void resent_mute(t_resent *x, t_floatarg tog);
void resent_bin(t_resent *x, t_floatarg fbin, t_floatarg speed);
void resent_setphase(t_resent *x, t_floatarg phase);
void resent_addphase(t_resent *x, t_floatarg phase);
void resent_setspeed( t_resent *x,  t_floatarg speed );
void resent_addspeed( t_resent *x,  t_floatarg speed );
void resent_size( t_resent *x,  t_floatarg size_ms );
void resent_free( t_resent *x );
void resent_store_incr( t_resent *x );
void resent_setspeed_and_phase( t_resent *x,  t_floatarg speed, t_floatarg phase );
void resent_tick(t_resent *x);
void resent_fftinfo(t_resent *x);
void resent_init(t_resent *x, short flag);
void resent_linephase(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_linespeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_randphase(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_randspeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_playthrough(t_resent *x, t_floatarg state);
float resent_randf(float min, float max);
void resent_winfac(t_resent *x, t_floatarg factor);

void resent_verbose(t_resent *x, t_floatarg t);
void resent_fftinfo(t_resent *x);
void resent_overlap(t_resent *x, t_floatarg f);
void resent_winfac(t_resent *x, t_floatarg f);

#if MSP
void main(void)
{
  setup((t_messlist **)&resent_class, (method)resent_new, 
  (method)resent_free, (short)sizeof(t_resent), 0L,A_GIMME,0);
  addmess((method)resent_dsp, "dsp", A_CANT, 0);
  addmess((method)resent_assist,"assist",A_CANT,0);
  addbang((method)resent_acquire_sample);
  addmess((method)resent_mute, "mute", A_FLOAT, 0);
  addmess((method)resent_linespeed, "linespeed", A_GIMME, 0);
  addmess((method)resent_linephase, "linephase", A_GIMME, 0);
  addmess((method)resent_randspeed, "randspeed", A_GIMME, 0);
  addmess((method)resent_randphase, "randphase", A_GIMME, 0);
  addmess((method)resent_bin, "bin", A_DEFFLOAT, A_DEFFLOAT, 0);
  addmess((method)resent_setphase, "setphase",  A_DEFFLOAT, 0);
  addmess((method)resent_addphase, "addphase",  A_DEFFLOAT, 0);
  addmess((method)resent_setspeed, "setspeed",  A_DEFFLOAT, 0);
  addmess((method)resent_addspeed, "addspeed",  A_DEFFLOAT, 0);
  addmess((method)resent_playthrough, "playthrough",  A_DEFFLOAT, 0);
  addmess((method)resent_store_incr, "store_incr",0);
  addmess((method)resent_fftinfo, "fftinfo",0);
  addmess((method)resent_overlap, "overlap",A_FLOAT,0);
  addmess((method)resent_winfac, "winfac",A_FLOAT,0);
  addmess((method)resent_setspeed_and_phase, "setspeed_and_phase",  A_DEFFLOAT, A_DEFFLOAT, 0);
  addmess((method)resent_size, "size", A_FLOAT,0);
//  addmess((method)resent_verbose, "verbose", A_FLOAT,0);
  addmess((method)resent_overlap,"overlap",A_DEFFLOAT,0);
  addmess((method)resent_winfac,"winfac",A_DEFFLOAT,0);
  addmess((method)resent_fftinfo,"fftinfo",0);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void resent_tilde_setup(void)
{
  resent_class = class_new(gensym("resent~"), (t_newmethod)resent_new, 
			 (t_method)resent_free ,sizeof(t_resent), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(resent_class, t_resent, x_f);
  class_addmethod(resent_class,(t_method)resent_dsp,gensym("dsp"),0);
  class_addmethod(resent_class,(t_method)resent_mute,gensym("mute"),A_FLOAT,0);
  class_addmethod(resent_class,(t_method)resent_linespeed,gensym("linespeed"),A_GIMME,0);
  class_addmethod(resent_class,(t_method)resent_linephase,gensym("linephase"),A_GIMME,0);
  class_addmethod(resent_class,(t_method)resent_randspeed,gensym("randspeed"),A_GIMME,0);
  class_addmethod(resent_class,(t_method)resent_randphase,gensym("randphase"),A_GIMME,0);
  class_addmethod(resent_class,(t_method)resent_bin,gensym("bin"),A_DEFFLOAT, A_DEFFLOAT,0);
// Pd may still have parser bug for similar starting strings
  class_addmethod(resent_class,(t_method)resent_setspeed_and_phase,gensym("ssap"),A_DEFFLOAT, A_DEFFLOAT,0);
  class_addmethod(resent_class,(t_method)resent_setphase,gensym("setphase"),A_DEFFLOAT,0);
  class_addmethod(resent_class,(t_method)resent_addphase,gensym("addphase"),A_DEFFLOAT,0);
  class_addmethod(resent_class,(t_method)resent_setspeed,gensym("setspeed"),A_DEFFLOAT,0);
  class_addmethod(resent_class,(t_method)resent_addspeed,gensym("addspeed"),A_DEFFLOAT,0);
  class_addmethod(resent_class,(t_method)resent_fftinfo,gensym("fftinfo"),0);
class_addmethod(resent_class,(t_method)resent_store_incr,gensym("store_incr"),0);
  class_addmethod(resent_class,(t_method)resent_playthrough,gensym("playthrough"),A_FLOAT,0); 
  class_addmethod(resent_class,(t_method)resent_size,gensym("size"),A_FLOAT,0);
  class_addmethod(resent_class,(t_method)resent_overlap,gensym("overlap"),A_FLOAT,0); 
  class_addmethod(resent_class,(t_method)resent_winfac,gensym("winfac"),A_FLOAT,0);  
  class_addmethod(resent_class,(t_method)resent_verbose,gensym("verbose"),A_FLOAT,0);
  class_addmethod(resent_class,(t_method)resent_acquire_sample,gensym("acquire_sample"),0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif



void resent_verbose(t_resent *x, t_floatarg t)
{
  x->verbose = (short)t;
}


void resent_overlap(t_resent *x, t_floatarg f)
{
int i = (int) f;
  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
	x->overlap = i;
	resent_init(x,1);
}

void resent_winfac(t_resent *x, t_floatarg f)
{
int i = (int)f;

  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = i;
	resent_init(x,2);
}

void resent_fftinfo(t_resent *x)
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void resent_size(t_resent *x, t_floatarg size)
{
  x->duration = size/1000.0;
  resent_init(x,1);
}

void resent_store_incr(t_resent *x)
{
  int i;
  float *store_incr = x->store_incr;
  float *frame_incr = x->frame_incr;
	
  for(i = 0; i < x->N2; i++){
    store_incr[i] = frame_incr[i];
  }
}
	
void resent_free(t_resent *x){
  int i ;
#if MSP
  dsp_free((t_pxobject *) x);
#endif
  for(i = 0; i < x->framecount; i++){
    freebytes(x->loveboat[i],0) ;
  }	
  freebytes(x->frame_phase,0);
  freebytes(x->composite_frame,0);
  freebytes(x->c_lastphase_in,0);
  freebytes(x->c_lastphase_out,0);
  freebytes(x->trigland,0);
  freebytes(x->bitshuffle,0);
  freebytes(x->Wanal,0);
  freebytes(x->Wsyn,0);
  freebytes(x->input,0);
  freebytes(x->Hwin,0);
  freebytes(x->buffer,0);
  freebytes(x->channel,0);
  freebytes(x->output,0);
  freebytes(x->frame_incr,0);
  freebytes(x->store_incr,0);
}

void resent_bin(t_resent *x, t_floatarg fbin, t_floatarg speed)
{
int bin_num = (int) fbin;

  if(bin_num >= 0 && bin_num < x->N2){
    x->frame_incr[bin_num] = speed ;
  } else {
    post("resent~: bin %d is out of range", bin_num);
  }
}

void resent_setphase( t_resent *x,  t_floatarg phase)
{
  float scaled_phase ;
  int i;
	
  if( phase < 0. )
    phase = 0. ;
  if( phase > 1. )
    phase = 1.;
  scaled_phase = phase * (float) x->framecount ;
  for( i = 0; i < x->N2; i++ ){
    x->frame_phase[i] = scaled_phase ;
  }
		
}

void resent_addphase( t_resent *x,  t_floatarg phase )
{
  float scaled_phase ;
  float *frame_phase = x->frame_phase;
  int framecount =  x->framecount;
  int i;
	
		
  if( phase < 0. )
    phase = 0. ;
  if( phase > 1. )
    phase = 1.;
  scaled_phase = phase * (float) framecount ;
  for( i = 0; i < x->N2; i++ ){
    frame_phase[i] += scaled_phase ;
    while( frame_phase[i] < 0 )
      frame_phase[i] += framecount;
    while( frame_phase[i] > framecount - 1 )
      frame_phase[i] -= framecount ;
  }

		
}

void resent_setspeed( t_resent *x,  t_floatarg speed )
{
  int i;
	
  for( i = 0; i < x->N2; i++ ){
	
    x->frame_incr[i] = speed ;
  }
  // post("speed reset to %f",speed);
		
}

void resent_addspeed( t_resent *x,  t_floatarg speed )
{
  int i;
  float *store_incr = x->store_incr;
  float *frame_incr = x->frame_incr;
		
  for( i = 0; i < x->N2; i++ ){	
    frame_incr[i] = store_incr[i] + speed ;
  }
	
		
}
void resent_setspeed_and_phase( t_resent *x,  t_floatarg speed, t_floatarg phase )
{
  float scaled_phase ;
  int i;
  if( phase < 0. )
    phase = 0. ;
  if( phase > 1. )
    phase = 1.;
	
  scaled_phase = phase * (float) x->framecount ;
  for( i = 0; i < x->N2; i++ ){
    x->frame_phase[i] = scaled_phase ;
    x->frame_incr[i] = speed ;
  }
//  post("ssap: speed reset to %f, phase reset to %f",speed,phase);
		
}

void resent_assist (t_resent *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:
      sprintf(dst,"(signal/bang) Input, Sample Trigger");
      break;
    }
  } else if (msg==2) {
    switch( arg){
    case 0: 
      sprintf(dst,"(signal) Output ");
      break;
    case 1:
      sprintf(dst,"(signal) Recording Sync");
      break;
    }
		
  }
}

void resent_tick(t_resent *x) {
  outlet_bang(x->m_bang);
}

void resent_init(t_resent *x,short initialized)
{
  int i;
  int last_framecount = x->framecount;

  x->lock = 1;
  
  if(!x->D)
    x->D = 256;
  if(!x->R)
    x->R = 44100;
  if(!power_of_two(x->winfac))
    x->winfac = 1;
  if(!power_of_two(x->overlap))
    x->overlap = 1;

  x->verbose = 0; // testing only
  
  x->N = x->D * x->overlap;
  x->Nw = x->N * x->winfac;
  limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
  x->N2 = (x->N)>>1;
  x->Nw2 = (x->Nw)>>1;
  x->inCount = -(x->Nw);
  x->current_frame = x->framecount = 0;
  x->fpos = x->last_fpos = 0;
  x->tadv = (float)x->D/(float)x->R;
  x->mult = 1. / (float) x->N;
  x->c_fundamental =  (float) x->R/( (x->N2)<<1 );
  x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
  x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
  if(x->duration < .005){
    x->duration = 1.0;
  }
  x->framecount =  x->duration/x->tadv ;
  x->read_me = 0;

  if(!initialized){
    x->frame_increment = 1.0 ;  
    x->mute = 0;
    x->playthrough = 0;
    x->sync = 0;
    x->frames_read = 0;

    x->Wanal = (float *) getbytes( (MAX_Nw) * sizeof(float));	
    x->Wsyn = (float *) getbytes( (MAX_Nw) * sizeof(float));	
    x->Hwin = (float *) getbytes( (MAX_Nw) * sizeof(float));	
    x->input = (float *) getbytes( MAX_Nw * sizeof(float) );	
    x->output = (float *) getbytes( MAX_Nw * sizeof(float) );
	x->buffer = (float *) getbytes( MAX_N * sizeof(float) );
    x->channel = (float *) getbytes( (MAX_N+2) * sizeof(float) );
    x->bitshuffle = (int *) getbytes( MAX_N * 2 * sizeof( int ) );
    x->trigland = (float *) getbytes( MAX_N * 2 * sizeof( float ) );
    x->c_lastphase_in = (float *) getbytes( (MAX_N2+1) * sizeof(float) );
    x->c_lastphase_out = (float *) getbytes( (MAX_N2+1) * sizeof(float) );
    x->composite_frame = (float *) getbytes( (MAX_N+2) * sizeof(float) );
    x->frame_incr = (float *) getbytes( MAX_N2 * sizeof(float) );
    x->store_incr = (float *) getbytes( MAX_N2 * sizeof(float) );
    x->frame_phase = (float *) getbytes( MAX_N2 * sizeof(float) );
    x->loveboat = (float **) getbytes(x->framecount * sizeof(float *));

    for(i=0;i<x->framecount;i++){
      x->loveboat[i] = (float *) getbytes((x->N+2) * sizeof(float));
      if(x->loveboat[i] == NULL){
				error("Insufficient Memory!");
				return;
      }
      memset((char *)x->loveboat[i],0,(x->N+2) * sizeof(float));
    }
  } else if(initialized == 1){
    for(i = 0; i < last_framecount; i++){
      freebytes(x->loveboat[i],0) ;
    }
    freebytes(x->loveboat,0);
    x->loveboat = (float **) getbytes(x->framecount * sizeof(float *));
    for(i=0;i<x->framecount;i++){
      x->loveboat[i] = (float *) getbytes((x->N+2) *sizeof(float));
      if(x->loveboat[i] == NULL){
				error("Insufficient Memory!");
				return;
      }
      memset((char *)x->loveboat[i],0,(x->N+2) * sizeof(float));
    }
  }
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
	memset((char *)x->c_lastphase_out,0,(x->N2+1)* sizeof(float));
	memset((char *)x->frame_incr,0,(x->N2)* sizeof(float));
	memset((char *)x->store_incr,0,(x->N2) * sizeof(float));
	memset((char *)x->frame_phase,0,(x->N2) * sizeof(float));

	
  init_rdft( x->N, x->bitshuffle, x->trigland);
  x->hopsize = x->N / x->overlap;	
  makewindows( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D);

  
  x->lock = 0;
}

void *resent_new(t_symbol *msg, short argc, t_atom *argv)
{
#if MSP
  t_resent *x = (t_resent *)newobject(resent_class);
//  x->m_bang = bangout((t_pxobject *)x);
//  x->m_clock = clock_new(x,(method)resent_tick);
  dsp_setup((t_pxobject *)x,1);
  outlet_new((t_pxobject *)x, "signal");
  outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
  t_resent *x = (t_resent *)pd_new(resent_class);
  outlet_new(&x->x_obj, gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
//  x->m_bang = outlet_new(&x->x_obj,gensym("bang"));
//  x->m_clock = clock_new(x,(void *)resent_tick);
#endif

  srand(clock());

  x->duration = atom_getfloatarg(0, argc, argv)/1000.0;
  x->overlap = atom_getfloatarg(1, argc, argv);
  x->winfac = atom_getfloatarg(2, argc, argv);
  x->D = sys_getblksize();
  x->R = sys_getsr();

  resent_init(x,0);

  return (x);
}

t_int *resent_perform(t_int *w)
{

  int iphase, amp, freq, i, j;
  float fincr;	
  float fpos;
  ////////////////////////////////////////////// 
  t_resent *x = (t_resent *) (w[1]);
  float *frame_incr = x->frame_incr ;
  float *frame_phase = x->frame_phase ;
  float *composite_frame = x->composite_frame ;
  float in_sample;
  t_float *in = (t_float *)(w[2]);
  t_float *out = (t_float *)(w[3]);
  t_float *sync_vec = (t_float *)(w[4]);
  t_int n = w[5];
	
  /* dereference structure */	

  int inCount = x->inCount;
  int R = x->R;
  int N = x->N;
  int N2 = x->N2;
  int D = x->D;
  int Nw = x->Nw;
  float *Wanal = x->Wanal;
  float *Wsyn = x->Wsyn;		
  float *input = x->input;
  float *output = x->output;
  float *buffer = x->buffer;
  float *channel = x->channel;
  float fframe = x->current_frame ;
  float last_fpos = x->last_fpos ;
  int framecount = x->framecount;
  float mult = x->mult ;
  int *bitshuffle = x->bitshuffle;
  float *trigland = x->trigland ;
  float *c_lastphase_in = x->c_lastphase_in;
  float *c_lastphase_out = x->c_lastphase_out;
  float c_fundamental = x->c_fundamental;
  float c_factor_in = x->c_factor_in;
  float c_factor_out = x->c_factor_out;
  float sync = x->sync;
  
  sync = (float)x->frames_read / (float)x->framecount;
  
  if(x->mute || x->lock){
	while(n--){
	  *out++ = 0.0;
	  *sync_vec++ = sync;
	}
	return (w+6);
  }
  
  inCount += D;


  
  if(x->read_me){
    for ( j = 0 ; j < Nw - D ; j++ ){
      input[j] = input[j+D];
    }
    if(x->playthrough){
      for (j = Nw - D; j < Nw; j++) {
				in_sample = input[j] = *in++;
				*out++ = in_sample * 0.5; // scale down
				*sync_vec++ = sync;
		  }
    } else{
      for (j = Nw - D; j < Nw; j++) {
				input[j] = *in++;
				*out++ = 0.0;
				*sync_vec++ = sync;
      }
    }

    fold( input, Wanal, Nw, buffer, N, inCount );	
    rdft( N, 1, buffer, bitshuffle, trigland );

    convert( buffer, x->loveboat[(x->frames_read)++], N2, c_lastphase_in, c_fundamental, c_factor_in );
    if(x->frames_read >= x->framecount){
      x->read_me = 0;
    } 	
  } 
  else {
    for( i = 0 ; i < N2; i++ ){
      amp = i<<1;
      freq = amp + 1 ;
      iphase = frame_phase[i]  ;
      if( iphase < 0 )
		iphase = 0;
      if( iphase > framecount - 1 )
		iphase = framecount - 1;
      composite_frame[amp] = x->loveboat[iphase][amp];
      composite_frame[freq] = x->loveboat[iphase][freq];
      frame_phase[i] += frame_incr[i] ;
      while( frame_phase[i] > framecount - 1)
		frame_phase[i] -= framecount ;
      while( frame_phase[i] < 0. )
		frame_phase[i] += framecount ;
    }

    unconvert(composite_frame, buffer, N2, c_lastphase_out, c_fundamental, c_factor_out);

    rdft( N, -1, buffer, bitshuffle, trigland );
    overlapadd( buffer, N, Wsyn, output, Nw, inCount );

    for (j = 0; j < D; j++){
      *out++ = output[j] * mult;
      *sync_vec++ = sync;
    }
    for (j = 0; j < Nw - D; j++){
      output[j] = output[j+D];
    }
  
    for (j = Nw - D; j < Nw; j++){
      output[j] = 0.;
    }
  }
	
  /* restore state variables */

  x->inCount = inCount %Nw;
  x->current_frame = fframe;
  x->last_fpos = last_fpos;
  x->sync = sync;


  return (w+6);
}
	


void resent_acquire_sample(t_resent *x)
{
  x->read_me = 1;
  x->frames_read = 0;

  return;
}

void resent_mute(t_resent *x, t_floatarg tog)
{
  x->mute = tog;	
}

void resent_playthrough(t_resent *x, t_floatarg state)
{
  x->playthrough = state;
}

void resent_linephase(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
{
  int bin1, bin2;
  float phase1, phase2, bindiff;
  int i;
  float m1, m2;

  bin1 = (int) atom_getfloatarg(0, argc, argv);
  phase1 = atom_getfloatarg(1, argc, argv) * x->framecount;
  bin2 = (int) atom_getfloatarg(2, argc, argv);
  phase2 = atom_getfloatarg(3, argc, argv) * x->framecount;

  if( bin1 > x->N2 || bin2 > x->N2 ){
    error("too high bin number");
    return;
  }
  bindiff = bin2 - bin1;
  if( bindiff < 1 ){
    error("make bin2 higher than bin 1, bye now");
    return;
  }
  for( i = bin1; i < bin2; i++ ){
    m2 = (float) i / bindiff;
    m1 = 1. - m2;
    x->frame_phase[i] = m1 * phase1 + m2 * phase2;
  }
}

void resent_randphase(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
{

  float minphase, maxphase;
  int i;
  int framecount = x->framecount;
  
  minphase = atom_getfloatarg(0, argc, argv);
  maxphase = atom_getfloatarg(1, argc, argv);
	
//  post("minphase %f maxphase %f",minphase, maxphase);
  if(minphase < 0.0)
    minphase = 0.0;
  if( maxphase > 1.0 )
    maxphase = 1.0;
  	
  for( i = 0; i < x->N2; i++ ){
    x->frame_phase[i] = (int) (resent_randf( minphase, maxphase ) * (float) (framecount - 1) ) ;	
  } 
}

void resent_randspeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
{

  float minspeed, maxspeed;
  int i;


  minspeed = atom_getfloatarg(0, argc, argv);
  maxspeed = atom_getfloatarg(1, argc, argv);

  for( i = 0; i < x->N2; i++ ){
    x->frame_incr[i] = resent_randf(minspeed, maxspeed);
  } 
}

void resent_linespeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
{
  int bin1, bin2;
  float speed1, speed2, bindiff;
  int i;
  float m1, m2;

  bin1 = (int) atom_getfloatarg(0, argc, argv);
  speed1 = atom_getfloatarg(1, argc, argv);
  bin2 = (int) atom_getfloatarg(2, argc, argv);
  speed2 = atom_getfloatarg(3, argc, argv);

  if( bin1 > x->N2 || bin2 > x->N2 ){
    error("too high bin number");
    return;
  }
  bindiff = bin2 - bin1;
  if( bindiff < 1 ){
    error("make bin2 higher than bin 1, bye now");
    return;
  }
  for( i = bin1; i < bin2; i++ ){
    m2 = (float) i / bindiff;
    m1 = 1. - m2;
    x->frame_incr[i] = m1 * speed1 + m2 * speed2;
  }
}
void resent_dsp(t_resent *x, t_signal **sp, short *count)
{
  if(x->R != sp[0]->s_sr || x->D != sp[0]->s_n){
    x->R = sp[0]->s_sr;
    x->D = sp[0]->s_n;
    if(x->verbose)
      post("new vector size: %d, new sampling rate:%d",x->D,x->R);
    resent_init(x,1);
  }		
  dsp_add(resent_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

float resent_randf(float min,float max)
{
  float rval;
  rval = (float) (rand() % RAND_MAX) / (float) RAND_MAX;
  return ( min + (max-min) * rval );	
}
