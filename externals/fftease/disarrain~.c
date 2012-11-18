#include "MSPd.h"
#include "fftease.h"

#if MSP
void *disarrain_class;
#endif 

#if PD
static t_class *disarrain_class;
#endif

#define OBJECT_NAME "disarrain~"


typedef struct _disarrain
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
  float *last_channel;
  float *composite_channel;
  float *output;
  int overlap;
  int winfac;
  //

  int *shuffle_mapping;
  int *last_shuffle_mapping;
  int *shuffle_tmp; // work space for making a new distribution
  int shuffle_count;// number of bins to swap
  int last_shuffle_count;// ditto from last shuffle mapping
  int max_bin;
  //
  float mult; 
  float *trigland;
  int *bitshuffle;
  //
  void *list_outlet;
  t_atom *list_data;
  short mute;
  short bypass;
  float frame_duration; // duration in seconds of a single frame
  float interpolation_duration; // duration in seconds of interpolation
  int interpolation_frames; // number of frames to interpolate
  int frame_countdown; // keep track of position in interpolation
  int overlap_factor;// determines window size, etc.
  float top_frequency;// for remapping spectrum
  int perform_method;// 0 for lean, 1 for full conversion
  // for convert
//  float *c_lastphase_in;
//  float *c_lastphase_out;
  float c_fundamental;
  float c_factor_in;
  float c_factor_out;
  // check switching algorithm
  short lock;// lock for switching mapping arrays, but not used now
  short force_fade; // new fadetime set regardless of situation
  short force_switch;// binds new distribution to change of bin count
} t_disarrain;

void *disarrain_new(t_symbol *msg, short argc, t_atom *argv);

t_int *disarrain_perform_lean(t_int *w);
t_int *disarrain_perform_full(t_int *w);
void disarrain_dsp(t_disarrain *x, t_signal **sp, short *count);
void disarrain_assist(t_disarrain *x, void *b, long m, long a, char *s);
void disarrain_switch_count (t_disarrain *x, t_floatarg i);
void disarrain_topfreq (t_disarrain *x, t_floatarg freq);
void disarrain_fadetime (t_disarrain *x, t_floatarg f);
void reset_shuffle( t_disarrain *x );
void disarrain_showstate( t_disarrain *x );
void disarrain_list (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv);
void disarrain_setstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv);
void disarrain_isetstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv);
int rand_index(int max);
void disarrain_mute(t_disarrain *x, t_floatarg toggle);
void disarrain_bypass(t_disarrain *x, t_floatarg toggle);
void copy_shuffle_array(t_disarrain *x);
void interpolate_frames_to_channel(t_disarrain *x);
void disarrain_killfade(t_disarrain *x);
void disarrain_forcefade(t_disarrain *x, t_floatarg toggle);
void disarrain_init(t_disarrain *x, short initialized);
void disarrain_free(t_disarrain *x);
void disarrain_overlap(t_disarrain *x, t_floatarg o);
void disarrain_winfac(t_disarrain *x, t_floatarg o);
void disarrain_fftinfo(t_disarrain *x);
void disarrain_force_switch(t_disarrain *x, t_floatarg toggle);


