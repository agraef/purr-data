#include "MSPd.h"

#include "fftease.h"

#if MSP
void *cavoc27_class;
#endif 

#if PD
static t_class *cavoc27_class;
#endif

#define OBJECT_NAME "cavoc27~"

/* NOTE THIS IS A MORE COMPLEX CA WITH 3 DIFFERENT STATES  */

typedef struct _cavoc27
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
  float *ichannel; //for interpolation
  float *tmpchannel; // for spectrum capture
  float *output;
  float frame_duration;
  int max_bin;

  // for CAVOC2

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
  short interpolate_flag;
  short capture_flag;
  short capture_lock;
  // FFT
  // for convert

  float *c_lastphase_in;
  float *c_lastphase_out;
  float c_fundamental;
  float c_factor_in;
  float c_factor_out;
  float mult; 
  float *trigland;
  int *bitshuffle;
  //
  void *list_outlet;
  t_atom *list_data;
  short mute;
  int winfac;//window factor
  int overlap;//overlap factor
  int vs;//vector size
  float hold_time;//hold time in seconds
} t_cavoc27;

void *cavoc27_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *cavoc27_perform(t_int *w);
void cavoc27_dsp(t_cavoc27 *x, t_signal **sp, short *count);
void cavoc27_assist(t_cavoc27 *x, void *b, long m, long a, char *s);
void cavoc27_free( t_cavoc27 *x);
int cavoc27_apply_rule( short left, short right, short center, short *rule);
float cavoc27_randf(float min, float max);
void cavoc27_rule (t_cavoc27 *x, t_symbol *msg, short argc, t_atom *argv);
void cavoc27_density (t_cavoc27 *x, t_floatarg density);
void cavoc27_hold_time (t_cavoc27 *x, t_floatarg hold_time);
void cavoc27_interpolate (t_cavoc27 *x, t_floatarg interpolate);
void cavoc27_capture_spectrum (t_cavoc27 *x, t_floatarg flag );
void cavoc27_capture_lock (t_cavoc27 *x, t_floatarg toggle );
void cavoc27_retune (t_cavoc27 *x, t_floatarg min, t_floatarg max);
void cavoc27_mute (t_cavoc27 *x, t_floatarg toggle);
void cavoc27_init(t_cavoc27 *x,short initialized);
void cavoc27_rand_set_spectrum(t_cavoc27 *x);
void cavoc27_rand_set_rule(t_cavoc27 *x);
void cavoc27_fftinfo(t_cavoc27 *x);
void cavoc27_overlap(t_cavoc27 *x, t_floatarg f);
void cavoc27_winfac(t_cavoc27 *x, t_floatarg f);

