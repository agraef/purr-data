#include "MSPd.h"

#include "fftease.h"

#if MSP
void *cavoc_class;
#endif 

#if PD
static t_class *cavoc_class;
#endif

#define OBJECT_NAME "cavoc~"

typedef struct _cavoc
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
  int	in_count;
  float *Wanal;	
  float *Wsyn;	
  float *input;	
  float *Hwin;
  float *buffer;
  float *channel;
  float *output;

  float frame_duration;
  int max_bin;

  float fundamental;
  float *last_frame;
  short left;
  short right;
  short center;
  short *rule;
  float density;
  float start_breakpoint;
  int hold_frames;
  int frames_left;
  int set_count;
  // FFT
  float *c_lastphase_out;
  float c_fundamental;
  float c_factor_out;
  float mult; 
  float *trigland;
  int *bitshuffle;
  //
  void *list_outlet;
  t_atom *list_data;
  short mute;
  int overlap;
  int winfac;
  short external_trigger;
  float hold_time;
} t_cavoc;

void *cavoc_new(t_symbol *msg, short argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *cavoc_perform(t_int *w);
void cavoc_dsp(t_cavoc *x, t_signal **sp, short *count);
void cavoc_assist(t_cavoc *x, void *b, long m, long a, char *s);
void cavoc_free( t_cavoc *x );
int cavoc_apply_rule( short left, short right, short center, short *rule);
float cavoc_randf(float min, float max);
void cavoc_rule (t_cavoc *x, t_symbol *msg, short argc, t_atom *argv);
void cavoc_density (t_cavoc *x, t_floatarg density);
void cavoc_hold_time (t_cavoc *x, t_floatarg hold_time);
void cavoc_retune (t_cavoc *x, t_floatarg min, t_floatarg max);
void cavoc_mute (t_cavoc *x, t_floatarg toggle);
void cavoc_external_trigger(t_cavoc *x, t_floatarg toggle);
void cavoc_init(t_cavoc *x,short initialized);
void cavoc_overlap(t_cavoc *x, t_floatarg f);
void cavoc_winfac(t_cavoc *x, t_floatarg f);
void cavoc_fftinfo(t_cavoc *x);

#if MSP
void main(void)
{
  setup((t_messlist **)&cavoc_class, (method)cavoc_new, (method)cavoc_free, 
  	(short)sizeof(t_cavoc), 0, A_GIMME, 0);
  addmess((method)cavoc_dsp, "dsp", A_CANT, 0);
  addmess((method)cavoc_assist,"assist",A_CANT,0);
  addmess((method)cavoc_rule,"rule",A_GIMME,0);
  addmess((method)cavoc_density,"density",A_FLOAT,0);
  addmess((method)cavoc_hold_time,"hold_time",A_FLOAT,0);
  addmess((method)cavoc_mute,"mute",A_FLOAT,0);
  addmess((method)cavoc_external_trigger,"external_trigger",A_FLOAT,0);
  addmess((method)cavoc_retune,"retune",A_FLOAT,A_FLOAT,0);
  addmess((method)cavoc_overlap,"overlap",A_FLOAT,0);
  addmess((method)cavoc_winfac,"winfac",A_FLOAT,0);
  addmess((method)cavoc_fftinfo,"fftinfo",0);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void cavoc_tilde_setup(void){
  cavoc_class = class_new(gensym("cavoc~"), (t_newmethod)cavoc_new, 
      (t_method)cavoc_free ,sizeof(t_cavoc), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(cavoc_class, t_cavoc, x_f);
  class_addmethod(cavoc_class,(t_method)cavoc_dsp,gensym("dsp"),0);
  class_addmethod(cavoc_class,(t_method)cavoc_mute,gensym("mute"),A_FLOAT,0);
  class_addmethod(cavoc_class,(t_method)cavoc_overlap,gensym("overlap"),A_FLOAT,0);
  class_addmethod(cavoc_class,(t_method)cavoc_winfac,gensym("winfac"),A_FLOAT,0);
  class_addmethod(cavoc_class,(t_method)cavoc_rule,gensym("rule"),A_GIMME,0);
	class_addmethod(cavoc_class,(t_method)cavoc_density,gensym("density"),A_FLOAT,0);
	class_addmethod(cavoc_class,(t_method)cavoc_hold_time,gensym("hold_time"),A_FLOAT,0);
	class_addmethod(cavoc_class,(t_method)cavoc_external_trigger,gensym("external_trigger"),A_FLOAT,0);
	class_addmethod(cavoc_class,(t_method)cavoc_retune,gensym("retune"),A_FLOAT,A_FLOAT,0);
  class_addmethod(cavoc_class,(t_method)cavoc_winfac,gensym("winfac"),A_FLOAT,0);
  class_addmethod(cavoc_class,(t_method)cavoc_overlap,gensym("overlap"),A_FLOAT,0);
  class_addmethod(cavoc_class,(t_method)cavoc_fftinfo,gensym("fftinfo"),0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void cavoc_fftinfo( t_cavoc *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void cavoc_overlap(t_cavoc *x, t_floatarg f)
{
  if(!power_of_two(f)){
    error("%f is not a power of two",f);
    return;
  }
	x->overlap = (int)f;
	cavoc_init(x,1);
}

void cavoc_winfac(t_cavoc *x, t_floatarg f)
{
	  if(!power_of_two(f)){
	    error("%f is not a power of two",f);
	    return;
	  }
	x->winfac = (int)f;
	cavoc_init(x,1);
}

void cavoc_external_trigger(t_cavoc *x, t_floatarg toggle)
{
	x->external_trigger = toggle;
}

void cavoc_mute (t_cavoc *x, t_floatarg toggle)
{
	x->mute = toggle;
}

void cavoc_retune(t_cavoc *x, t_floatarg min, t_floatarg max)
{
  int i;

  if( max <= 0 || min <= 0 || min > max ){
    error("bad values for min and max multipliers");
    return;
  }
  if( min < .1 )
    min = 0.1;
  if( max > 2.0 )
    max = 2.0;
  for( i = 0; i < x->N2 + 1; i++ ){
    x->channel[ i * 2 + 1 ] = x->c_fundamental * (float) (i / 2) * cavoc_randf(min, max);
  }

}

void cavoc_density(t_cavoc *x, t_floatarg density)
{
  int i;
  if( density < 0.0001 ){
    density = .0001;
  } else if( density > .9999 ){
    density = 1.0;
  }
  x->density = density;
  x->start_breakpoint = 1.0 - x->density;
  for( i = 0; i < x->N2 + 1; i++ ){
    if( cavoc_randf(0.0, 1.0) > x->start_breakpoint ){
      x->channel[ i * 2 ] = 1;
      ++(x->set_count);
    } else {
      x->channel[ i * 2 ] = 0;
    }
  }
  for( i = 0; i < x->N+2; i++ ){
    x->last_frame[i] = x->channel[i];
  }
}

void cavoc_hold_time(t_cavoc *x, t_floatarg f)
{

	if(f <= 0){
		error("negative or zero hold time.");
		return;
	}
	x->hold_time = f;
  x->hold_frames = (int) ((x->hold_time/1000.0) / x->frame_duration);
  if( x->hold_frames < 1 )
    x->hold_frames = 1;
  x->frames_left = x->hold_frames;

}

void cavoc_rule (t_cavoc *x, t_symbol *msg, short argc, t_atom *argv)
{
  int i;
  short *rule = x->rule;
  if( argc != 8 ){
    error("the rule must be size 8");
    return;
  }
  
  for( i = 0; i < 8; i++ ){
    rule[i] = (short) atom_getfloatarg(i, argc, argv);
//    post("%d",rule[i]);
  }
}

void cavoc_free( t_cavoc *x ){
#if MSP
  dsp_free((t_pxobject *) x);
#endif
  free(x->trigland);
  free(x->bitshuffle);
  free(x->Wanal);
  free(x->Wsyn);
  free(x->input);
  free(x->Hwin);
  free(x->buffer);
  free(x->channel);
  free(x->last_frame);
  free(x->output);
  free(x->c_lastphase_out);
  free(x->rule);  
}

void cavoc_assist (t_cavoc *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
   	 case 0: sprintf(dst,"(signal) Trigger");break;
    }
  } else if (msg==2) {
    switch (arg) {
  	  case 0:sprintf(dst,"(signal) Output"); break;
    }
  }
}

void *cavoc_new(t_symbol *msg, short argc, t_atom *argv)
{
#if MSP
  t_cavoc *x = (t_cavoc *)newobject(cavoc_class);
  dsp_setup((t_pxobject *)x,1);
  outlet_new((t_pxobject *)x, "signal");
 #endif

#if PD
	t_cavoc *x = (t_cavoc *)pd_new(cavoc_class);
  outlet_new(&x->x_obj, gensym("signal"));
#endif

  x->D = sys_getblksize();
  x->R = sys_getsr();

  x->density = atom_getfloatarg(0,argc,argv);
  x->hold_time = atom_getintarg(1,argc,argv);
  x->overlap = atom_getintarg(2,argc,argv);
  x->winfac = atom_getintarg(3,argc,argv);

  
	cavoc_init(x,0);
  return (x);
}

void cavoc_init(t_cavoc *x,short initialized)
{
int i;

  if(!x->D)
    x->D = 256;
  if(!x->R)
    x->R = 44100;
  if(!power_of_two(x->overlap))
    x->overlap = 4;
  if(!power_of_two(x->winfac))
    x->winfac = 1;
  x->N = x->D * x->overlap;
  x->Nw = x->N * x->winfac;
limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
  x->mult = 1. / (float) x->N;
  x->N2 = (x->N)>>1;
  x->Nw2 = (x->Nw)>>1;
  x->in_count = -(x->Nw);
  x->c_fundamental =  (float) x->R/(float)((x->N2)<<1);
  x->frame_duration = (float)x->D/(float) x->R;
  if(x->hold_time <= 100) /* in milliseconds */
  	x->hold_time = 100;


  cavoc_hold_time(x, x->hold_time);  	

	if(!initialized){
	  
	  srand(time(0));
	  x->mute = 0;
	  x->set_count = 0;
	  x->external_trigger = 0;
	  if( x->density < 0.0 ){
	    x->density = 0;
	  } else if( x->density > 1.0 ){
	    x->density = 1.0;
	  }
	  x->start_breakpoint = 1.0 - x->density;
	  
	  x->Wanal = (float *) calloc( MAX_Nw, sizeof(float) );	
	  x->Wsyn = (float *) calloc( MAX_Nw, sizeof(float) );	
	  x->input = (float *) calloc( MAX_Nw, sizeof(float) );	
	  x->Hwin = (float *) calloc( MAX_Nw, sizeof(float) );
	  x->buffer = (float *) calloc( MAX_N, sizeof(float) );
	  x->channel = (float *) calloc( MAX_N+2, sizeof(float) );
	  x->last_frame = (float *) calloc(MAX_N+2, sizeof(float));
	  x->output = (float *) calloc( MAX_Nw, sizeof(float) );
	  x->bitshuffle = (int *) calloc( MAX_N * 2, sizeof( int ) );
	  x->trigland = (float *) calloc( MAX_N * 2, sizeof( float ) );
	  x->c_lastphase_out = (float *) calloc( MAX_N2+1, sizeof(float) );
	  
	  x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
	  x->rule = (short *)  calloc(8, sizeof(short));

	  x->rule[2] = x->rule[3] = x->rule[5] = x->rule[6] = 1;
	  x->rule[0] = x->rule[1] = x->rule[4] = x->rule[7] = 0;

	} 
		memset((char *)x->input,0,x->Nw * sizeof(float));
		memset((char *)x->output,0,x->Nw * sizeof(float));
		memset((char *)x->buffer,0,x->N * sizeof(float));
		memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));
		memset((char *)x->last_frame,0,(x->N+2) * sizeof(float));

  init_rdft( x->N, x->bitshuffle, x->trigland);
  makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);

  for(i = 0; i < x->N2 + 1; i++){
    if(cavoc_randf(0.0, 1.0) > x->start_breakpoint){
      x->channel[ i * 2 ] = 1;
      ++(x->set_count);
    } else {
      x->channel[i * 2] = 0;
    }
    x->channel[i * 2 + 1] = x->c_fundamental * (float) (i / 2) * cavoc_randf(.9,1.1);
  }

//  post("turned on %d of a possible %d bins", x->set_count, x->N2+1 );
  
  for( i = 0; i < x->N+2; i++ ){
    x->last_frame[i] = x->channel[i];
  }
//  post("cavoc~ FFT size: %d",x->N);
}

t_int *cavoc_perform(t_int *w)
{
  int	i,j;
  float oldfrac,newfrac;
  t_cavoc *x = (t_cavoc *)(w[1]);
  float *trigger_vec = (t_float *)(w[2]);
  float *out = (t_float *)(w[3]);
  t_int n = w[4];

  int frames_left = x->frames_left;
  float *input = x->input;
  float *buffer = x->buffer;
  int in_count = x->in_count;
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
  int hold_frames = x->hold_frames;
  short *rule = x->rule;
  short left = x->left;
  short right = x->right;
  short center = x->center;
  float *last_frame = x->last_frame;
  float *c_lastphase_out = x->c_lastphase_out;
  float c_fundamental = x->c_fundamental;
  float c_factor_out = x->c_factor_out;
  short external_trigger = x->external_trigger;
  short new_event = 0;
  
  in_count += D;

  if( x->mute ){
    while( n-- ){
      *out++ = 0.0;
    }
    return (w+5);
  }
  if(external_trigger){// only accurate to within a vector because of FFT
  	for(i=0;i<n;i++){
  		if(trigger_vec[i]){
  			new_event = 1;
  			break;
  		}
  	}
  } else if(!--frames_left){
    frames_left = hold_frames;
  	new_event = 1;
  }
  
  if(new_event){
    for( i = 2; i < N; i+=2 ){
      left = last_frame[ i - 2];
      center = last_frame[i] ;
      right = last_frame[i+2];
      channel[i] = cavoc_apply_rule(left, right, center, rule );
    }
    /* boundary cases */
    center = last_frame[0];
    right = last_frame[2];
    left = last_frame[N];
    channel[0] = cavoc_apply_rule(left, right, center, rule );

    center = last_frame[N];
    right = last_frame[0];
    left = last_frame[N - 2];
    channel[N] = cavoc_apply_rule(left, right, center, rule );


    /* save composite frame for next time */
    for( i = 0; i < N+1; i++ ){
      last_frame[i] = channel[i];
    }
  }
  unconvert( channel, buffer, N2, c_lastphase_out, c_fundamental, c_factor_out  );
  rdft( N, -1, buffer, bitshuffle, trigland );
  overlapadd( buffer, N, Wsyn, output, Nw, in_count);

  for ( j = 0; j < D; j++ )
    *out++ = output[j] * mult;

  for ( j = 0; j < Nw - D; j++ )
    output[j] = output[j+D];
			
  for ( j = Nw - D; j < Nw; j++ )
    output[j] = 0.;	

  /* restore state variables */
  x->in_count = in_count % Nw;
  x->frames_left = frames_left;
  return (w+5);
}		

int cavoc_apply_rule(short left, short right, short center, short *rule){

  if( ! center ){
    if( ! left && ! right){
      return  rule[0];
    } else if ( ! left && right ){ 
      return rule[1];
    } else if ( left && ! right ) {
      return rule[2];
    } else if (left && right) {
      return rule[3];
    }
  } else {
    if( ! left && ! right){
      return rule[4];
    } else if ( ! left && right ){ 
      return rule[5];
    } else if ( left && ! right ) {
      return rule[6];
    } else if (left && right) {
      return rule[7];
    }
  }
  return 0;// never happens
}

float cavoc_randf(float min, float max)
{
	float randv;
	
	randv = (float) (rand() % 32768) / 32768.0 ;
	
	return (min + ((max-min) * randv))  ;
}
void cavoc_dsp(t_cavoc *x, t_signal **sp, short *count)
{
  if(sp[0]->s_n != x->D || x->R != sp[0]->s_sr){
    x->D = sp[0]->s_n;
    x->R = sp[0]->s_sr;
    cavoc_init(x,1);
  }

  dsp_add(cavoc_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec,  sp[0]->s_n);
}