#if MSP
void main(void)
{
  setup((t_messlist **)&disarrain_class, (method)disarrain_new, (method)disarrain_free, 
  (short)sizeof(t_disarrain), 0, A_GIMME, 0);
  addmess((method)disarrain_dsp, "dsp", A_CANT, 0);
  addint((method)disarrain_switch_count);
  addbang((method)reset_shuffle);
  addmess((method)disarrain_showstate,"showstate",0);
  addmess ((method)disarrain_list, "list", A_GIMME, 0);
  addmess ((method)disarrain_setstate, "setstate", A_GIMME, 0);
  addmess ((method)disarrain_isetstate, "isetstate", A_GIMME, 0);
  addmess((method)disarrain_assist,"assist",A_CANT,0);
  addmess ((method)disarrain_mute, "mute", A_FLOAT, 0);
  addmess ((method)disarrain_topfreq, "topfreq", A_FLOAT, 0);
  addmess ((method)disarrain_fadetime, "fadetime", A_FLOAT, 0);
  addmess ((method)disarrain_bypass, "bypass", A_FLOAT, 0);
  addmess ((method)disarrain_forcefade, "forcefade", A_FLOAT, 0);
  addmess ((method)disarrain_force_switch, "force_switch", A_FLOAT, 0);
  addmess ((method)disarrain_switch_count, "switch_count", A_FLOAT, 0);
  addmess ((method)disarrain_killfade, "killfade", 0);
  addmess ((method)reset_shuffle, "reset_shuffle", 0);
  
  addmess((method)disarrain_overlap, "overlap",  A_DEFFLOAT, 0);
  addmess((method)disarrain_winfac, "winfac",  A_DEFFLOAT, 0);
  addmess((method)disarrain_fftinfo, "fftinfo", 0);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void disarrain_tilde_setup(void)
{
  disarrain_class = class_new(gensym("disarrain~"), (t_newmethod)disarrain_new, 
			 (t_method)disarrain_free ,sizeof(t_disarrain), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(disarrain_class, t_disarrain, x_f);
  class_addmethod(disarrain_class, (t_method)disarrain_dsp, gensym("dsp"), 0);
  class_addmethod(disarrain_class, (t_method)disarrain_showstate, gensym("showstate"), 0);
  class_addmethod(disarrain_class, (t_method)disarrain_list, gensym("list"), A_GIMME, 0);
  class_addmethod(disarrain_class, (t_method)disarrain_setstate, gensym("setstate"), A_GIMME, 0);
  class_addmethod(disarrain_class, (t_method)disarrain_isetstate, gensym("isetstate"), A_GIMME, 0);

  class_addmethod(disarrain_class, (t_method)disarrain_mute, gensym("mute"), A_FLOAT, 0);
  class_addmethod(disarrain_class, (t_method)disarrain_topfreq, gensym("topfreq"), A_FLOAT, 0);
  class_addmethod(disarrain_class, (t_method)disarrain_fadetime, gensym("fadetime"), A_FLOAT, 0);
  class_addmethod(disarrain_class, (t_method)disarrain_bypass, gensym("bypass"), A_FLOAT, 0);
  class_addmethod(disarrain_class, (t_method)disarrain_forcefade, gensym("forcefade"), A_FLOAT, 0);
class_addmethod(disarrain_class, (t_method)disarrain_force_switch, gensym("force_switch"), A_FLOAT, 0);


//  class_addmethod(disarrain_class, (t_method)disarrain_killfade, gensym("reset"), A_FLOAT, 0);
  class_addmethod(disarrain_class, (t_method)reset_shuffle, gensym("bang"),  0);
  class_addmethod(disarrain_class, (t_method)reset_shuffle, gensym("reset_shuffle"),  0);
  class_addmethod(disarrain_class, (t_method)disarrain_switch_count, gensym("switch_count"), A_FLOAT, 0);
  
  class_addmethod(disarrain_class, (t_method)disarrain_overlap, gensym("overlap"), A_DEFFLOAT,0);
  class_addmethod(disarrain_class, (t_method)disarrain_winfac, gensym("winfac"), A_DEFFLOAT,0);
  class_addmethod(disarrain_class, (t_method)disarrain_fftinfo, gensym("fftinfo"), 0);
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void disarrain_free(t_disarrain *x)
{
#if MSP
    dsp_free((t_pxobject *) x);
#endif
     freebytes(x->Wanal, x->Nw * sizeof(float));	
     freebytes(x->Wsyn, x->Nw * sizeof(float));	
     freebytes(x->input, x->Nw * sizeof(float));	
     freebytes(x->Hwin, x->Nw * sizeof(float));
     freebytes(x->buffer, x->N * sizeof(float));
     freebytes(x->channel, (x->N+2) * sizeof(float));
     freebytes(x->last_channel, (x->N+2) * sizeof(float));
     freebytes(x->composite_channel, x->N+2 * sizeof(float));
     freebytes(x->output, x->Nw * sizeof(float));
     freebytes(x->bitshuffle, (x->N * 2) * sizeof(int));
     freebytes(x->trigland, x->N * 2 * sizeof(float));
     freebytes(x->shuffle_mapping, x->N2 * sizeof(int)) ;
     freebytes(x->last_shuffle_mapping, x->N2 * sizeof(int)) ;
     freebytes(x->shuffle_tmp, x->N2 * sizeof(int)) ;
     freebytes(x->list_data,(x->N+2) * sizeof(t_atom)) ;
//     freebytes(x->c_lastphase_in, (x->N2+1)*sizeof(float));
//     freebytes(x->c_lastphase_out,(x->N2+1)* sizeof(float));
}



void disarrain_init(t_disarrain *x, short initialized)
{
  int i;
  float curfreq;

  if(!power_of_two(x->winfac)){
    x->winfac = 1;
  }
  if(!power_of_two(x->overlap)){
    x->overlap = 4;
  }	
  x->N = x->D * x->overlap;
  x->Nw = x->N * x->winfac;	
  limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
  x->c_fundamental =  (float) x->R/(float)( (x->N2)<<1 );
  x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
  x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;

  x->N2 = (x->N)>>1;
  x->Nw2 = (x->Nw)>>1;
  x->inCount = -(x->Nw);
  x->mult = 1. / (float) x->N;
  
  x->lock = 1; // not good enough
  
  if(initialized == 0){
    x->Wanal = (float *) calloc(MAX_Nw , sizeof(float));	
    x->Wsyn = (float *) calloc(MAX_Nw , sizeof(float));	
    x->input = (float *) calloc(MAX_Nw , sizeof(float));	
    x->Hwin = (float *) calloc(MAX_Nw , sizeof(float));
    x->buffer = (float *) calloc(MAX_N , sizeof(float));
    x->channel = (float *) calloc(MAX_N+2 , sizeof(float));
    x->last_channel = (float *) calloc(MAX_N+2 , sizeof(float));
    x->composite_channel = (float *) calloc(MAX_N+2 , sizeof(float));
    x->output = (float *) calloc(MAX_Nw , sizeof(float));
    x->bitshuffle = (int *) calloc((MAX_N * 2) , sizeof(int));
    x->trigland = (float *) calloc(MAX_N * 2 , sizeof(float));
    x->shuffle_mapping = (int *) calloc( MAX_N2 , sizeof(int) ) ;
    x->last_shuffle_mapping = (int *) calloc( MAX_N2 , sizeof(int) ) ;
    x->shuffle_tmp = (int *) calloc( MAX_N2 , sizeof(int) ) ;
    x->list_data = (t_atom *) calloc((MAX_N+2) , sizeof(t_atom) ) ;
    x->mute = 0;
    x->bypass = 0;
    x->force_fade = 0;
    x->interpolation_duration = 0.1; //seconds
  } 
		memset((char *)x->input,0,x->Nw * sizeof(float));
		memset((char *)x->output,0,x->Nw * sizeof(float));
		memset((char *)x->buffer,0,x->N * sizeof(float));
		memset((char *)x->channel,0,(x->N+2) * sizeof(float));
		memset((char *)x->last_channel,0,(x->N+2) * sizeof(float));


  init_rdft( x->N, x->bitshuffle, x->trigland);
  makewindows( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D);
 
  
  if(initialized != 2){
	  if( x->top_frequency < x->c_fundamental || x->top_frequency > 20000) {
	    x->top_frequency = 1000.0 ;
	  }
	  x->max_bin = 1;  
	  curfreq = 0;
	  while( curfreq < x->top_frequency ) {
	    ++(x->max_bin);
	    curfreq += x->c_fundamental ;
	  }
	  for( i = 0; i < x->N2; i++ ) {
	    x->shuffle_mapping[i] = x->last_shuffle_mapping[i] = i*2;
	  }
	  reset_shuffle(x); // set shuffle lookup
	  copy_shuffle_array(x);// copy it to the last lookup (for interpolation)
	  x->frame_duration = (float) x->D / (float) x->R;
	  x->interpolation_frames = x->interpolation_duration / x->frame_duration;
	  x->frame_countdown = 0;
	  x->shuffle_count = 0;
	  x->last_shuffle_count = 0;
  }
  x->lock = 0;
 }


void disarrain_force_switch(t_disarrain *x, t_floatarg f)
{
	x->force_switch = (short)f;
}

void disarrain_fadetime (t_disarrain *x, t_floatarg f)
{
  int frames;
  float duration;
  
  // forcefade allows forcing new fadetime at any time
  if(! x->force_fade) {
#if MSP
    if(!sys_getdspstate()){
      return; // DSP is inactive
    }
#endif
    if(x->frame_countdown) {
      error("disarrain: fade in progress, fadetime reset blocked");
      return;
    }
  }
  
  duration = f * .001;
  frames = duration / x->frame_duration;
  if( frames <= 1){
    error("%s: too short fade",OBJECT_NAME);
    return;
  }
  x->interpolation_duration = f * .001;
  x->interpolation_frames = frames;

}

void disarrain_killfade(t_disarrain *x)
{
  x->frame_countdown = 0;

}

void disarrain_topfreq (t_disarrain *x, t_floatarg freq)
{
  float funda = (float) x->R / (2. * (float) x->N) ;
  float curfreq;
 
  if( freq < funda || freq > 20000) {
    freq = 1000.0 ;
  }
  x->max_bin = 1;  
  curfreq = 0;
  while( curfreq < freq ) {
    ++(x->max_bin);
    curfreq += funda ;
  }
}

void disarrain_assist (t_disarrain *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0: sprintf(dst,"(signal) Input"); break;
    }
  } else if (msg==2) {
    switch (arg) {
    case 0:	sprintf(dst,"(signal) Output"); break;
    case 1: sprintf(dst,"(signal) Interpolation Sync"); break;
    case 2: sprintf(dst,"(list) Current State"); break;
    }
  }
}

void *disarrain_new(t_symbol *msg, short argc, t_atom *argv)
{

#if MSP
  t_disarrain *x = (t_disarrain *)newobject(disarrain_class);
  x->list_outlet = listout((t_pxobject *)x);
  dsp_setup((t_pxobject *)x,1);
  outlet_new((t_pxobject *)x, "signal");
  outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
  t_disarrain *x = (t_disarrain *)pd_new(disarrain_class);
  outlet_new(&x->x_obj, gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
  x->list_outlet = outlet_new(&x->x_obj,gensym("list"));
#endif

  srand(time(0));

  x->D = sys_getblksize();
  x->R = sys_getsr(); 
  x->top_frequency = atom_getfloatarg(0,argc,argv);
  x->overlap_factor = atom_getintarg(1,argc,argv);
  x->winfac = atom_getintarg(2,argc,argv);

  disarrain_init(x,0);
  return (x);
}

void disarrain_forcefade(t_disarrain *x, t_floatarg toggle)
{
  x->force_fade = (short)toggle;	
}

void disarrain_mute(t_disarrain *x, t_floatarg toggle)
{
  x->mute = (short)toggle;	
}

void disarrain_bypass(t_disarrain *x, t_floatarg toggle)
{
  x->bypass = (short)toggle;	
}

void disarrain_overlap(t_disarrain *x, t_floatarg df)
{
int o = (int)df;
  if(!power_of_two(o)){
    error("%d is not a power of two",o);
    return;
  }
  x->overlap = (int)o;
  disarrain_init(x,1);
}

void disarrain_winfac(t_disarrain *x, t_floatarg f)
{
int wf = (int)f;
  if(!power_of_two(wf)){
    error("%f is not a power of two",wf);
    return;
  }
  x->winfac = wf;
  disarrain_init(x,2); /* calling lighter reinit routine */
}

void disarrain_fftinfo( t_disarrain *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


// lean convert perform method
t_int *disarrain_perform_lean(t_int *w)
{
  t_disarrain *x = (t_disarrain *) (w[1]);
  t_float *in = (t_float *)(w[2]);
  t_float *out = (t_float *)(w[3]);
  t_float *vec_sync = (t_float *)(w[4]);
  int n = w[5];
  int R = x->R;
  int Nw = x->Nw;
  int N = x->N ;
  int N2 = x-> N2;
  int Nw2 = x->Nw2;
  float *Wanal = x->Wanal;
  float *Wsyn = x->Wsyn;
  float *Hwin = x->Hwin;
  	
  float *input = x->input; 
  float *output = x->output;
  float *buffer = x->buffer;
  float *channel = x->channel;
  float *last_channel = x->last_channel;
  int		i,j;
  int inCount = x->inCount;
				
  int	D = x->D;
  float tmp;
  float ival = 0.0;
  int *shuffle_mapping = x->shuffle_mapping;
  int shuffle_count = x->shuffle_count;
  int *last_shuffle_mapping = x->last_shuffle_mapping;
  int last_shuffle_count = x->last_shuffle_count;	
  float mult = x->mult ;
  int *bitshuffle = x->bitshuffle;
  float *trigland = x->trigland ;
  int frame_countdown = x->frame_countdown; // will read from variable
  int interpolation_frames = x->interpolation_frames;
  if( x->mute || x->lock ){
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

  inCount += D;

  for ( j = 0 ; j < Nw - D ; j++ ){
    input[j] = input[j+D];
  }
  for ( j = Nw - D; j < Nw; j++ ) {
    input[j] = *in++;
  }

  fold(input, Wanal, Nw, buffer, N, inCount);	
  rdft(N, 1, buffer, bitshuffle, trigland);
    
  leanconvert(buffer, channel, N2);

  // first time for interpolation, just do last frame 

  if(frame_countdown == interpolation_frames){

    for( i = 0, j = 0; i < last_shuffle_count ; i++, j+=2){
      tmp = channel[j];
      channel[j] = channel[last_shuffle_mapping[i]];
      channel[last_shuffle_mapping[i]] = tmp;
    }
    --frame_countdown;
  } 
  else if( frame_countdown > 0 ){
    ival = (float)frame_countdown/(float)interpolation_frames;
    // copy current frame to lastframe
    for(j = 0; j < N; j+=2){
      last_channel[j] = channel[j];
    }	
    // make last frame swap
    for(i = 0, j = 0; i < last_shuffle_count ; i++, j+=2){
      tmp = last_channel[j];
      last_channel[j] = last_channel[last_shuffle_mapping[i]];
      last_channel[last_shuffle_mapping[i]] = tmp;

    }	
    // make current frame swap
    for( i = 0, j = 0; i < shuffle_count ; i++, j+=2){
      tmp = channel[j];
      channel[j]  = channel[shuffle_mapping[i]];
      channel[shuffle_mapping[i]]  = tmp;

    }
    // now interpolate between the two

    for(j = 0; j < N; j+=2){
      channel[j] = channel[j] + ival * (last_channel[j] - channel[j]);
    }
		
    --frame_countdown;
    if(frame_countdown <= 0){
      copy_shuffle_array(x);
    }
  } else {
    // otherwise straight swapping
    for( i = 0, j = 0; i < shuffle_count ; i++, j+=2){
      tmp = channel[j];
      channel[j]  = channel[ shuffle_mapping[i]];
      channel[shuffle_mapping[i]] = tmp;     
    }
  }
  leanunconvert( channel, buffer,  N2 );

  rdft( N, -1, buffer, bitshuffle, trigland );
  overlapadd( buffer, N, Wsyn, output, Nw, inCount);

  for ( j = 0; j < D; j++ )
    *out++ = output[j] * mult;

  for ( j = 0; j < Nw - D; j++ )
    output[j] = output[j+D];
			
  for ( j = Nw - D; j < Nw; j++ )
    output[j] = 0.;
/* send out sync signal */
  for(j = 0; j < n; j++){
  	vec_sync[j] = ival;
  }
  /* restore state variables */
  x->inCount = inCount % Nw;
  x->frame_countdown = frame_countdown;
  return (w+6);
}		


void interpolate_frames_to_channel(t_disarrain *x)
{
  float ival;
  float tmp;
  int i,j;
  int frame_countdown = x->frame_countdown;
  int interpolation_frames = x->interpolation_frames;
  float *channel = x->channel;
  float *last_channel = x->last_channel;
  int *shuffle_mapping = x->shuffle_mapping;
  int shuffle_count = x->shuffle_count;
  int *last_shuffle_mapping = x->shuffle_mapping;
  int last_shuffle_count = x->shuffle_count;	
  int local_max_bins;
  int N = x->N;
	
  ival = (float)frame_countdown/(float)interpolation_frames;

//  post("interpolation:%f",ival);
  local_max_bins = (shuffle_count > last_shuffle_count)? shuffle_count : last_shuffle_count;
  // copy channel (only amplitudes)
  for(j = 0; j < N; j+=2){
    last_channel[j] = channel[j];
  }
  // make last frame
  for( i = 0, j = 0; i < last_shuffle_count ; i++, j+=2){
    tmp = last_channel[j];
    last_channel[j] = last_channel[last_shuffle_mapping[i]];
    last_channel[last_shuffle_mapping[i]] = tmp;
  }
  // make current frame
  for( i = 0, j = 0; i < shuffle_count ; i++, j+=2){
    tmp = channel[j];
    channel[j]  = channel[shuffle_mapping[i]];
    channel[shuffle_mapping[i]]  = tmp;
  }
  // now interpolate between the two

  for(j = 0; j < N; j+=2){
    // channel[j] = channel[j] + ival * (last_channel[j] - channel[j]);
		
    // or better?
    channel[j] += ival * (last_channel[j] - channel[j]);
  }
}


void disarrain_switch_count (t_disarrain *x, t_floatarg f)
{
int i = f;
/*
#if MSP
  if(! sys_getdspstate()){
    return; // DSP is inactive
  }
#endif
*/
  if( x->frame_countdown && !x->force_fade){
    error("%s: fade in progress, no action taken",OBJECT_NAME);
    return;
  }
  if( i < 0 ){
    i = 0;
  }
  if( i > x->N2 ) {
    i = x->N2;
  }
  copy_shuffle_array(x);
  x->last_shuffle_count = x->shuffle_count;
  x->shuffle_count = i;
  x->frame_countdown = x->interpolation_frames; // force interpolation
}


void reset_shuffle (t_disarrain *x)
{
  int i;
  int temp, p1, p2;
  int max = x->max_bin;
  int  N2 = x->N2;
  
  int *shuffle_tmp = x->shuffle_tmp;
  int *shuffle_mapping = x->shuffle_mapping;

  
  copy_shuffle_array(x);

  for( i = 0; i < N2; i++ ) {
    shuffle_tmp[i] = i;
  }
  // improve this algorithm
		
  for( i = 0; i < max; i++ ) {
    p1 = shuffle_tmp[ rand_index( max ) ];
    p2 = shuffle_tmp[ rand_index( max ) ];
    temp = shuffle_tmp[p1];
    shuffle_tmp[ p1 ] = shuffle_tmp[ p2 ];
    shuffle_tmp[ p2 ] = temp;
  }
	
	
	
  // now map to amplitude channels
  for( i = 0; i < N2; i++ ) {
    shuffle_tmp[i] *= 2;
  }
	
  // force interpolation
  x->frame_countdown = x->interpolation_frames;	
  
  x->lock = 1;
  for( i = 0; i < N2; i++ ) {
    shuffle_mapping[i] = shuffle_tmp[i];
  }

  x->lock = 0;
}

void copy_shuffle_array(t_disarrain *x)
{
  int i;
  int N2 = x->N2;
  int *shuffle_mapping = x->shuffle_mapping;
  int *last_shuffle_mapping = x->last_shuffle_mapping;	


  for(i = 0; i<N2; i++){
    last_shuffle_mapping[i] = shuffle_mapping[i];
  }
  x->last_shuffle_count = x->shuffle_count;

}

int rand_index(int max) {

  return (rand() % max);
}

void disarrain_dsp(t_disarrain *x, t_signal **sp, short *count)
{
  long i;

  if(x->D != sp[0]->s_n ||x->D != sp[0]->s_n ) {
    x->R = sp[0]->s_sr;
    x->D = sp[0]->s_n;
    disarrain_init(x,1);
  }

  dsp_add(disarrain_perform_lean, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);

}

// ENTER STORED SHUFFLE
void disarrain_list (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv) {
  short i;
  int ival;
//  post("list message called");
  x->shuffle_count = argc;
//  post("list: count now %d",x->shuffle_count );
  for (i=0; i < argc; i++) {
  #if MSP
    ival = argv[i].a_w.w_long;
  #endif
  
  #if PD
    ival = atom_getfloatarg(i,argc,argv);
  #endif
    if (ival < x->N2) {
      x->shuffle_mapping[i] = ival;
//      post("set %d to %d",i, x->shuffle_mapping[ i ]);
    } else {
//      post ("%d out of range",ival);
    }
		
  }
//  post("last val is %d", x->shuffle_mapping[argc - 1]);
  return;
}


void disarrain_isetstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv) {
  short i;
  int ival;
  
//  x->last_shuffle_count = x->shuffle_count;
  
  copy_shuffle_array(x);
  x->shuffle_count = argc;
  
  
//  x->lock = 1;
    
  for (i=0; i < argc; i++) {
    ival = 2 * atom_getfloatarg(i,argc,argv);

    if ( ival < x->N2 && ival >= 0) {
      x->shuffle_mapping[ i ] = ival;
    }else {
    	error("%s: %d is out of range",OBJECT_NAME, ival);
    }
  }

//  x->lock = 0;
  x->frame_countdown = x->interpolation_frames;
  
  return;
}

void disarrain_setstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv) {
  short i;
  int ival;
  
  x->shuffle_count = argc;
  for (i=0; i < argc; i++) {
	  ival = 2 *atom_getfloatarg(i,argc,argv);

    if ( ival < x->N2 && ival >= 0) {
      x->shuffle_mapping[ i ] = ival;
    } else {
    	error("%s: %d is out of range",OBJECT_NAME, ival);
    }
  }
  return;
}

// REPORT CURRENT SHUFFLE STATUS
void disarrain_showstate (t_disarrain *x ) {

  t_atom *list_data = x->list_data;

  short i;
#if MSP
  for( i = 0; i < x->shuffle_count; i++ ) {
    SETLONG(list_data+i,x->shuffle_mapping[i]/2);
  }
#endif

#if PD
  for( i = 0; i < x->shuffle_count; i++ ) {
    SETFLOAT(list_data+i,(float)x->shuffle_mapping[i]/2);
  }
#endif	
  outlet_list(x->list_outlet,0,x->shuffle_count,list_data);

  return;
}