#if MSP
void main(void)
{
  setup((t_messlist **)&cavoc27_class, (method)cavoc27_new, (method)cavoc27_free, 
  (short)sizeof(t_cavoc27), 0, A_GIMME, 0);
  addmess((method)cavoc27_dsp, "dsp", A_CANT, 0);
  addmess((method)cavoc27_assist,"assist",A_CANT,0);
  addmess((method)cavoc27_rule,"rule",A_GIMME,0);
  addmess((method)cavoc27_density,"density",A_FLOAT,0);
  addmess((method)cavoc27_hold_time,"hold_time",A_FLOAT,0);
  addmess((method)cavoc27_interpolate,"interpolate",A_FLOAT,0);
  addmess((method)cavoc27_overlap,"overlap",A_FLOAT,0);
  addmess((method)cavoc27_winfac,"winfac",A_FLOAT,0);
  addmess((method)cavoc27_fftinfo,"fftinfo",0);
  addmess((method)cavoc27_retune,"retune",A_FLOAT,A_FLOAT,0);
  addmess((method)cavoc27_capture_spectrum,"capture_spectrum",A_FLOAT,0);
  addmess((method)cavoc27_mute,"mute",A_FLOAT,0);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif
#if PD
/* Pd Initialization */
void cavoc27_tilde_setup(void)
{
  cavoc27_class = class_new(gensym("cavoc27~"), (t_newmethod)cavoc27_new, 
			 (t_method)cavoc27_free ,sizeof(t_cavoc27), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(cavoc27_class, t_cavoc27, x_f);
  class_addmethod(cavoc27_class, (t_method)cavoc27_dsp, gensym("dsp"), 0);
  class_addmethod(cavoc27_class, (t_method)cavoc27_mute, gensym("mute"), A_FLOAT,0);
  class_addmethod(cavoc27_class, (t_method)cavoc27_rule, gensym("rule"), A_GIMME,0);
  class_addmethod(cavoc27_class, (t_method)cavoc27_density, gensym("density"), A_DEFFLOAT,0);
  class_addmethod(cavoc27_class, (t_method)cavoc27_hold_time, gensym("hold_time"), A_DEFFLOAT,0);
  class_addmethod(cavoc27_class, (t_method)cavoc27_interpolate, gensym("interpolate"), A_DEFFLOAT, 0);
  class_addmethod(cavoc27_class, (t_method)cavoc27_capture_spectrum, gensym("capture_spectrum"), A_FLOAT, 0);
  class_addmethod(cavoc27_class, (t_method)cavoc27_retune, gensym("retune"), A_DEFFLOAT,A_DEFFLOAT,0);
  class_addmethod(cavoc27_class, (t_method)cavoc27_overlap, gensym("overlap"), A_FLOAT, 0);
  class_addmethod(cavoc27_class, (t_method)cavoc27_winfac, gensym("winfac"), A_FLOAT, 0);
  class_addmethod(cavoc27_class, (t_method)cavoc27_fftinfo, gensym("fftinfo"), 0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void cavoc27_rand_set_rule(t_cavoc27 *x)
{
int i;
float rval;
  for( i = 0; i < 27; i++ ){
    rval = cavoc27_randf(0.0,1.0);
    if( rval < .333 )
      x->rule[i] = 0;
    else if(rval < .666 )
      x->rule[i] = 1;
    else x->rule[i] = 2;
  }
}

void cavoc27_retune(t_cavoc27 *x, t_floatarg min, t_floatarg max)
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
    x->channel[ i * 2 + 1 ] = x->c_fundamental * (float) (i / 2) * cavoc27_randf(min, max);
  }

}

void cavoc27_density(t_cavoc27 *x, t_floatarg density)
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
    if( cavoc27_randf(0.0, 1.0) > x->start_breakpoint ){
      if( cavoc27_randf(0.0,1.0) > 0.5 ){
	x->channel[ i * 2 ] = 1;
      }
      else {
	x->channel[ i * 2 ] = 2;
      }
      ++(x->set_count);
    } else {
      x->channel[ i * 2 ] = 0;
    }
  }
  for( i = 0; i < x->N+2; i++ ){
    x->last_frame[i] = x->channel[i];
  }
}

void cavoc27_mute (t_cavoc27 *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void cavoc27_hold_time(t_cavoc27 *x, t_floatarg hold_time)
{

  if(hold_time <= 0){
  	post("illegal hold time %f",hold_time);
  	return;
  }
  x->hold_time = hold_time;
  if(! x->frame_duration){
	error("%s: zero frame duration",OBJECT_NAME);
	x->frame_duration = .15;
  }
  x->hold_frames = (int) ( (hold_time/1000.0) / x->frame_duration);
  if( x->hold_frames < 1 )
    x->hold_frames = 1;
  //  x->frames_left = x->hold_frames;

}

void cavoc27_interpolate(t_cavoc27 *x, t_floatarg flag)
{
  x->interpolate_flag = (short) flag;
}

void cavoc27_capture_spectrum(t_cavoc27 *x, t_floatarg flag )
{
// now identical to capture_lock
  x->capture_lock = (short)flag; 
}

void cavoc27_capture_lock(t_cavoc27 *x, t_floatarg flag )
{
  x->capture_lock = (short)flag; 
}

void cavoc27_rule (t_cavoc27 *x, t_symbol *msg, short argc, t_atom *argv)
{
  int i;
  short *rule = x->rule;
  if( argc != 27 ){
    error("the rule must be size 18");
    return;
  }
  
  for( i = 0; i < 27; i++ ){
    rule[i] = (short) atom_getfloatarg( i, argc, argv);
  }
}

void cavoc27_free( t_cavoc27 *x ){
#if MSP
  dsp_free( (t_pxobject *) x);
#endif
  free(x->trigland);
  free(x->bitshuffle);
  free(x->Wanal);
  free(x->Wsyn);
  free(x->input);
  free(x->Hwin);
  free(x->buffer);
  free(x->channel);
  free(x->ichannel);
  free(x->tmpchannel);
  free(x->last_frame);
  free(x->output);
  free(x->c_lastphase_out);
  free(x->rule);  
}

void cavoc27_assist (t_cavoc27 *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:
      sprintf(dst,"unused(signal)");
      break;
    }
  } else if (msg==2) {
    switch (arg) {
    case 0:
      sprintf(dst,"output(signal)"); break;
    }
  }
}

