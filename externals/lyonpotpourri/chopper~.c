#include "MSPd.h"

static t_class *chopper_class;

#define MAXSTORE 1024
#define OBJECT_NAME "chopper~"
	
typedef struct _chopper
{

  t_object x_obj;
  float x_f;
  t_symbol *l_sym;
  long l_chan;
  float increment;
  double fbindex;
  float buffer_duration;
  float minseg;
  float maxseg;
  float segdur;
  float minincr;
  float maxincr;
  int loop_samps;
  int samps_to_go ;
  int loop_start;
  int bindex ;
  int taper_samps;
  int loop_min_samps;
  int loop_max_samps;
  float R;
  float ldev;
  float st_dev ;
  int lock_loop;
  int force_new_loop;
  int framesize;
  short mute;
  short disabled;
  int setup_chans;
  int *stored_starts;
  int *stored_samps;   
  float *stored_increments;
  short preempt;
  short loop_engaged;
  short data_recalled;
  short initialize_loop;
  short fixed_increment_on;
  float fixed_increment;
  float retro_odds;
  float fade_level;
  int transp_loop_samps;
  float taper_duration;
  short lock_terminated;
  int preempt_samps;
  int preempt_count;
  short recalling_loop;
  float jitter_factor;
  float rdur_factor;
  float rinc_factor;
  short increment_adjusts_loop ;
  short loop_adjust_inverse;
  long b_frames;
  long b_nchans;
  t_word *b_samples;
} t_chopper;


t_int *chopper_perform_stereo(t_int *w);
t_int *choppermono_perform(t_int *w);
t_int *chopper_perform_stereo_nointerpol(t_int *w);
t_int *chopper_perform_mono(t_int *w);
t_int *chopper_pd_perform(t_int *w);
void chopper_dsp(t_chopper *x, t_signal **sp);
void chopper_set(t_chopper *x, t_symbol *s);
void chopper_mute(t_chopper *x, t_floatarg toggle);
void chopper_increment_adjust(t_chopper *x, t_floatarg toggle);
void chopper_adjust_inverse(t_chopper *x, t_floatarg toggle);
void *chopper_new(t_symbol *msg, short argc, t_atom *argv);
void chopper_in1(t_chopper *x, long n);
void chopper_set_minincr(t_chopper *x, t_floatarg n);
void chopper_set_maxincr(t_chopper *x, t_floatarg n);
void chopper_set_minseg(t_chopper *x, t_floatarg n);
void chopper_set_maxseg(t_chopper *x, t_floatarg n);
void chopper_taper(t_chopper *x, t_floatarg f);
void chopper_fixed_increment(t_chopper *x, t_floatarg f);
void chopper_lockme(t_chopper *x, t_floatarg n);
void chopper_force_new(t_chopper *x);
float chopper_boundrand(float min, float max);
void chopper_assist(t_chopper *x, void *b, long m, long a, char *s);
void chopper_dblclick(t_chopper *x);
void chopper_show_loop(t_chopper *x);
void chopper_set_loop(t_chopper *x, t_symbol *msg, short argc, t_atom *argv);
void chopper_randloop( t_chopper *x);
void chopper_store_loop(t_chopper *x, t_floatarg loop_bindex);
void chopper_recall_loop(t_chopper *x,  t_floatarg loop_bindex);
void chopper_free(t_chopper *x) ;
void chopper_retro_odds(t_chopper *x, t_floatarg f);
void chopper_jitter(t_chopper *x, t_floatarg f);
void chopper_jitterme(t_chopper *x);
void chopper_rdur(t_chopper *x, t_floatarg f);
void chopper_rdurme(t_chopper *x);
void chopper_rinc(t_chopper *x, t_floatarg f);
void chopper_rincme(t_chopper *x);
void chopper_adjust_inverse(t_chopper *x, t_floatarg toggle);
t_int *chopper_performtest(t_int *w);
void chopper_init(t_chopper *x, short initialized);
void chopper_seed(t_chopper *x, t_floatarg seed);
void chopper_testrand(t_chopper *x);

t_symbol *ps_buffer;


void chopper_testrand(t_chopper *x)
{
	float rval = chopper_boundrand(0.0, 1.0);
	post("random btwn 0.0 1.0: %f",rval);
}

