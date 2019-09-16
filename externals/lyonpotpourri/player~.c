#include "MSPd.h"

static t_class *player_class;


#include "time.h"
#include "stdlib.h"

#define MAX_CHANNELS (1)
#define DEFAULT_MAX_OVERLAP (8) // number of overlapping instances allowed
#define FORWARD 1
#define BACKWARD 2
#define ACTIVE 0
#define INACTIVE 1
#define MAX_VEC 2048

#define MAXIMUM_VECTOR (8192)

#define OBJECT_NAME "player~"
#define COMPILE_DATE "7.3.06"

typedef struct
{
	float phase; // current phase in frames
	float gain; // gain for this note
	short status;// status of this event slot
		float increment;// first increment noted (only if using static increments)
			
} t_event;

typedef struct _player
{

	t_object x_obj;
	float x_f;
	t_symbol *wavename; // name of waveform buffer
	float sr; // sampling rate
	short hosed; // buffers are bad
	float fadeout; // fadeout time in sample frames (if truncation)
	float sync; // input from groove sync signal
	float increment; // read increment
	short direction; // forwards or backwards
	int most_recent_event; // position in array where last note was initiated
	long b_nchans; // channels of buffer
	int overlap_max; // max number of simultaneous plays 
	t_event *events; //note attacks
	int active_events; // how many currently activated notes?
	short connections[4]; // state of signal connections
	short interpolation_tog; // select for interpolation or not
	short mute;
	short static_increment; // flag to use static increment (off by default)
							// variables only for Pd
	int vs; // signal vector size
	float *trigger_vec; // copy of input vector (Pd only)
	float *increment_vec; // copy of input vector (Pd only)
	t_word *b_samples; // pointer to array data
	long b_valid; // state of array
	long b_frames; // number of frames (in Pd frames are mono)
} t_player;

void player_setbuf(t_player *x, t_symbol *wavename);
void *player_new(t_symbol *msg, short argc, t_atom *argv);
t_int *player_perform_mono(t_int *w);
t_int *player_perform_mono_interpol(t_int *w);
t_int *player_perform_stereo(t_int *w);
t_int *player_perform_stereo_interpol(t_int *w);
t_int *player_perform_stereo_interpol_nocopy(t_int *w);
t_int *player_perform_hosed1(t_int *w);
t_int *player_perform_hosed2(t_int *w);
t_int *pd_player(t_int *w);

void player_dsp(t_player *x, t_signal **sp);
float player_boundrand(float min, float max);
void player_dsp_free(t_player *x);
void player_float(t_player *x, double f);
void player_interpolation(t_player *x, t_float f);
void player_mute(t_player *x, t_floatarg f);
void player_static_increment(t_player *x, t_floatarg f);
void player_stop(t_player *x);
void player_info(t_player *x);
void player_init(t_player *x,short initialized);

void player_tilde_setup(void)
{
	player_class = class_new(gensym("player~"), (t_newmethod)player_new, 
							 (t_method)player_dsp_free ,sizeof(t_player), 0, A_GIMME,0);
	CLASS_MAINSIGNALIN(player_class, t_player, x_f );
	class_addmethod(player_class, (t_method)player_dsp, gensym("dsp"), 0);
//	class_addmethod(player_class, (t_method)player_mute, gensym("mute"), A_DEFFLOAT,0);
	class_addmethod(player_class, (t_method)player_setbuf, gensym("setbuf"),A_DEFSYM, 0);
	class_addmethod(player_class, (t_method)player_mute, gensym("mute"), A_FLOAT, 0);
	class_addmethod(player_class, (t_method)player_static_increment, gensym("static_increment"), A_FLOAT, 0);
	class_addmethod(player_class, (t_method)player_stop, gensym("stop"), 0);
	potpourri_announce(OBJECT_NAME);
	
}

void player_static_increment(t_player *x, t_floatarg f)
{
	x->static_increment = f;
}

void player_stop(t_player *x)
{
	int i;
	
	for(i = 0; i < x->overlap_max; i++){
		x->events[i].status = INACTIVE;
		x->events[i].phase = 0.0;
		x->events[i].phase = 0.0;
		x->events[i].gain = 0.0;
	}
}



void player_mute(t_player *x, t_floatarg f)
{
	x->mute = f;
}