void *cavoc27_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_cavoc27 *x = (t_cavoc27 *)newobject(cavoc27_class);
  dsp_setup((t_pxobject *)x,1);
  outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
  int i;
  t_cavoc27 *x = (t_cavoc27 *)pd_new(cavoc27_class);
  outlet_new(&x->x_obj, gensym("signal"));
#endif

  x->overlap = 0;
  x->winfac=0;
  x->hold_time=0;
  x->density = 0;
  
  x->density = atom_getfloatarg(0,argc,argv);
  x->hold_time = atom_getfloatarg(1,argc,argv) * .001;// convert to seconds
  x->overlap = atom_getfloatarg(2,argc,argv);
  x->winfac = atom_getfloatarg(3,argc,argv);

  if(x->density <= 0)
  	x->density = .0001;
  if(x->density >= 1)
  	x->density = .9999;
  x->start_breakpoint = 1.0 - x->density;
  if(!x->hold_time)
  	x->hold_time = 0.15;
    	
  x->vs = sys_getblksize();
  x->R = sys_getsr();

  cavoc27_init(x,0);
  return (x);
}

void cavoc27_overlap(t_cavoc27 *x, t_floatarg f)
{
int i = (int) f;
  if(!power_of_two(i)){
    post("%f is not a power of two",f);
    return;
  }
	x->overlap = i;
	cavoc27_init(x,1);
}

void cavoc27_winfac(t_cavoc27 *x, t_floatarg f)
{
int i = (int)f;

  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = i;
	cavoc27_init(x,2);
}

void cavoc27_fftinfo(t_cavoc27 *x)
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void cavoc27_init(t_cavoc27 *x,short initialized)
{
int i;

  if(!power_of_two(x->overlap))
    x->overlap = 4;
  if(!power_of_two(x->winfac))
    x->winfac = 1;
//post("init with o %d and wf %d",x->overlap,x->winfac);
  x->D = x->vs;
  x->N = x->D * x->overlap;
  x->Nw = x->N * x->winfac;
  limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);	


  x->N2 = (x->N)>>1;
  x->Nw2 = (x->Nw)>>1;
  x->inCount = -(x->Nw);  
  x->mult = 1. / (float) x->N;
  x->c_fundamental =  (float) x->R/( (x->N2)<<1 );
  x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
  x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
  x->frame_duration = (float)x->D/(float) x->R;
  x->hold_frames = (int) (x->hold_time/x->frame_duration);
  x->frames_left = x->hold_frames;
