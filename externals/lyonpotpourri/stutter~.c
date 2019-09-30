#include "MSPd.h"

static t_class *stutter_class;
long rand_state = 0 ;

#define OBJECT_NAME "stutter~"

// REALLY MESSED UP - need to figure out 64-bit and may need to start from scratch

/* still needs implementation for mono. Also update to use rand() function.
 Also free memory function. Change from pre-inc/dec to normal form.
 
 Obviously this is a legacy external. Chopper probably does everything stutter does
 only better.
 */

typedef struct _stutter
	{
		t_object x_obj;
		t_float x_f;
		t_symbol *l_sym;
		t_symbol *bufname;
		t_garray *theBuffer;
		t_garray *l_buf;
		long l_chan;
		///
		int loop_samps;
		int samps_to_go ;
		int loop_start;
		int echos;
		int min_echo;
		int max_echo;
		int b_index;
		//
		int taper_samps;
		t_float taper_dur;
		int loop_min_samps;
		int loop_max_samps;
		t_float loop_min_duration;
		t_float loop_max_duration;
		t_float taper_duration;
		t_float R;
		t_float ldev;
		t_float st_dev;
		int lock_loop;
		int new_loop_loop;
		t_float buffer_duration;
		int framesize;
		int bufchans;
		short verbose;
		short mute_me;
		int *stored_starts;
		int *stored_samps;
		long b_valid;
		//float *b_samples;
        t_word *b_samples;
		long b_frames;
	} t_stutter;

t_int *stutter_perform(t_int *w);

void stutter_dsp(t_stutter *x, t_signal **sp);
void *stutter_new(t_symbol *msg, short argc, t_atom *argv);
void stutter_in1(t_stutter *x, long n);
void stutter_min_looptime(t_stutter *x, t_floatarg n);
void stutter_max_looptime(t_stutter *x, t_floatarg n);
void stutter_randomize_start(t_stutter *x, t_floatarg n);
void stutter_randomize_dur(t_stutter *x, t_floatarg n);
void stutter_lockme(t_stutter *x, t_floatarg n);
void stutter_new_loop(t_stutter *x);

void stutter_assist(t_stutter *x, void *b, long m, long a, char *s);
void stutter_dblclick(t_stutter *x);
void stutter_verbose(t_stutter *x, t_floatarg t);
void stutter_mute(t_stutter *x, t_floatarg t);
void stutter_show_loop(t_stutter *x);
void stutter_set_loop(t_stutter *x, t_symbol *msg, short argc, t_atom *argv);
void stutter_store_loop(t_stutter *x, t_floatarg loop_b_index);
void stutter_recall_loop(t_stutter *x,  t_floatarg loop_b_index);
void stutter_taper(t_stutter *x,  t_floatarg f);
void stutter_min_echo(t_stutter *x,  t_floatarg f);
void stutter_max_echo(t_stutter *x,  t_floatarg f);
void stutter_minmax_echo(t_stutter *x,  t_floatarg minf, t_floatarg maxf);
float boundrand(float min, float max);
void stutter_init(t_stutter *x,short initialized);
void stutter_info(t_stutter *x);
void stutter_version(t_stutter *x);
void stutter_setarray(t_stutter *x);
float erand(void);

t_symbol *ps_buffer;