void *player_new(t_symbol *msg, short argc, t_atom *argv)
{
	
	t_player *x = (t_player *)pd_new(player_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal") );
	if(argc < 1 || argv[0].a_type != A_SYMBOL){
		post("%s: warning: no buffer name given: defaulting to "
                     "empty symbol",OBJECT_NAME);
	}
	x->wavename = atom_getsymbolarg(0,argc,argv);
	x->b_nchans = 1;	
	x->overlap_max = atom_getfloatarg(2,argc,argv);
	if(x->overlap_max <= 0 || x->overlap_max > 128){
		x->overlap_max = DEFAULT_MAX_OVERLAP;
	}
	// post("%d overlaps for %s",x->overlap_max,x->wavename->s_name);
	x->sr = sys_getsr();
	x->vs = sys_getblksize();
	if(!x->sr)
		x->sr = 44100;
	if(!x->vs)
		x->vs = 256;  
	player_init(x,0);
	//   player_setbuf(x, x->wavename);
	return (x);
}

void player_init(t_player *x,short initialized)
{
	int i;
	
	if(!initialized){
		x->most_recent_event = 0;
		x->active_events = 0;
		x->increment = 1.0;
		x->direction = FORWARD;
		
		x->events = (t_event *) calloc(x->overlap_max, sizeof(t_event));
		x->mute = 0;
		x->interpolation_tog = 1; // interpolation by default
		x->static_increment = 0; // by default increment is adjustable through note
		for(i = 0; i < x->overlap_max; i++){
			x->events[i].status = INACTIVE;
			x->events[i].increment = 0.0;
			x->events[i].phase = 0.0;
			x->events[i].gain = 0.0;
		}
		//		post("using local vectors");
		x->increment_vec = malloc(MAXIMUM_VECTOR * sizeof(float));
		x->trigger_vec = malloc(MAXIMUM_VECTOR * sizeof(float));
	} else {
		for(i = 0; i < x->overlap_max; i++){
			x->events[i].status = INACTIVE;
		}
		x->increment_vec = realloc(x->increment_vec, x->vs * sizeof(float));
		x->trigger_vec = realloc(x->trigger_vec, x->vs * sizeof(float));
	}
	
}
void player_setbuf(t_player *x, t_symbol *wavename)
{
	int frames;

	t_garray *a;
	
	x->hosed = 0;

	x->b_frames = 0;
	x->b_valid = 0;
	if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class)))
    {
		if (*wavename->s_name) pd_error(x, "player~: %s: no such array",
										wavename->s_name);
		x->b_samples = 0;
		x->hosed = 1;
    }
	else if (!garray_getfloatwords(a, &frames, &x->b_samples))
    {
		pd_error(x, "%s: bad template for player~", wavename->s_name);
		x->b_samples = 0;
		x->hosed = 1;
    }
	else  {
		x->b_frames = frames;
		x->b_valid = 1;
		garray_usedindsp(a);
	}
	if(! x->b_valid ){
		post("player~ got invalid buffer");
	}
	
}

t_int *player_perform_hosed1(t_int *w)
{
	
	//  t_player *x = (t_player *) (w[1]);
	float *outchan = (t_float *)(w[4]);
	int n = (int) w[5];
	
	memset((void *)outchan,0,sizeof(float) * n);
	return(w+6);
}

t_int *player_perform_hosed2(t_int *w)
{
	
	//  t_player *x = (t_player *) (w[1]);
	float *out1 = (t_float *)(w[4]);
	float *out2 = (t_float *)(w[5]);
	int n = (int) w[6];
	
	//  while(n--) *outchan++ = 0.0;
	memset((void *)out1,0,sizeof(float) * n);
	memset((void *)out2,0,sizeof(float) * n);
	return(w+7);
}


/* New mono version for Pd */