//  x->frames_left = 0;
  x->set_count = 0;

	if(!initialized){
	  srand(time(0));
	  x->interpolate_flag = 0;
	  x->capture_lock = 0;
	  
	  x->mute = 0;
	  x->Wanal = (float *) calloc(MAX_Nw, sizeof(float));	
	  x->Wsyn = (float *) calloc(MAX_Nw, sizeof(float));	
	  x->input = (float *) calloc(MAX_Nw, sizeof(float));	
	  x->Hwin = (float *) calloc(MAX_Nw, sizeof(float));
	  x->buffer = (float *) calloc(MAX_N, sizeof(float));
	  x->channel = (float *) calloc(MAX_N+2, sizeof(float));
	  x->ichannel = (float *) calloc(MAX_N+2, sizeof(float));
	  x->tmpchannel = (float *) calloc(MAX_N+2, sizeof(float));
	  x->rule = (short *) calloc(27, sizeof(short));
	  x->last_frame = (float *) calloc(MAX_N+2, sizeof(float));
	  x->output = (float *) calloc(MAX_Nw, sizeof(float));
	  x->bitshuffle = (int *) calloc(MAX_N * 2, sizeof(int));
	  x->trigland = (float *) calloc(MAX_N * 2, sizeof(float));
	  x->c_lastphase_in = (float *) calloc(MAX_N2+1, sizeof(float));
	  x->c_lastphase_out = (float *) calloc(MAX_N2+1, sizeof(float));

  
	} 
		memset((char *)x->input,0,x->Nw * sizeof(float));
		memset((char *)x->output,0,x->Nw * sizeof(float));
		memset((char *)x->buffer,0,x->N * sizeof(float));
		memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
		memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));


  cavoc27_rand_set_rule(x); 
  init_rdft( x->N, x->bitshuffle, x->trigland);
  makehanning( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
  cavoc27_rand_set_spectrum(x);
  for( i = 0; i < x->N+2; i++ ){
    x->last_frame[i] = x->channel[i];
  }
}

void cavoc27_rand_set_spectrum(t_cavoc27 *x)
{
int i;
float rval;

  //set spectrum
  for( i = 0; i < x->N2 + 1; i++ ){
    if( cavoc27_randf(0.0, 1.0) > x->start_breakpoint){
      rval = cavoc27_randf(0.0, 1.0);
      if( rval < 0.5 ){
				x->channel[ i * 2 ] = 1;
			}
      else {
        x->channel[ i * 2 ] = 2;
      }
      ++(x->set_count);
    } else {
      x->channel[ i * 2 ] = 0;
    }
    x->channel[ i * 2 + 1 ] = x->c_fundamental * (float) (i / 2) * cavoc27_randf(.9,1.1);
  }
}