void chopper_mute(t_chopper *x, t_floatarg toggle)
{
	x->mute = (short) toggle;
}

void chopper_seed(t_chopper *x, t_floatarg seed)
{
	srand((long)seed);
}

void chopper_tilde_setup(void){
  chopper_class = class_new(gensym("chopper~"), (t_newmethod)chopper_new, 
      (t_method)chopper_free ,sizeof(t_chopper), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(chopper_class, t_chopper, x_f);
  class_addmethod(chopper_class,(t_method)chopper_dsp,gensym("dsp"),0);
  class_addmethod(chopper_class,(t_method)chopper_mute,gensym("mute"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_taper,gensym("taper"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_fixed_increment,gensym("fixed_increment"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_retro_odds,gensym("retro_odds"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_show_loop,gensym("show_loop"),0);
  class_addmethod(chopper_class,(t_method)chopper_set_loop,gensym("set_loop"),A_GIMME,0);
  class_addmethod(chopper_class,(t_method)chopper_store_loop,gensym("store_loop"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_recall_loop,gensym("recall_loop"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_increment_adjust,gensym("increment_adjust"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_adjust_inverse,gensym("adjust_inverse"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_jitter,gensym("jitter"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_rdur,gensym("rdur"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_rinc,gensym("rinc"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_set_minincr,gensym("set_minincr"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_set_maxincr,gensym("set_maxincr"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_set_minseg,gensym("set_minseg"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_set_maxseg,gensym("set_maxseg"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_lockme,gensym("lockme"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_force_new,gensym("force_new"),0);
  class_addmethod(chopper_class,(t_method)chopper_seed,gensym("seed"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_testrand,gensym("testrand"),0);
  potpourri_announce(OBJECT_NAME);
}


void chopper_increment_adjust(t_chopper *x, t_floatarg toggle)
{
  x->increment_adjusts_loop = (short)toggle;
  
}

void chopper_adjust_inverse(t_chopper *x, t_floatarg toggle)
{
  x->loop_adjust_inverse = (short)toggle;
}

void chopper_fixed_increment(t_chopper *x, t_floatarg f)
{
  float new_samps = 0;
  float rectf;

  x->fixed_increment = f;
  if( f ){
    x->fixed_increment_on = 1;
  } else {
    x->fixed_increment_on = 0;
  }
  
  rectf = fabs(f);
  
  if( x->lock_loop && rectf > 0.0 ){

    if( x->loop_adjust_inverse ){
      new_samps = (float) x->loop_samps * rectf ;
    } else {
      new_samps = (float) x->loop_samps / rectf ;
    }
    if( f > 0 ){
      if( x->loop_start + new_samps >= x->framesize ){
		return;
      } else {
		x->increment = x->fixed_increment;
      }
    } else {
      if( x->loop_start - new_samps < 0) {
		return;
      } else {
		x->increment = x->fixed_increment;
      }
    }
		
  }
  
  if( x->increment_adjusts_loop ){
    x->transp_loop_samps = new_samps;
  }
  

}

void chopper_jitter(t_chopper *x, t_floatarg f)
{
  f *= 0.1; // scale down a bit
  if( f >= 0. && f <= 1.0 )
    x->jitter_factor = f;
}

void chopper_rdur(t_chopper *x, t_floatarg f)
{

  if( f >= 0. && f <= 1.0 )
    x->rdur_factor = f;
}

void chopper_rinc(t_chopper *x, t_floatarg f)
{
  // f *= 0.1; // scale down a bit

  if( f >= 0. && f <= 1.0 )
    x->rinc_factor = f;
}


void chopper_retro_odds(t_chopper *x, t_floatarg f)
{

  if( f < 0 )
    f = 0;
  if( f > 1 )
    f = 1;
		
  x->retro_odds = f;

}

void chopper_show_loop(t_chopper *x)
{
  post("start: %d, samps: %d, increment: %f", x->loop_start, x->transp_loop_samps, x->increment);
  post("minloop %f, maxloop %f", x->minseg, x->maxseg);
}

void chopper_store_loop(t_chopper *x, t_floatarg f)
{
  int loop_bindex = (int) f;

  if( loop_bindex < 0 || loop_bindex >= MAXSTORE ){
    error("bindex %d out of range", loop_bindex);
    return;
  }
	
  x->stored_starts[ loop_bindex ] = x->loop_start;
  x->stored_samps[ loop_bindex ] = x->transp_loop_samps;
  x->stored_increments[ loop_bindex ] = x->increment;

  post("storing loop %d: %d %d %f",loop_bindex, 
       x->stored_starts[ loop_bindex ],x->stored_samps[ loop_bindex ],  x->stored_increments[ loop_bindex ] );

}

void chopper_recall_loop(t_chopper *x, t_floatarg f)
{
  // bug warning: recall preceding store will crash program
  // need to add warning
int loop_bindex = (int) f;

  if( loop_bindex < 0 || loop_bindex >= MAXSTORE ){
    error("bindex %d out of range", loop_bindex);
    return;
  }
	
  if( ! x->stored_samps[ loop_bindex ] ){
    error("no loop stored at position %d!", loop_bindex);
    return;
  }

	
  x->loop_start = x->stored_starts[ loop_bindex ];
  x->samps_to_go = x->transp_loop_samps = x->stored_samps[ loop_bindex ];
	 
  if( x->loop_min_samps > x->transp_loop_samps )
    x->loop_min_samps = x->transp_loop_samps ;
  if( x->loop_max_samps < x->transp_loop_samps )
    x->loop_max_samps = x->transp_loop_samps ;
  x->increment = x->stored_increments[ loop_bindex ];
  x->preempt_count = x->preempt_samps;
  // post("preempt samps:%d", x->preempt_count);
  x->recalling_loop = 1;
  //  x->data_recalled = 1;
}

void chopper_set_loop(t_chopper *x, t_symbol *msg, short argc, t_atom *argv)
{
  if( argc < 3 ){
    error("format: start samples increment");
    return;
  }
  x->loop_start = atom_getintarg(0,argc,argv);
  x->loop_samps = atom_getintarg(1,argc,argv);
  x->increment = atom_getfloatarg(2,argc,argv);
  x->data_recalled = 1;

  x->samps_to_go = x->loop_samps;
  x->fbindex = x->bindex = x->loop_start;
//  post("loop set to: st %d samps %d incr %f", x->loop_start, x->loop_samps,x->increment);
}

void chopper_taper(t_chopper *x, t_floatarg f)
{
  f /= 1000.0;
	
  if( f > 0 ){
    x->taper_samps = (float) x->R * f ;
  }
  if( x->taper_samps < 2 )
    x->taper_samps = 2;
}



void *chopper_new(t_symbol *msg, short argc, t_atom *argv)
{
  t_chopper *x = (t_chopper *)pd_new(chopper_class);
  outlet_new(&x->x_obj, gensym("signal"));
  x->R = sys_getsr();
  x->l_sym = atom_getsymbolarg(0,argc,argv);
  chopper_init(x,0);
  return (x);
}


void chopper_init(t_chopper *x, short initialized) 
{
	if(!initialized){

		srand(time(0));
	
	  if(!x->R) {
		error("zero sampling rate - set to 44100");
		x->R = 44100;
	  }
	  x->minseg = 0.1;
	  x->maxseg = 0.8 ;
	  x->minincr = 0.5 ;
	  x->maxincr = 2.0 ;
	  x->data_recalled = 0;		
	  x->segdur = 0;
	  x->bindex = 0 ;
	  x->taper_duration /= 1000.0;
	  if( x->taper_duration < .0001 || x->taper_duration > 10.0 )
		x->taper_duration = .0001;
		x->increment_adjusts_loop = 0;
	  x->taper_samps = x->R * x->taper_duration;
	  if(x->taper_samps < 2)
		x->taper_samps = 2;
		
	  x->preempt_samps = 5;
	  x->loop_adjust_inverse = 0;
	  x->preempt = 1;
	  x->recalling_loop = 0;	
	  x->ldev = 0;
	  x->lock_loop = 0;
	  x->buffer_duration = 0.0 ;
	  x->st_dev = 0.0;
	  x->framesize = 0;
	  x->force_new_loop = 0;
	  x->mute = 0;
	  x->disabled = 1;
	  x->initialize_loop = 1;
	  x->loop_engaged = 0;
	  x->fixed_increment_on = 0;
	  x->retro_odds = 0.5;
	  x->fade_level = 1.0;
	  x->lock_terminated = 0;
		
	  x->stored_starts = calloc(MAXSTORE, sizeof(int));
	  x->stored_samps = calloc(MAXSTORE, sizeof(int));
	  x->stored_increments = calloc(MAXSTORE, sizeof(int));
		
	} else {
		x->taper_samps = x->R * x->taper_duration;
		if(x->taper_samps < 2)
		  x->taper_samps = 2;
	}
}

void chopper_free(t_chopper *x) 
{
  free(x->stored_increments);
  free(x->stored_samps);
  free(x->stored_starts);
}

void chopper_jitterme(t_chopper *x)
{
  float new_start;
  float jitter_factor = x->jitter_factor;
  new_start = (1.0 + chopper_boundrand(-jitter_factor, jitter_factor) ) * (float) x->loop_start ;
	
  if( new_start < 0 ){
//    error("jitter loop %d out of range", new_start);
    new_start = 0;

  }
  else if( new_start + x->transp_loop_samps >= x->framesize ){
//    error("jitter loop %d out of range", new_start);
    new_start = x->framesize - x->transp_loop_samps ;
  }
  if( new_start >= 0 )
    x->loop_start = new_start;
}

void chopper_rdurme(t_chopper *x)
{
  float new_dur;
  float rdur_factor = x->rdur_factor;
	
  new_dur = (1.0 + chopper_boundrand( -rdur_factor, rdur_factor)) * (float) x->transp_loop_samps;
  if( new_dur > x->loop_max_samps )
    new_dur = x->loop_max_samps;
  if( new_dur < x->loop_min_samps )
    new_dur = x->loop_min_samps;

  x->transp_loop_samps = new_dur;
}

void chopper_rincme(t_chopper *x )
{
  float new_inc = 0;
//  int count = 0;
  int new_samps;
  float rinc_factor = x->rinc_factor;
	
  /* test generate a new increment */
  new_inc = (1.0 + chopper_boundrand( 0.0, rinc_factor)) ;
  if( chopper_boundrand(0.0,1.0) > 0.5 ){
    new_inc = 1.0 / new_inc;
  }
	
  // test for transgression
	
//	post("increment adjust:%d",x->increment_adjusts_loop);
	
  if( fabs(new_inc * x->increment) < x->minincr ) {
    new_inc = x->minincr / fabs(x->increment) ; // now when we multiply - increment is set to minincr
  }
  else if ( fabs(new_inc * x->increment) > x->maxincr ){
    new_inc = x->maxincr / fabs(x->increment) ; // now when we multiply - increment is set to maxincr
  }

 if(x->increment_adjusts_loop){
 	 new_samps = (float) x->transp_loop_samps / new_inc ; 
  } else {
  	new_samps = x->transp_loop_samps;
  }
    
  new_inc *= x->increment ;
  if( x->increment > 0 ){
    if( x->loop_start + new_samps >= x->framesize ){
      new_samps = (x->framesize - 1) - x->loop_start ;
    }
  } else {
    if( x->loop_start - new_samps < 0) {
      new_samps = x->loop_start + 1;
    }
  }
  x->transp_loop_samps = new_samps;
  x->increment = new_inc;		
}

void chopper_randloop( t_chopper *x )
{
  int framesize = x->b_frames;//test
//  long bindex = x->fbindex;
  float segdur = x->segdur;
  int loop_start = x->loop_start;
  int loop_samps = x->loop_samps;
  int transp_loop_samps = x->transp_loop_samps;
  int samps_to_go = x->samps_to_go;
  float increment = x->increment;
//  int taper_samps = x->taper_samps ;
//  float taper_duration = x->taper_duration;
  float minincr = x->minincr;
  float maxincr = x->maxincr;
  float minseg = x->minseg;
  float maxseg = x->maxseg;
  float buffer_duration = x->buffer_duration;
  float R = x->R;
  float fixed_increment = x->fixed_increment;

  short fixed_increment_on = x->fixed_increment_on;
  float retro_odds = x->retro_odds;
	
  if(fixed_increment_on){
    increment = fixed_increment;
  } else {
    increment = chopper_boundrand(minincr,maxincr);
  }
  segdur = chopper_boundrand( minseg, maxseg );
  loop_samps = segdur * R * increment; // total samples in segment
//  post("rand: segdur %f R %f increment %f lsamps %d",segdur,R,increment,loop_samps);
  transp_loop_samps = segdur * R ; // actual count of samples to play back
  samps_to_go = transp_loop_samps;  
  if( loop_samps >= framesize ){
    loop_samps = framesize - 1;
    loop_start = 0;
  } else {
//    post("rand: bufdur %f segdur %f",buffer_duration, segdur);
    loop_start = R * chopper_boundrand( 0.0, buffer_duration - segdur );
    if( loop_start + loop_samps >= framesize ){
      loop_start = framesize - loop_samps;
      if( loop_start < 0 ){
				loop_start = 0;
				error("negative starttime");
      }
    }
  }
  if( chopper_boundrand(0.0,1.0) < retro_odds ){
    increment *= -1.0 ;
    loop_start += (loop_samps - 1);
  }

//	post("randset: lstart %d lsamps %d incr %f segdur %f",loop_start,loop_samps,increment,segdur);
  x->samps_to_go = samps_to_go;
  x->fbindex = x->bindex = loop_start;
  x->loop_start = loop_start;
  x->loop_samps = loop_samps;
  x->transp_loop_samps = transp_loop_samps;
  x->increment = increment;
  x->segdur = segdur;
}

t_int *chopper_pd_perform(t_int *w)
{
  int bindex, bindex2;
  float sample1, m1, m2;	
  t_chopper *x = (t_chopper *)(w[1]);
  t_float *out1 = (t_float *)(w[2]);
  int n = (int) w[3];
  

  /*********************************************/
  t_word *tab = x->b_samples;
  long b_frames = x->b_frames;
  long nc = x->b_nchans;
  float segdur = x->segdur;
  int taper_samps = x->taper_samps ;
  float taper_duration = x->taper_duration;
  float minseg = x->minseg;
  float maxseg = x->maxseg;
  int lock_loop = x->lock_loop;
  int force_new_loop = x->force_new_loop;
  float R = x->R;
  short initialize_loop = x->initialize_loop;
  float fade_level = x->fade_level;
  short preempt = x->preempt;
  int preempt_count = x->preempt_count;
  int preempt_samps = x->preempt_samps;
  short recalling_loop = x->recalling_loop;
  float preempt_gain;
  float jitter_factor = x->jitter_factor;
  float rdur_factor = x->rdur_factor;
  float rinc_factor = x->rinc_factor;

  if(x->mute){
      while(n--) { *out1++ = 0.0; }
      return (w+4);
  }
	
  /* SAFETY CHECKS */
  if( b_frames <= 0 || nc != 1) {
    x->disabled = 1;
  }
	
  if(x->mute || x->disabled){
    while(n--){
      *out1++ = 0.0;
    }
    return (w+4);
  }
	
  if(x->framesize != b_frames) {
    x->framesize = b_frames ;
    x->buffer_duration = (float)  b_frames / R ;
    initialize_loop = 1;
  }	
  else if(x->buffer_duration <= 0.0) { /* THIS WILL HAPPEN THE FIRST TIME */
    x->framesize = b_frames ;
    x->buffer_duration = (float)  b_frames / R ;
    initialize_loop = 1;
//	post("initializing from perform method");
  }
  if(maxseg > x->buffer_duration){
    maxseg = x->buffer_duration ;
  }
	
  if(minseg < 2. * taper_duration)
    minseg = taper_duration;

  /* SET INITIAL SEGMENT */
	
  bindex = x->fbindex;
  
  if(initialize_loop){ /* FIRST TIME ONLY */
    chopper_randloop(x);
    bindex = x->fbindex;
    initialize_loop = 0;
  }

  while(n--){
    if( lock_loop )  {
      if ( recalling_loop ) { 
        bindex = x->fbindex ;
		x->fbindex += x->increment;
		--preempt_count;
		preempt_gain = fade_level  * ((float) preempt_count / (float) preempt_samps);
		*out1++ = tab[bindex].w_float * preempt_gain;
		if( preempt_count <= 0) {
		  bindex = x->fbindex = x->loop_start;
		  recalling_loop = 0;
		}
      }
		
      else if(force_new_loop){

		if( bindex < 0 || bindex >= b_frames ){
		  x->fbindex = bindex = b_frames/2; // start in the middle
		}
		// should switch to <
		if( preempt && preempt_samps > x->samps_to_go ){ /* PREEMPT FADE */

		  --preempt_count;
		  preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );
		  bindex = x->fbindex ;
		  x->fbindex += x->increment;
			 
						
		  *out1++ = tab[ bindex ].w_float * preempt_gain;
		  if(! preempt_count) {
			chopper_randloop(x);
			bindex = x->fbindex;
			force_new_loop = 0;
		  }
		} 
		else { 
		  /* IMMEDIATE FORCE NEW LOOP AFTER PREEMPT FADE */
		  chopper_randloop(x);
		  
		  force_new_loop = 0;
		  bindex = x->fbindex ;
		  bindex2 = bindex << 1;	
		  x->fbindex += x->increment;
		  
		  --(x->samps_to_go);
		  if( x->samps_to_go <= 0 ){
			x->fbindex = x->loop_start;
			bindex = x->fbindex;
			x->samps_to_go = x->transp_loop_samps;
		  }
		  if( x->samps_to_go > x->transp_loop_samps - taper_samps ){
			fade_level =  (float)(x->transp_loop_samps - x->samps_to_go)/(float)taper_samps ;
			*out1++ = tab[bindex].w_float * fade_level;
		  } else if( x->samps_to_go < taper_samps ) {
			fade_level = (float)(x->samps_to_go)/(float)taper_samps;
			*out1++ = tab[bindex].w_float * fade_level;
		  } else {	
			fade_level = 1.0;
			*out1++ = tab[bindex].w_float;
		  }
		}
				
      } 
      /* REGULAR PLAYBACK */
      else { 

		if( bindex < 0 || bindex >= b_frames ){
		  error("lock_loop: bindex %d is out of range", bindex);
		  x->fbindex = bindex = b_frames / 2;
		}
		bindex = floor( (double) x->fbindex );
		m2 = x->fbindex - bindex ;
		m1 = 1.0 - m2;
		
	//	bindex2 = bindex << 1;
		x->fbindex += x->increment;
		
		--(x->samps_to_go);
		if( x->samps_to_go <= 0 ){
		  if( rdur_factor ){
			chopper_rdurme( x );
		  }
		  if( jitter_factor ){
			chopper_jitterme( x );
		  }
		  if( rinc_factor ) {
			chopper_rincme( x );
		  }
		  x->fbindex = x->loop_start;
		  // x->fbindex -= x->transp_loop_samps;
		  bindex = x->fbindex;
		  x->samps_to_go = x->transp_loop_samps;
		}
		
		if( bindex >= b_frames ){
		  sample1 = tab[bindex].w_float;
		} else {
		  sample1 = m1 * tab[bindex].w_float + m2 * tab[bindex + 1].w_float;
		}
		if( x->samps_to_go > x->transp_loop_samps - taper_samps ){
		  fade_level =  (float)(x->transp_loop_samps - x->samps_to_go)/(float)taper_samps ;
		  *out1++ = sample1 * fade_level;
		} 
		else if( x->samps_to_go < taper_samps ) {
		  fade_level = (float)(x->samps_to_go)/(float)taper_samps;
		  *out1++ = sample1 * fade_level;
		} 
		else {	
		  fade_level = 1.0;
		  *out1++ = sample1;
		}
      }
    } /* END OF LOCK LOOP */
    /* RECALL STORED LOOP */
    
    else if (recalling_loop) { 
      bindex = x->fbindex ;
      x->fbindex += x->increment;
      --preempt_count;
      preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );
					
      *out1++ = tab[bindex].w_float * preempt_gain;
      if( preempt_count <= 0) {
		x->fbindex = x->loop_start;
		bindex = x->fbindex;
		recalling_loop = 0;
      }
    }
    
    else {
      if( force_new_loop ){
	/* FORCE LOOP CODE : MUST PREEMPT */
		force_new_loop = 0;
	/* NEED CODE HERE*/
      } 
      else {  /* NORMAL OPERATION */
		fade_level = 1.0; /* default level */

		if( bindex < 0 || bindex >= b_frames ){
		  error("force loop: bindex %d is out of range", bindex);
		  post("frames:%d start:%d, samps2go:%d, tloopsamps:%d, increment:%f", 
			   x->framesize, bindex, x->samps_to_go, x->transp_loop_samps, x->increment);
		  chopper_randloop(x);
		  bindex = x->fbindex;
		}
		bindex = x->fbindex ;
		x->fbindex += x->increment;
		
		if( x->samps_to_go > x->transp_loop_samps - taper_samps ){
		  fade_level =  (float)(x->transp_loop_samps - x->samps_to_go)/(float)taper_samps ;
		  *out1++ = tab[bindex].w_float * fade_level;
		} 
		else if(x->samps_to_go < taper_samps) {
		  fade_level = (float)(x->samps_to_go)/(float)taper_samps;
		  *out1++ = tab[bindex].w_float * fade_level;
		} 
		else {	
		  fade_level = 1.0;
		  *out1++ = tab[bindex].w_float;
		}
		--(x->samps_to_go);
		if( x->samps_to_go <= 0 ){
		  chopper_randloop( x );
		  bindex = x->fbindex;
		}
      }
    }
  }

  x->recalling_loop = recalling_loop;
  x->fade_level = fade_level;
  x->initialize_loop = initialize_loop;
  x->maxseg = maxseg;
  x->minseg = minseg;	
  x->segdur = segdur;
  x->force_new_loop = force_new_loop;
  return (w+4);
  
}

void chopper_force_new(t_chopper *x)
{
  x->preempt_count = x->preempt_samps;
  x->force_new_loop = 1;

}

void chopper_lockme(t_chopper *x, t_floatarg n)
{
	x->lock_loop = (short) n;
//	post("lock loop set to %d from %f",x->lock_loop,n);
}

//set min time for loop
void chopper_set_minincr(t_chopper *x, t_floatarg n)
{
//  post("set minincr to %f", n);
	
  if( n < .005 ){
    n = .005;
  }
  //x->minincr = n ;
}

// set deviation factor
void chopper_set_maxseg(t_chopper *x, t_floatarg n)
{

  n /= 1000.0 ; // convert to seconds
  if( n > 120. )
    n = 120.;
  //post("set maxseg to %f", n);
  x->maxseg = n;
  x->loop_max_samps = x->maxseg * x->R;
}

void chopper_set_minseg(t_chopper *x, t_floatarg n)
{

  n /= 1000.0 ; // convert to seconds
	
  if( n < 0.03 )
    n = 0.03;
  //post("set minseg to %f", n);
  x->minseg = n;
  x->loop_min_samps = x->minseg * x->R;
}

// set max time for loop
void chopper_set_maxincr(t_chopper *x, t_floatarg n)
{
  if( n > 4 ){
    n = 4;
  }
  //post("set maxincr to %f", n);
  x->maxincr =  n ;
}



void chopper_set(t_chopper *x, t_symbol *wavename)
{
  int frames;
  t_garray *a;
  x->disabled = 0;

  x->b_frames = 0;
  x->b_nchans = 1;
  if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class))) {
      if (*wavename->s_name) pd_error(x, "chopper~: %s: no such array",
				      wavename->s_name);
      x->b_samples = 0;
      x->disabled = 1;
    }
  else if (!garray_getfloatwords(a, &frames, &x->b_samples)) {
      pd_error(x, "%s: bad template for chopper~", wavename->s_name);
      x->b_samples = 0;
      x->disabled = 1;
    }
  else  {
    x->b_frames = frames;
//    post("%d frames in buffer",x->b_frames);
    garray_usedindsp(a);
  }
}



void chopper_dsp(t_chopper *x, t_signal **sp)
{
  chopper_set(x,x->l_sym);
  if(x->R != sp[0]->s_sr){
    x->R = sp[0]->s_sr;
  	chopper_init(x,1);
  }
  if(x->disabled){
    return;
  }

	dsp_add(chopper_pd_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);

}


float chopper_boundrand(float min, float max)
{
   return min + (max-min) * ((float) (rand() % RAND_MAX)/(float)RAND_MAX);

}