t_int *player_perform_mono_interpol(t_int *w)
{
	t_player *x = (t_player *) (w[1]);
	float *t_vec = (t_float *)(w[2]);
	float *i_vec = (t_float *)(w[3]);
	float *outchan = (t_float *)(w[4]);
	int n = (int) w[5];
	t_word *b_samples;
	long b_nchans;
	t_event *events = x->events;

	float increment = x->increment;
	int overlap_max = x->overlap_max;
	int iphase;
	float fphase;
	float gain;
	short insert_success;
	int new_insert;
	int i,j,k;
	float *trigger_vec = x->trigger_vec;
	float *increment_vec = x->increment_vec;
	short bail;
	short static_increment = x->static_increment;
	float maxphase;
	float frac;
	int theft_candidate;
	int flimit;
	float samp1, samp2;
	long b_frames;
	float vincrement;
	
	if(x->mute || x->hosed){
		memset((void *)outchan,0,sizeof(float) * n);
		return(w+6);
	}
	player_setbuf(x, x->wavename);
	b_samples = x->b_samples;
	b_nchans = x->b_nchans;
	b_frames = x->b_frames;  
    
	if(! x->b_valid) {
		player_stop(x);
		memset((void *)outchan,0,sizeof(float) * n);
		return(w+6);		
	}
	// DO THIS BETTER

	for(i = 0; i < n; i++){
		trigger_vec[i] = t_vec[i];
		increment_vec[i] = i_vec[i];
	}


	/* test if we even need to do anything */
	bail = 1;	
	for(i = 0; i < overlap_max; i++){
		if(events[i].status == ACTIVE){
			bail = 0;
			break;
		}
	} 
	if(bail){
		for(i = 0; i < n; i++){
			if(trigger_vec[i]){
				bail = 0;
			}
		}
	}
	if(bail){
		memset((void *)outchan,0,sizeof(float) * n);
		return(w+6);
	}
	
	/* main sample playback code */

	vincrement = increment_vec[0];

	memset((void *)outchan,0,sizeof(float) * n);
	flimit = (b_frames - 1) * 2;
	for(i = 0; i < overlap_max; i++){
		if(events[i].status == ACTIVE){
			gain = events[i].gain;
			for(j = 0; j < n; j++){ //vector loop
				iphase = events[i].phase;
				frac = events[i].phase - iphase;
				if(static_increment){
					increment = events[i].increment;
				} else {

					increment = increment_vec[j];

				}
			//	iphase *= 2;
				if(increment > 0){ // moving forward into sample
					if(iphase == flimit){
						outchan[j] += b_samples[iphase].w_float * gain;
					} else {
						samp1 = b_samples[iphase].w_float;
						samp2 = b_samples[iphase + 1].w_float;
						outchan[j] += gain * (samp1 + frac * (samp2-samp1));
					}
				} 
				// moving backwards into sample				
				else {
					if(iphase == 0.0){
						outchan[j] += b_samples[iphase].w_float * gain;
					} else {
						samp2 = b_samples[iphase].w_float;
						samp1 = b_samples[iphase - 1].w_float;
						outchan[j] += gain * (samp1 + frac * (samp2-samp1));
					}
				}
				
				if(static_increment){
					events[i].phase += events[i].increment;
				}
				else {


					events[i].phase += increment_vec[j];

				}
				if( events[i].phase < 0.0 || events[i].phase >= b_frames){
					events[i].status = INACTIVE;
					break;
				}
			}
		}
	}
	/* trigger responder and initial playback code */
	for(i=0; i<n; i++){
		if(trigger_vec[i]){
			gain = trigger_vec[i];

			increment = increment_vec[i];
			insert_success = 0;
			
			/* put new event into event list */
			for(j=0; j<overlap_max; j++){
				if(events[j].status == INACTIVE){
					events[j].status = ACTIVE;
					events[j].gain = gain;
					events[j].increment = increment;
					if(increment > 0){
						events[j].phase = 0.0;
					} else {
						events[j].phase = b_frames - 1;
					}
					insert_success = 1;
					new_insert = j;
					break;
				}
			}	
			
			if(!insert_success){ // steal a note if necessary
				
				maxphase = 0;
				theft_candidate = 0;
				for(k = 0; k < overlap_max; k++){
					if(events[k].phase > maxphase){
						maxphase = events[k].phase;
						theft_candidate = k;
					}
				}
				// post("stealing note at slot %d", theft_candidate);
				new_insert = theft_candidate;
				events[new_insert].gain = gain;
				events[new_insert].increment = increment;
				if(increment > 0){
					events[new_insert].phase = 0.0;
				} else {
					events[new_insert].phase = b_frames - 1;
				}
				insert_success = 1;
			}
			
			for(k=i; k<n; k++){
				
				//roll out for remaining portion of vector
				fphase = events[new_insert].phase;
				iphase = (int)floorf(fphase);
				frac = fphase - iphase;
		//		iphase *= 2; // double for stereo
				/* do interpolation */
				if(increment > 0){ // moving forward into sample
					if(iphase == flimit){
						outchan[k] += b_samples[iphase].w_float * gain;
					} else {
						samp1 = b_samples[iphase].w_float;
						samp2 = b_samples[iphase + 1].w_float;
						outchan[k] += gain * (samp1 + frac * (samp2-samp1));
					}
				} 
				// moving backwards into sample				
				else {
					if(iphase == 0.0){
						outchan[k] += b_samples[iphase].w_float * gain;
					} else {
						samp2 = b_samples[iphase].w_float;
						samp1 = b_samples[iphase - 1].w_float;
						outchan[k] += gain * (samp1 + frac * (samp2-samp1));
					}
				}
				/* advance phase */
				if(static_increment){
					increment = events[new_insert].increment;
				} else {

					increment = increment_vec[k];
				}
				
				events[new_insert].phase += increment; 
				
				
				/* note termination conditions */
				if( events[new_insert].phase < 0.0 || events[new_insert].phase >= b_frames){
					events[new_insert].status = INACTIVE;
					break;
				}
			}					
		}
	}
	return (w+6);
}



float player_boundrand(float min, float max)
{
	return min + (max-min) * ((float) (rand() % RAND_MAX)/ (float) RAND_MAX);
}


void player_dsp_free(t_player *x)
{

	free(x->events);
	free(x->increment_vec);
	free(x->trigger_vec);

}

void player_dsp(t_player *x, t_signal **sp)
{
	
	player_setbuf(x, x->wavename);
	
	if(x->sr != sp[0]->s_sr){
		x->sr = sp[0]->s_sr;
		if(!x->sr){
			post("warning: zero sampling rate!");
			x->sr = 44100;
		}
	} 
	
	if(x->vs != sp[0]->s_n){
		x->vs = sp[0]->s_n;
		player_init(x,1);
	}
	
	if(x->b_frames <= 0 && ! x->hosed){
		post("empty buffer, external disabled until it a sound is loaded");
		x->hosed = 1;
	}
	
	
	player_stop(x); // turn off all players to start
	
	if(x->hosed)
		dsp_add(player_perform_hosed1, 5, x, 
				sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, (t_int)sp[0]->s_n);
	else{
		dsp_add(player_perform_mono_interpol, 5, x, 
				sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, (t_int)sp[0]->s_n);
	}
	
}