t_int *cavoc27_perform(t_int *w)
{
  int	i,j;
  float m1,m2;
  ////////////
  t_cavoc27 *x = (t_cavoc27 *) (w[1]);
	
  float *in = (t_float *)(w[2]);
  float *out = (t_float *)(w[3]);
  t_int n = w[4];

  int frames_left = x->frames_left;
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
  float *tmpchannel = x->tmpchannel;
  float *ichannel = x->ichannel;
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
  float *c_lastphase_in = x->c_lastphase_in;
  float c_fundamental = x->c_fundamental;
  float c_factor_out = x->c_factor_out;
  float c_factor_in = x->c_factor_in;
  short interpolate_flag = x->interpolate_flag;


  if( x->mute ){
    while( n-- ){
      *out++ = 0.0;
    }
    return (w+5);
  }

  x->inCount += D;  
  
  if( x->capture_flag || x->capture_lock ) {


    for ( j = 0 ; j < (Nw - D) ; j++ ){
      input[j] = input[j+D];
    }
    for ( j = (Nw-D), i = 0 ; j < Nw; j++, i++ ) {
      input[j] = *in++;
    }

    fold( input, Wanal, Nw, buffer, N, x->inCount );   
    rdft( N, 1, buffer, bitshuffle, trigland );
    convert( buffer, tmpchannel, N2, c_lastphase_in, c_fundamental, c_factor_in );

    // ONLY COPY PHASES
    for( i = 1; i < N+2; i += 2 ){
      channel[i] = tmpchannel[i];
      // last_frame[i] = channel[i] = tmpchannel[i];
    }

    if( x->capture_flag ){
   		 --(x->capture_flag);
   	}
    // on final frame - swap in channel (just phases though )

    
  }
  if( --frames_left <= 0){
    /* save composite frame for next time  (only amps) */
/*    for( i = 0; i < N+1; i++ ){
      last_frame[i] = channel[i];
    }*/

    for( i = 0; i < N+1; i += 2 ){
      last_frame[i] = channel[i];
    }

    frames_left = hold_frames;
    for( i = 2; i < N; i+=2 ){
      left = last_frame[ i - 2 ];
      center = last_frame[i] ;
      right = last_frame[i+2];
      channel[i] = cavoc27_apply_rule(left, right, center, rule );
    }
    /* boundary cases */
    center = last_frame[0];
    right = last_frame[2];
    left = last_frame[N];
    channel[0] = cavoc27_apply_rule(left, right, center, rule );

    center = last_frame[N];
    right = last_frame[0];
    left = last_frame[N - 2];
    channel[N] = cavoc27_apply_rule(left, right, center, rule );
  }
  if( interpolate_flag ){
    m1 = (float) frames_left / (float) hold_frames ;
    m2 = 1.0 - m1;
    for( i = 0; i <N+2; i += 2 ){
      ichannel[i] = m1 * last_frame[i] + m2 * channel[i];
      ichannel[i+1] = channel[i+1];
    }

    unconvert( ichannel, buffer, N2, c_lastphase_out, c_fundamental, c_factor_out  );
  } else {
    unconvert( channel, buffer, N2, c_lastphase_out, c_fundamental, c_factor_out  );
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
  x->inCount = inCount;
  x->frames_left = frames_left;
  return (w+5);
}		

int cavoc27_apply_rule( short left, short right, short center, short *rule){

  if( left == 0 && center == 0 && right == 0 )
    return rule[0];
  if( left == 1 && center == 0 && right == 1 )
    return rule[1];
  if( left == 1 && center == 0 && right == 0 )
    return rule[2];
  if( left == 0 && center == 0 && right == 1 )
    return rule[3];
  if( left == 2 && center == 0 && right == 2 )
    return rule[4];
  if( left == 2 && center == 0 && right == 0 )
    return rule[5];
  if( left == 0 && center == 0 && right == 2 )
    return rule[6];
  if( left == 2 && center == 0 && right == 1 )
    return rule[7];
  if( left == 1 && center == 0 && right == 2 )
    return rule[8];

  if( left == 0 && center == 1 && right == 0 )
    return rule[9];
  if( left == 1 && center == 1 && right == 1 )
    return rule[10];
  if( left == 1 && center == 1 && right == 0 )
    return rule[11];
  if( left == 0 && center == 1 && right == 1 )
    return rule[12];
  if( left == 2 && center == 1 && right == 2 )
    return rule[13];
  if( left == 2 && center == 1 && right == 0 )
    return rule[14];
  if( left == 0 && center == 1 && right == 2 )
    return rule[15];
  if( left == 2 && center == 1 && right == 1 )
    return rule[16];
  if( left == 1 && center == 1 && right == 2 )
    return rule[17];

  if( left == 0 && center == 2 && right == 0 )
    return rule[18];
  if( left == 1 && center == 2 && right == 1 )
    return rule[19];
  if( left == 1 && center == 2 && right == 0 )
    return rule[20];
  if( left == 0 && center == 2 && right == 1 )
    return rule[21];
  if( left == 2 && center == 2 && right == 2 )
    return rule[22];
  if( left == 2 && center == 2 && right == 0 )
    return rule[23];
  if( left == 0 && center == 2 && right == 2 )
    return rule[24];
  if( left == 2 && center == 2 && right == 1 )
    return rule[25];
  if( left == 1 && center == 2 && right == 2 )
    return rule[26];
  return 0; //should never happen  
}

float cavoc27_randf(float min, float max)
{
	float randv, retval;
	
	randv = (float) (rand() % 32768) / 32768.0 ;
	return (retval = min + (max-min) * randv)  ;
}
void cavoc27_dsp(t_cavoc27 *x, t_signal **sp, short *count)
{
  if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr) {
  	x->vs = sp[0]->s_n;
  	x->R = sp[0]->s_sr;
  	cavoc27_init(x,1);
  }
  dsp_add(cavoc27_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec,  sp[0]->s_n);
}