void stutter_tilde_setup(void){
	stutter_class = class_new(gensym("stutter~"), (t_newmethod)stutter_new, 
								NO_FREE_FUNCTION,sizeof(t_stutter), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(stutter_class, t_stutter, x_f);
	class_addmethod(stutter_class,(t_method)stutter_dsp,gensym("dsp"),0);
	class_addmethod(stutter_class,(t_method)stutter_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(stutter_class,(t_method)stutter_show_loop,gensym("show_loop"),0);
	class_addmethod(stutter_class,(t_method)stutter_set_loop,gensym("set_loop"),A_GIMME,0);
	class_addmethod(stutter_class,(t_method)stutter_store_loop,gensym("store_loop"),A_FLOAT,0);
	class_addmethod(stutter_class,(t_method)stutter_recall_loop,gensym("recall_loop"),A_FLOAT,0);
	class_addmethod(stutter_class,(t_method)stutter_taper,gensym("taper"),A_FLOAT,0);
	class_addmethod(stutter_class,(t_method)stutter_min_echo,gensym("min_echo"),A_FLOAT,0);
	class_addmethod(stutter_class,(t_method)stutter_max_echo,gensym("max_echo"),A_FLOAT,0);
    class_addmethod(stutter_class,(t_method)stutter_minmax_echo,gensym("minmax_echo"),A_FLOAT,A_FLOAT,0);
	class_addmethod(stutter_class,(t_method)stutter_min_looptime,gensym("min_looptime"),A_FLOAT,0);
	class_addmethod(stutter_class,(t_method)stutter_max_looptime,gensym("max_looptime"),A_FLOAT,0);
	class_addmethod(stutter_class,(t_method)stutter_lockme,gensym("lockme"),A_FLOAT,0);
	class_addmethod(stutter_class,(t_method)stutter_new_loop,gensym("new_loop"),0);
	class_addmethod(stutter_class,(t_method)stutter_randomize_dur,gensym("randomize_dur"),A_FLOAT,0);
	class_addmethod(stutter_class,(t_method)stutter_randomize_start,gensym("randomize_start"),A_FLOAT,0);
	ps_buffer = gensym("buffer~");
	potpourri_announce(OBJECT_NAME);
}

void stutter_mute(t_stutter *x, t_floatarg tog)
{
	x->mute_me = (int)tog;
}

void stutter_verbose(t_stutter *x, t_floatarg tog)
{
	x->verbose = (int)tog;
}


void stutter_setarray(t_stutter *x)
{
	t_garray *b;
	t_symbol *bufname = x->bufname;
	int frames;
	
	if (!(b = (t_garray *)pd_findbyclass(bufname, garray_class)))
    {
		if (*bufname->s_name) pd_error(x, "stutter~: %s: no such array",
										bufname->s_name);
		x->b_valid = 0;
    }
	else if (!garray_getfloatwords(b, &frames, &x->b_samples))
    {
		pd_error(x, "%s: bad template for stutter~", bufname->s_name);
		x->b_samples = 0;
		x->b_valid = 1;
    }
	else  {
		x->b_frames = frames;
		x->b_valid = 1;
		x->theBuffer = b;
		garray_usedindsp(b);
	}
        //if(! x->b_valid ){
        //    post("stutter~ got invalid buffer");
        //}
}

t_int *stutter_perform(t_int *w)
{
	t_stutter *x = (t_stutter *)(w[1]);
	t_float *out1 = (t_float *)(w[2]);
	int n = (int) w[3];
	long b_frames = x->b_frames;
	int next_pointer = 4;
	t_word *tab;
    t_float theSample;
	long b_index;
	int loop_start, samps_to_go, echos, loop_samps, taper_samps;
	int loop_max_samps, loop_min_samps;
	int lock_loop;
	t_float sdev, ldev, startdev, st_dev;
	
	stutter_setarray(x);
    
	if(x->mute_me || ! x->b_valid) {
		while( n-- ) {
			*out1++ = 0.0;
		}
		return w+next_pointer;
	} else {
		
		tab = x->b_samples;
		b_frames = x->b_frames;
		b_index = x->b_index;
		loop_start = x->loop_start;
		loop_samps = x->loop_samps;
		samps_to_go = x->samps_to_go;
		echos = x->echos;
		taper_samps = x->taper_samps ;
		loop_max_samps = x->loop_max_samps;
		loop_min_samps = x->loop_min_samps;
		ldev = x->ldev;
		lock_loop = x->lock_loop;
		st_dev = x->st_dev;
		
		if( x->framesize != b_frames ) {
			x->framesize = b_frames;
			x->buffer_duration = (float)  b_frames / (float) x->R ;
		}	
		
		while( n-- ) {
			if( b_index < 0 ){
				b_index = 0;
			} else if( b_index > b_frames - 1) {
				b_index = 0;
			}
			theSample = tab[ b_index ].w_float;
			if( samps_to_go > loop_samps - taper_samps ){
				*out1++ = theSample * ( (float)(loop_samps - samps_to_go)/(float)taper_samps );
				++b_index;
			} else if( samps_to_go < taper_samps ) {
				*out1++ = theSample * ( (float)(samps_to_go)/(float)taper_samps );
				++b_index;
				
			} else {
				*out1++ = theSample;
				++b_index;
			}
			if( ! --samps_to_go ){
				b_index = loop_start ;
				sdev = ldev * loop_samps ;
				if( erand() < .5 ){
					sdev = -sdev;
				}
				if(  ( --echos <= 0 ) && ( ! lock_loop ) ) {
					echos = (int)boundrand((float)x->min_echo,(float)x->max_echo);
					samps_to_go = loop_samps = 
					loop_min_samps + ( erand() * (float)(loop_max_samps-loop_min_samps) ) ;
					loop_start = erand() * (b_frames - loop_samps) ;
				} else {
					loop_samps += sdev;
					
					if( loop_samps < loop_min_samps ){
						loop_samps = loop_min_samps;
					} else if( loop_samps > loop_max_samps ) {
						loop_samps = loop_max_samps;
					}
					samps_to_go = loop_samps;
					if( st_dev ) {
						startdev = erand() * st_dev * (t_float) loop_samps;
						if( erand() < .5 ){
							startdev = -startdev;
						}
						loop_start += startdev ;
						if( loop_start < 0 ) {
							loop_start = 0 ;
						} else if ( loop_start + loop_samps > b_frames) {
							loop_start = b_frames - loop_samps;
						}
					}
					if( loop_start + loop_samps >= b_frames ){
						loop_start = (b_frames - loop_samps) - 1;
					}
					
				}
			}
		}
		x->b_index = b_index;
		x->loop_start = loop_start;
		x->loop_samps = loop_samps;
		x->samps_to_go = samps_to_go;
		x->echos = echos;
		return w + next_pointer;
	}
}

float erand(void) {
	static int im = 6075 ;
	static int ia = 106 ;
	static int ic = 1283 ;
	rand_state = (rand_state * ia +  ic) % im ;
	return ( (float) rand_state / (float) im );
}

void stutter_info(t_stutter *x)
{
	post("there are %d frames in this buffer. Duration is %f.",x->b_frames, x->buffer_duration);
}

void stutter_new_loop(t_stutter *x)
{
	x->echos = (int)boundrand((float)x->min_echo,(float)x->max_echo);
	x->samps_to_go = x->loop_samps = boundrand((float)x->loop_min_samps,(float)x->loop_max_samps);
	x->loop_start = boundrand(0.0,1.0) * (x->framesize - x->loop_samps) ;
}

void stutter_show_loop(t_stutter *x)
{
	post("start %d samps %d", x->loop_start, x->loop_samps);
}

void stutter_store_loop(t_stutter *x, t_floatarg loop_b_index)
{
	int i = loop_b_index;
	x->stored_starts[i] = x->loop_start;
	x->stored_samps[i] = x->loop_samps;
	// post("loop stored at position %d", i);
}

void stutter_recall_loop(t_stutter *x, t_floatarg loop_b_index)
{
	// bug warning: recall preceding store will crash program
	// need to add warning
	int i = loop_b_index;
	
	if(!x->stored_samps[i]){
		error("no loop stored at position %d!", i);
		return;
	}
	x->loop_start = x->stored_starts[ i ];
	x->samps_to_go = x->loop_samps = x->stored_samps[ i ];
	if( x->loop_min_samps > x->loop_samps )
		x->loop_min_samps = x->loop_samps ;
	if( x->loop_max_samps < x->loop_samps )
		x->loop_max_samps = x->loop_samps ;
	// post("loop recalled from position %d", i);
}


void stutter_set_loop(t_stutter *x, t_symbol *msg, short argc, t_atom *argv)
{
	float temp;
	atom_arg_getfloat(&temp, 0, argc, argv);
	x->loop_start = temp;
	atom_arg_getfloat(&temp, 0, argc, argv);
	x->loop_samps = temp;
	//	post("loop set to: %d %d", x->loop_start, x->loop_samps);
}

void stutter_lockme(t_stutter *x, t_floatarg f)
{
    int n = (int) f;
	if( n > 0 ) {
		x->lock_loop = 1;
	} else {
		x->lock_loop = 0;
	}
}
//set min time for loop
void stutter_min_looptime(t_stutter *x, t_floatarg n)
{
	n /= 1000.0;
	
	if( n < .0001 ){
		n = .0001;
	}
	x->loop_min_samps = x->R * n ;
	if( x->loop_min_samps >= x->loop_max_samps ){
		x->loop_min_samps = x->loop_max_samps - 1;
	}
	
}

// set deviation factor
void stutter_randomize_dur(t_stutter *x, t_floatarg n)
{
	
	if( n < 0 )
		n = 0;
	if( n > 1 )
		n = 1.;
	x->ldev = n;
}
void stutter_randomize_start(t_stutter *x, t_floatarg n)
{
	
	if( n < 0 )
		n = 0;
	if( n > 1 )
		n = 1.;
	x->st_dev = n;
}

// set max time for loop
void stutter_max_looptime(t_stutter *x, t_floatarg n)
{
	n /= 1000.0;
	
	if( x->buffer_duration > 0.0 ) {
		if( n >= x->buffer_duration * .49) {
			n = x->buffer_duration * .49 ;
		}
	}
	x->loop_max_samps = x->R * n ;
	if( x->loop_max_samps <= x->loop_min_samps ){
		x->loop_max_samps = x->loop_min_samps + 1;
	}
}




void *stutter_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_stutter *x = (t_stutter *)pd_new(stutter_class);
	outlet_new(&x->x_obj, gensym("signal"));

	srand(time(0));
	
	if(argc<1){
		post("stutter~: warning: no array name given (defaulting to "
                     "empty symbol)");
	}
	x->bufchans = 1;
	x->bufname = atom_getsymbolarg(0,argc,argv);
	x->loop_min_duration = atom_getintarg(1,argc,argv)/1000.0;
	x->loop_max_duration = atom_getintarg(2,argc,argv)/1000.0;
	x->taper_duration = atom_getintarg(3,argc,argv)/1000.0;
	
	if(!x->loop_min_duration)
		x->loop_min_duration = .02;
	if(!x->loop_max_duration)
		x->loop_max_duration = 0.2;
	if(!x->taper_duration)
		x->taper_duration = .002;
	
	x->R = sys_getsr();
	if(!x->R)
		x->R = 44100.0;
	stutter_init(x,0);
	return x;
}

float boundrand(float min, float max)
{
	return min + (max-min) * ((float)rand()/RAND_MAX);
}

void stutter_taper(t_stutter *x,  t_floatarg f)
{
	f *= .001;
	if(f>0){
		x->taper_duration = f;
		x->taper_samps = x->R * x->taper_duration;
	}
}

void stutter_min_echo(t_stutter *x,  t_floatarg f)
{
	int ec = (int)f;
	if(ec>0 && ec < x->max_echo){
		x->min_echo = ec;
	} else {
		error("min echo must be less than max echo, and greater than zero");
	}
}
void stutter_max_echo(t_stutter *x,  t_floatarg f)
{
	int ec = (int)f;
	
	if(ec > x->min_echo){
		x->max_echo = ec;
	} else {
		error("max echo must be greater than min echo");
	}
}

void stutter_minmax_echo(t_stutter *x,  t_floatarg minf, t_floatarg maxf)
{
    int minec = (int)minf;
    int maxec = (int)maxf;
    
    if( minec < maxec){
        x->min_echo = minec;
        x->max_echo - maxec;
    }
    else {
        error("bad inputs to minmax_echo");
    }
}


void stutter_init(t_stutter *x,short initialized)
{
	int i;
	
	if(!initialized){
		x->loop_min_samps = x->loop_min_duration * (float)x->R;
		x->loop_max_samps = x->loop_max_duration * (float)x->R;
		x->samps_to_go = x->loop_samps = x->loop_min_samps + 
		(erand() * (float)(x->loop_max_samps-x->loop_min_samps) ) ;
		
		x->loop_start = 0;
		x->min_echo = 2;
		x->max_echo = 12;
		x->echos = (int) boundrand((float)x->min_echo,(float)x->max_echo);
		x->b_index = x->loop_start ;
		x->samps_to_go = x->loop_samps;
		x->taper_samps = x->R * x->taper_duration;
		x->ldev = 0;
		x->lock_loop = 0;
		x->buffer_duration = 0.0 ;
		x->st_dev = 0.0;
		x->framesize = 0;
		x->new_loop_loop = 0;
		x->mute_me = 0;
		x->verbose = 0;
		x->stored_starts = t_getbytes(1024 * sizeof(int));
		x->stored_samps = t_getbytes(1024 * sizeof(int));
		for(i = 0; i < 1024; i++){
			x->stored_samps[i] = 0;
		}
	} else {
		x->loop_min_samps = x->loop_min_duration * (float)x->R;
		x->loop_max_samps = x->loop_max_duration * (float)x->R;
		x->samps_to_go = x->loop_samps = x->loop_min_samps + 
		(erand() * (float)(x->loop_max_samps - x->loop_min_samps)) ;
		x->loop_start = 0;
		x->b_index = x->loop_start;
		x->samps_to_go = x->loop_samps;
		x->taper_samps = x->R * x->taper_duration;
		x->ldev = 0;
		x->lock_loop = 0;
		x->buffer_duration = 0.0 ;
		x->st_dev = 0.0;
		x->framesize = 0;
		x->new_loop_loop = 0;
	}
}
void stutter_dsp(t_stutter *x, t_signal **sp)
{
    stutter_setarray(x);

	if(x->R != sp[0]->s_sr){
		x->R = sp[0]->s_sr;
		x->taper_samps = x->R * x->taper_duration;
		x->loop_min_samps = .02 * (t_float)x->R;
		x->loop_max_samps = .2 * (t_float) x->R;
	}
	dsp_add(stutter_perform, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);
}

