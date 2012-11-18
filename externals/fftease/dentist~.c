#include "MSPd.h"
#include "fftease.h"

#if MSP
void *dentist_class;
#endif 

#if PD
static t_class *dentist_class;
#endif

#define OBJECT_NAME "dentist~"

typedef struct _dentist
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
  short *bin_selection;
  short *last_bin_selection;
  int *active_bins;
  int tooth_count;
  int ramp_frames;
  int frames_left;
  float frame_duration;
  int max_bin;
  //
  float mult; 
  float *trigland;
  int *bitshuffle;
  int overlap;
  int winfac;
  float topfreq;
  float funda;
  //
  void *list_outlet;
  short direct_update;
  short mute;
  t_atom *list_data;
  short interpolate_singles;
  float sync;
} t_dentist;

void *dentist_new(t_symbol *msg, short argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *dentist_perform(t_int *w);
void dentist_dsp(t_dentist *x, t_signal **sp, short *count);
void dentist_assist(t_dentist *x, void *b, long m, long a, char *s);
void set_switch_bins (t_dentist *x, int i);
void reset_shuffle(t_dentist *x);
void dentist_showstate(t_dentist *x);
void dentist_direct_update(t_dentist *x, t_floatarg toggle);
void dentist_mute(t_dentist *x, t_floatarg toggle);
void dentist_setstate(t_dentist *x, t_symbol *msg, short argc, t_atom *argv);
void dentist_ramptime(t_dentist *x, t_floatarg ramp_ms);
int rand_index(int max);
void dentist_init(t_dentist *x, short initialized);
void dentist_bins_pd (t_dentist *x, t_floatarg i);
void dentist_topfreq(t_dentist *x, t_floatarg f);
void dentist_free(t_dentist *x);
void dentist_toothcount(t_dentist *x, t_floatarg newcount);
void dentist_scramble(t_dentist *x);
void dentist_activate_bins(t_dentist *x, t_floatarg f);
void dentist_interpolate_singles(t_dentist *x, t_floatarg f);
void dentist_overlap(t_dentist *x, t_floatarg o);
void dentist_winfac(t_dentist *x, t_floatarg o);
void dentist_fftinfo(t_dentist *x);
void dentist_mute(t_dentist *x, t_floatarg toggle);


#if MSP
void main(void)
{
  setup((t_messlist **)&dentist_class, (method)dentist_new, (method)dsp_free, 
  (short)sizeof(t_dentist), 0, A_GIMME, 0);
  addmess((method)dentist_dsp, "dsp", A_CANT, 0);
  addint((method)set_switch_bins);
//  addbang((method)reset_shuffle);
  addmess((method)dentist_showstate,"showstate",0);
//  addmess((method)dentist_direct_update,"direct_update",A_FLOAT, 0);
  addmess((method)dentist_mute,"mute",A_FLOAT, 0);
  addmess((method)dentist_setstate, "setstate", A_GIMME, 0);
  addmess((method)dentist_ramptime, "ramptime", A_FLOAT, 0);
  addmess((method)dentist_topfreq, "topfreq", A_FLOAT, 0);
  addmess((method)dentist_toothcount, "toothcount", A_FLOAT, 0);
//  addmess((method)dentist_activate_bins, "activate_bins", A_FLOAT, 0);
  addmess((method)dentist_interpolate_singles, "interpolate_singles", A_FLOAT, 0);
  addmess((method)dentist_scramble, "scramble", 0);
  addmess((method)dentist_assist,"assist",A_CANT,0);
  addmess((method)dentist_overlap, "overlap",  A_DEFFLOAT, 0);
  addmess((method)dentist_winfac, "winfac",  A_DEFFLOAT, 0);
  addmess((method)dentist_fftinfo, "fftinfo", 0);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void dentist_tilde_setup(void)
{
  dentist_class = class_new(gensym("dentist~"), (t_newmethod)dentist_new, 
			 (t_method)dentist_free ,sizeof(t_dentist), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(dentist_class, t_dentist, x_f);
  class_addmethod(dentist_class,(t_method)dentist_dsp,gensym("dsp"),0);
  class_addmethod(dentist_class,(t_method)dentist_mute,gensym("mute"),A_FLOAT,0);
  class_addmethod(dentist_class,(t_method)dentist_showstate,gensym("showstate"),0);
//  class_addmethod(dentist_class,(t_method)dentist_direct_update,gensym("direct_update"),A_FLOAT,0);
  class_addmethod(dentist_class,(t_method)dentist_setstate,gensym("setstate"),A_GIMME,0);
  class_addmethod(dentist_class,(t_method)dentist_ramptime,gensym("ramptime"),A_FLOAT,0);
//  class_addmethod(dentist_class,(t_method)dentist_bins_pd,gensym("dentist_bins"),A_FLOAT,0);
  class_addmethod(dentist_class,(t_method)dentist_topfreq,gensym("topfreq"),A_FLOAT,0);
  class_addmethod(dentist_class,(t_method)dentist_toothcount,gensym("toothcount"),A_FLOAT,0);
  class_addmethod(dentist_class,(t_method)dentist_interpolate_singles,gensym("interpolate_singles"),A_FLOAT, 0);
  class_addmethod(dentist_class,(t_method)dentist_scramble,gensym("scramble"),0);
//  class_addmethod(dentist_class,(t_method)dentist_activate_bins,gensym("activate_bins"),A_FLOAT,0);
  class_addmethod(dentist_class, (t_method)dentist_overlap, gensym("overlap"), A_DEFFLOAT,0);
  class_addmethod(dentist_class, (t_method)dentist_winfac, gensym("winfac"), A_DEFFLOAT,0);
  class_addmethod(dentist_class, (t_method)dentist_fftinfo, gensym("fftinfo"), 0);

  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void dentist_interpolate_singles(t_dentist *x, t_floatarg f)
{
    x->interpolate_singles = (short)f;
//    post("singles interp: %d",x->interpolate_singles);
}

void dentist_free(t_dentist *x)
{
/* Pd might be having difficulty freeing its memory */
#if MSP
    dsp_free((t_pxobject *)x);
#endif
    free(x->Wanal);
    free(x->Wsyn);
    free(x->input);
    free(x->Hwin);
    free(x->buffer);
    free(x->channel);
    free(x->output);
    free(x->bitshuffle);
    free(x->trigland);
    free(x->bin_selection);
    free(x->active_bins);
    free(x->last_bin_selection);
    free(x->list_data);

//post("avoiding freeing memory here");
}

void dentist_overlap(t_dentist *x, t_floatarg f)
{
int o = (int)f;
  if(!power_of_two(o)){
    error("%d is not a power of two",o);
    return;
  }
  x->overlap = o;
  dentist_init(x,1);
}

void dentist_winfac(t_dentist *x, t_floatarg f)
{
int w = (int)f;
  if(!power_of_two(w)){
    error("%d is not a power of two",w);
    return;
  }
  x->winfac = w;
  dentist_init(x,2);
}

void dentist_fftinfo( t_dentist *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void dentist_direct_update( t_dentist *x, t_floatarg toggle)
{
  x->direct_update = (short)toggle;
}

void dentist_mute( t_dentist *x, t_floatarg toggle )
{
  x->mute = (short)toggle;
}

void dentist_assist (t_dentist *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:
      sprintf(dst,"(signal) Input ");
      break;
    }
  } else if (msg==2) {
    switch (arg) {
    case 0: sprintf(dst,"(signal) Output"); break;
    case 1: sprintf(dst,"(signal) Interpolation Sync"); break;
    case 2: sprintf(dst,"(list) Current Selected Bins"); break;			
    }
  }
}

void *dentist_new(t_symbol *msg, short argc, t_atom *argv)
{
#if MSP
  t_dentist *x = (t_dentist *)newobject(dentist_class);
  x->list_outlet = listout((t_pxobject *)x);
  dsp_setup((t_pxobject *)x,1);
  outlet_new((t_pxobject *)x, "signal");
  outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
  t_dentist *x = (t_dentist *)pd_new(dentist_class);
  outlet_new(&x->x_obj, gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
  x->list_outlet = outlet_new(&x->x_obj,gensym("list"));
#endif

  // INITIALIZATIONS
	
  x->D = sys_getblksize();
  x->R = sys_getsr(); 
  x->topfreq = atom_getfloatarg(0,argc,argv);
  x->overlap = atom_getfloatarg(1,argc,argv);
  x->winfac = atom_getfloatarg(2,argc,argv);
  if(!power_of_two(x->overlap))
  	x->overlap = 4;
  if(!power_of_two(x->winfac))
  	x->winfac = 1;
  
  dentist_init(x,0);
  return (x);
}

void dentist_topfreq(t_dentist *x, t_floatarg f)
{
float funda = x->funda;
float curfreq;

    if(f < 50 || f > x->R/2.0)
        return;
        
    x->topfreq = f;
    x->max_bin = 1;  
    curfreq = 0;
    while(curfreq < x->topfreq) {
        ++(x->max_bin);
        curfreq += funda ;
    }
}

void dentist_init(t_dentist *x, short initialized)
{
	
	float curfreq;
	int i;
	
    x->N = x->overlap * x->D;
    x->Nw = x->N * x->winfac;	
  limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	
	x->N2 = (x->N)>>1;
    x->Nw2 = (x->Nw)>>1;
    x->inCount = -(x->Nw);
    
	if(!initialized){
		x->sync = 0;
		x->mute = 0;
		x->direct_update = 0;
		if(x->topfreq < 100)
			x->topfreq = 100.0;
		x->Wanal = (float *) calloc(MAX_Nw, sizeof(float));	
		x->Wsyn = (float *) calloc(MAX_Nw, sizeof(float));	
		x->input = (float *) calloc(MAX_Nw, sizeof(float));	
		x->Hwin = (float *) calloc(MAX_Nw, sizeof(float));
		x->buffer = (float *) calloc(MAX_N, sizeof(float));
		x->channel = (float *) calloc(MAX_N+2, sizeof(float));
		x->output = (float *) calloc(MAX_Nw, sizeof(float));
		x->bitshuffle = (int *) calloc(MAX_N * 2, sizeof(int));
		x->trigland = (float *) calloc(MAX_N * 2, sizeof(float));
		x->bin_selection = (short *) calloc( MAX_N2, sizeof(short));
		x->active_bins = (int *) calloc( MAX_N2, sizeof(int));
		x->last_bin_selection = (short *) calloc( MAX_N2, sizeof(short)) ;
		x->list_data = (t_atom *) calloc( MAX_N + 2, sizeof(t_atom));
		x->tooth_count = 0;
		x->interpolate_singles = 1;
		x->ramp_frames = 0;
		dentist_scramble(x);
	} 
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->buffer,0,x->N * sizeof(float));
	memset((char *)x->channel,0,(x->N+2) * sizeof(float));

	
    x->mult = 1. / (float) x->N;
    x->frame_duration = (float) x->D / (float) x->R;
    x->frames_left = 0;
    x->funda = (float) x->R / (float) x->N;
    x->max_bin = 1;  
/*    curfreq = 0;
	
    while(curfreq < x->topfreq) {
        ++(x->max_bin);
        curfreq += x->funda;
    }*/
    if(!x->funda){
    	error("%s: zero sampling rate!",OBJECT_NAME);
    	return;
    }
    x->max_bin = (int) (x->topfreq / x->funda);
    if(x->max_bin < 1)
    	x->max_bin = 1;
    
	init_rdft( x->N, x->bitshuffle, x->trigland);
	makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
    
    for( i = 0; i < x->N2; i++) {
        x->last_bin_selection[i] = x->bin_selection[i];
    }
}

t_int *dentist_perform(t_int *w)
{
  int	i,j;
  float oldfrac,newfrac;
  int  tooth_count;

  t_dentist *x = (t_dentist *) (w[1]);
	
  t_float *in = (t_float *)(w[2]);
  t_float *out = (t_float *)(w[3]);
  t_float *sync_vec = (t_float *)(w[4]);
  t_int n = w[5];

  int frames_left = x->frames_left;
  int ramp_frames = x->ramp_frames;
  short *bin_selection = x->bin_selection;
  short *last_bin_selection = x->last_bin_selection;
  /* dereference struncture  */	
  float *input = x->input;
  float *buffer = x->buffer;
  int inCount = x->inCount;
  int R = x->R;
  int N = x->N;
  int N2 = x->N2;
  int D = x->D;
  int Nw = x->Nw;
  float *Wanal = x->Wanal;
  float *Wsyn = x->Wsyn;
  float *output = x->output;
  float *channel = x->channel;
  float mult = x->mult ;
  int *bitshuffle = x->bitshuffle;
  float *trigland = x->trigland ;
  float sync = x->sync;


  if( x->mute ){
    while( n-- ){
      *out++ = 0.0;
      *sync_vec++ = sync;
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

  fold( input, Wanal, Nw, buffer, N, inCount );	
  rdft( N, 1, buffer, bitshuffle, trigland );
  leanconvert( buffer, channel, N2 );

  if(frames_left > 0 && ramp_frames > 0) {
    // INTERPOLATE ACCORDING TO POSITION IN RAMP
    oldfrac = (float) frames_left / (float) ramp_frames ;
    sync = newfrac = 1.0 - oldfrac;
    for( i = 0; i < N2 ; i++){
      if( (! bin_selection[i]) && (! last_bin_selection[i]) ){
		channel[i * 2]  = 0;
      } else if (bin_selection[i] && last_bin_selection[i]) {
		// channel[i * 2]  *= 1; NO ACTION
      } else if (bin_selection[i]) {
		channel[i * 2]  *= newfrac;
      } else if (last_bin_selection[i]) {
		channel[i * 2]  *= oldfrac;
      }
    }
    --frames_left;
    if( ! frames_left ){
      // Copy current to last
      for( i = 0; i < N2; i++) {
		last_bin_selection[i] = bin_selection[i];
      }
    }
  } else {
    for( i = 0; i < N2 ; i++){
      if( ! bin_selection[ i ] ){
		channel[ i * 2 ]  = 0;
      }
    }
    oldfrac = 0.0;
    sync = 1.0;
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

  /* restore state variables */
  for(i=0; i < n; i++){
  	sync_vec[i] = sync;
  }
  x->inCount = inCount % Nw;
  x->frames_left = frames_left;
  x->sync = sync;
  return (w+6);
}		


void set_switch_bins (t_dentist *x, int i)
{
  if( i < 0 ){
    i = 0;
  }
  if( i > x->N2 ) {
    i = x->N2;
  }
  x->tooth_count = i;
  if( x->direct_update ){
    reset_shuffle(x);
  }
  return;
}
//identical function for Pd
void dentist_bins_pd (t_dentist *x, t_floatarg i)
{
  if( i < 0 ){
    i = 0;
  }
  if( i > x->N2 ) {
    i = x->N2;
  }
  x->tooth_count = (int)i;
  if(x->direct_update){
    reset_shuffle(x);
  }
  return;
}

// experimental, not to be used
void dentist_activate_bins(t_dentist *x, t_floatarg f)
{
    if(f < 0 || f > x->max_bin){
#if PD
        post("* %d bin out of range",(int)f);
#endif        
        return;
    }
    x->tooth_count = (int)f;
}

void dentist_scramble(t_dentist *x)
{
short *last_bin_selection = x->last_bin_selection;
short *bin_selection = x->bin_selection;
int *active_bins = x->active_bins;
int N2 = x->N2;
int i,tmp,b1,b2;
int maxswap = x->max_bin;
int tooth_count = x->tooth_count;

  /* for(i = 0; i < x->N2; i++){
    last_bin_selection[i] = bin_selection[i];
  }*/
  for(i=0; i<N2; i++){
    bin_selection[i] = 0;
    active_bins[i] = i;
  }
  while(maxswap > 0){
    b1 = maxswap;
    b2 = rand_index(maxswap);
    tmp = active_bins[b1];
    active_bins[b1] = active_bins[b2];
    active_bins[b2] = tmp;
    --maxswap;
  }
  for( i = 0; i < x->tooth_count; i++ ) {
    x->bin_selection[active_bins[i]] = 1;
  }
  x->frames_left = x->ramp_frames;
  if(! x->ramp_frames) {
    for(i = 0; i < N2; i++){
      last_bin_selection[i] = bin_selection[i];
    }
  }    
}


void dentist_toothcount(t_dentist *x, t_floatarg newcount)
{
int i;
int nc = (int) newcount;
int max = x->max_bin;
int tooth_count = x->tooth_count;
int newbin;
int oldbin;

if(nc < 0 || nc > x->N2){
#if PD
    post("* %d out of range",nc);
#endif
#if MSP
    error("%d out of range",nc);
#endif
    return;
}
  /* for(i = 0; i < x->N2; i++){
    x->last_bin_selection[i] = x->bin_selection[i];
  }*/
  if(nc < x->tooth_count){
    for(i = nc; i < tooth_count; i++){
        x->bin_selection[x->active_bins[i]] = 0;
    }
  } else {
    for(i = tooth_count; i < nc; i++){
        x->bin_selection[x->active_bins[i]] = 1;
    }
  }
  // if immediate reset
  if(x->interpolate_singles){
//  post("setting frames left");
    x->frames_left = x->ramp_frames;
  }
  if(! x->ramp_frames) {
    for(i = 0; i < x->N2; i++){
      x->last_bin_selection[i] = x->bin_selection[i];
    }
  }
  x->tooth_count = nc;
}


void reset_shuffle (t_dentist *x)
{
  int i;
  int temp, p1, p2;
  int max;

  max = x->max_bin;
  for(i = 0; i < x->N2; i++){
    x->last_bin_selection[i] = x->bin_selection[i];
    x->bin_selection[i] = 0;
  }
  for(i = 0; i < x->max_bin; i++) {
    x->active_bins[i] = rand_index(max);
    x->bin_selection[x->active_bins[i]] = 1;
  }
  x->frames_left = x->ramp_frames;
  if(! x->ramp_frames) { // Ramp Off - Immediately set last to current
    for( i = 0; i < x->N2; i++ ){
      x->last_bin_selection[ i ] = x->bin_selection[ i ];
    }
  }
}

int rand_index(int max) {
  return (rand() % max);
}

void dentist_setstate (t_dentist *x, t_symbol *msg, short argc, t_atom *argv) {
  short i;
  int selex;
  
  short *last_bin_selection = x->last_bin_selection;
  short *bin_selection = x->bin_selection;
  int *active_bins = x->active_bins;
  x->tooth_count = argc;
  
  for(i = 0; i < x->N2; i++){
    last_bin_selection[i] = bin_selection[i]; // needed here
    bin_selection[i] = 0;
  }

  for (i=0; i < argc; i++) {
    selex = atom_getfloatarg(i,argc,argv);
    if (selex < x->N2 && selex >= 0 ) {
      active_bins[i] = selex;
      bin_selection[selex] = 1;
    } else {
      post ("%d out of range bin",selex);
    }		
  }


  x->frames_left = x->ramp_frames;
  if(! x->ramp_frames) { // Ramp Off - Immediately set last to current
    for(i = 0; i < x->N2; i++){
      last_bin_selection[i] = bin_selection[i];
    }
  }

  return;
}
void dentist_ramptime (t_dentist *x, t_floatarg ramp_ms) {
	
  if(ramp_ms <= 0){
    x->ramp_frames = 0;
    return;
  }
  
  x->frames_left = x->ramp_frames = (int)(ramp_ms * .001 / x->frame_duration);
  return;
}
// REPORT CURRENT SHUFFLE STATUS
void dentist_showstate (t_dentist *x) {

  t_atom *list_data = x->list_data;

  short i, count;
  float data;

  count = 0;
  for(i = 0; i < x->tooth_count; i++ ) {
    data = x->active_bins[i];
#if MSP
      SETLONG(list_data+count,x->active_bins[i]);
 #endif
 #if PD
        SETFLOAT(list_data+count,data);
 #endif
      ++count;
  }	
  outlet_list(x->list_outlet,0,x->tooth_count,list_data);

  return;
}
void dentist_dsp(t_dentist *x, t_signal **sp, short *count)
{
  long i;
  if(x->R != sp[0]->s_sr || x->D != sp[0]->s_n){
    x->R = sp[0]->s_sr;
    x->D = sp[0]->s_n;
    dentist_init(x,1);
  }
  dsp_add(dentist_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

