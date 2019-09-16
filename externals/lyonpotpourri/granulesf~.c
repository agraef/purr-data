#include "MSPd.h"

#define MAXGRAINS (512) // just for present to get lower overhead

#define MAXSCALE (8192)
#define OBJECT_NAME "granulesf~"

static t_class *granulesf_class;

typedef struct {
    float amplitude;
    float panL;
    float panR;
    long delay; // samples to wait until event starts
    long duration;// length in samples of event
    float phase; // phase for frequency oscillator
    float ephase; // phase for envelope
    float si; // sampling increment for frequency
    float esi; // sampling increment for envelope
    float endframe;//boundary frame (extremes are 0 or size-1); approach depends on sign of si
    short active;//status of this slot (inactives are available for new grains)
} t_grain;

typedef struct {
	t_word *b_samples;
	long b_frames;
	long b_nchans;//not needed here
} t_pdbuffer;


typedef struct _granulesf
{
    
    t_object x_obj;
    float x_f;
    t_pdbuffer *wavebuf; // holds waveform samples
    t_pdbuffer *windowbuf; // holds window samples
	t_symbol *wavename; // name of waveform buffer
	t_symbol *windowname; // name of window buffer
    
	float sr; // sampling rate
	short mute;
	short hosed; // buffers are bad
	/* Global grain data*/
	long events; // number of events in a block
	long horizon; // length of block for random events
	float min_incr; // minimum frequency for a grain
	float max_incr; // maximum frequency for a grain
	float minpan; // minimum pan for a grain
	float maxpan; // maxium pan for a grain
	float minamp; // minimum amplitude for a grain
	float maxamp; // maximum amplitude for a grain
	float mindur; // minumum duration for a grain
	float maxdur; // maximum duration for a grain
	t_grain *grains; // stores grain data
	float *pitchscale; // contains a frequency grid for pitch constraint
	int pitchsteps; // number of members in scale
	float transpose; // factor for scaling all pitches
	float pitch_deviation; // factor to adjust scaled pitches
	short steady; // toggles pulsed rhythmic activity
	float lowblock_increment; //lowest allowed frequency
	float highblock_increment;// highest allowed frequency
	float mindur_ms;//store duration in ms
	float maxdur_ms;//ditto
	float horizon_ms;//ditto
	short constrain_scale;//flag to only use bounded portion of scale rather than all of it
	short nopan;//stereo channels go straight out, mono goes to center
	long minskip;//minimum inskip in samples (default = zero)
	long maxskip;//maximum inskip in samples (default = maximum possible given dur/increment of note)
	long b_nchans;//channels in buffer (always 1 for Pd, at least today)
	long b_frames;//frames in waveform buffer
	float retro_odds;//odds to play sample backwards
	short interpolate;//flag to interpolate samples - on by default
	short interpolate_envelope;//flag to interpolate envelope
} t_granulesf;

void granulesf_setbuf(t_granulesf *x, t_symbol *wavename, t_symbol *windowname);
void *granulesf_new(t_symbol *msg, short argc, t_atom *argv);
t_int *granulesf_perform(t_int *w);
t_int *granulesf_perform_no_interpolation(t_int *w);
t_int *granulesf_performhose(t_int *w);
void granulesf_dsp(t_granulesf *x, t_signal **sp);
void granulesf_reload(t_granulesf *x);
void granulesf_spray(t_granulesf *x);
void granulesf_pitchspray(t_granulesf *x);
void granulesf_transpose(t_granulesf *x, t_floatarg t);
void granulesf_pitchdev(t_granulesf *x, t_floatarg d);
void granulesf_lowblock(t_granulesf *x, t_floatarg f);
void granulesf_highblock(t_granulesf *x, t_floatarg f);
void granulesf_events(t_granulesf *x, t_floatarg e);
float granulesf_boundrand(float min, float max);
void *granulesf_grist(t_granulesf *x, t_symbol *msg, short argc, t_atom *argv);
void *granulesf_grain(t_granulesf *x, t_symbol *msg, short argc, t_atom *argv);
void *granulesf_setscale(t_granulesf *x, t_symbol *msg, short argc, t_atom *argv);
void granulesf_info(t_granulesf *x);
void granulesf_mute(t_granulesf *x, t_floatarg toggle);
void granulesf_steady(t_granulesf *x, t_floatarg toggle);
void granulesf_constrain_scale(t_granulesf *x, t_floatarg toggle);
void granulesf_dsp_free(t_granulesf *x);
void granulesf_init(t_granulesf *x,short initialized);
void granulesf_constrain(int *index_min, int *index_max, float min_incr, float max_incr, float *scale, int steps);
void granulesf_interpolate(t_granulesf *x, t_floatarg toggle);
void granulesf_nopan(t_granulesf *x, t_floatarg toggle);
void granulesf_retro_odds(t_granulesf *x, t_floatarg o);
void granulesf_seed(t_granulesf *x, t_floatarg seed);
void granulesf_interpolate_envelope(t_granulesf *x, t_floatarg toggle);


void granulesf_tilde_setup(void){
    granulesf_class = class_new(gensym("granulesf~"), (t_newmethod)granulesf_new,
                                (t_method)granulesf_dsp_free,sizeof(t_granulesf), 0,A_GIMME,0);
    CLASS_MAINSIGNALIN(granulesf_class, t_granulesf, x_f);
    class_addmethod(granulesf_class,(t_method)granulesf_dsp,gensym("dsp"),0);
    class_addmethod(granulesf_class,(t_method)granulesf_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_setbuf,gensym("setbuf"),A_DEFSYM,A_DEFSYM,0);
    class_addmethod(granulesf_class,(t_method)granulesf_spray,gensym("spray"),0);
    class_addmethod(granulesf_class,(t_method)granulesf_info,gensym("info"),0);
    class_addmethod(granulesf_class,(t_method)granulesf_pitchspray,gensym("pitchspray"),0);
    class_addmethod(granulesf_class,(t_method)granulesf_transpose,gensym("transpose"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_events,gensym("events"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_pitchdev,gensym("pitchdev"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_lowblock,gensym("lowblock"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_highblock,gensym("highblock"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_steady,gensym("steady"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_constrain_scale,gensym("constrain_scale"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_grist,gensym("grist"),A_GIMME,0);
    class_addmethod(granulesf_class,(t_method)granulesf_grain,gensym("grain"),A_GIMME,0);
    class_addmethod(granulesf_class,(t_method)granulesf_setscale,gensym("setscale"),A_GIMME,0);
    class_addmethod(granulesf_class,(t_method)granulesf_interpolate,gensym("interpolate"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_nopan,gensym("nopan"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_retro_odds,gensym("retro_odds"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_seed,gensym("seed"),A_FLOAT,0);
    class_addmethod(granulesf_class,(t_method)granulesf_interpolate_envelope,gensym("interpolate_envelope"),A_FLOAT,0);
    potpourri_announce(OBJECT_NAME);
}

void granulesf_interpolate_envelope(t_granulesf *x, t_floatarg toggle)
{
	x->interpolate_envelope = toggle;
}

void granulesf_seed(t_granulesf *x, t_floatarg seed)
{
	srand((long)seed);
}

void granulesf_retro_odds(t_granulesf *x, t_floatarg o)
{
	if(o < 0 || o > 1){
		error("retro odds must be within [0.0 - 1.0]");
		return;
	}
	x->retro_odds = 0;
}

void granulesf_interpolate(t_granulesf *x, t_floatarg toggle)
{
	x->interpolate = toggle;
	post("toggle DACs to change interpolation status");
}

void granulesf_nopan(t_granulesf *x, t_floatarg toggle)
{
	x->nopan = toggle;
}

void granulesf_constrain_scale(t_granulesf *x, t_floatarg toggle)
{
	x->constrain_scale = toggle;
}
void granulesf_lowblock(t_granulesf *x, t_floatarg f)
{
	if(f > 0){
		x->lowblock_increment = f;
	}
}

void granulesf_highblock(t_granulesf *x, t_floatarg f)
{
	if(f > 0){
		x->highblock_increment = f;
	}
}

void granulesf_pitchdev(t_granulesf *x, t_floatarg d)
{
	if(d < 0 ){
		error("pitch deviation must be positive");
		return;
	}
	x->pitch_deviation = d;
}

void granulesf_mute(t_granulesf *x, t_floatarg toggle)
{
	x->mute = toggle;
}

void granulesf_steady(t_granulesf *x, t_floatarg toggle)
{
	x->steady = toggle;
}

void granulesf_events(t_granulesf *x, t_floatarg e)
{
	if( e <= 0 ){
		post("events must be positive!");
		return;
	}
	x->events = e;
}

void granulesf_transpose(t_granulesf *x, t_floatarg t)
{
	if( t <= 0 ){
		error("transpose factor must be greater than zero!");
		return;
	}
	x->transpose = t;
}

void *granulesf_setscale(t_granulesf *x, t_symbol *msg, short argc, t_atom *argv)
{
	int i;
	float *pitchscale = x->pitchscale;
	if( argc >= MAXSCALE ){
		error("%d is the maximum size scale", MAXSCALE);
		return 0;
	}
	if( argc < 2 ){
		error("there must be at least 2 members in scale");
		return 0;
	}
	for(i=0; i < argc; i++){
		pitchscale[i] = atom_getfloatarg(i,argc,argv);
	}
	x->pitchsteps = argc;
    return 0;
}

void granulesf_constrain(int *index_min, int *index_max, float min_incr, float max_incr, float *scale, int steps)
{
	int imax = steps - 1;
	int imin = 0;
	while(scale[imin] < min_incr && imin < imax){
		++imin;
	}
	if(imin == imax){
		post("could not constrain minimum index  - your grist parameters are out of range for this scale");
		*index_min = 0;
		*index_max = steps - 1;
		return;
	}
    while(scale[imax] > max_incr && imax > 0){
		--imax;
	}
	if(imax < 1 || imax <= imin){
		post("could not constrain maximum index - your grist parameters are out of range for this scale");
		*index_min = 0;
		*index_max = steps - 1;
		return;
	}
	*index_min = imin;
	*index_max = imax;
}

void granulesf_pitchspray(t_granulesf *x)
{
	int i,j;
	long grainframes;
	long b_frames = x->wavebuf->b_frames;
	long eframes = x->windowbuf->b_frames;
	long minskip = x->minskip;
	long maxskip = x->maxskip;
	float retro_odds = x->retro_odds;
	long horizon = x->horizon; // length of block for random events
	float mindur = x->mindur;
	float maxdur = x->maxdur;
	float min_incr = x->min_incr; // minimum frequency for a grain
	float max_incr = x->max_incr; // maximum frequency for a grain
	float minpan = x->minpan; // minimum pan for a grain
	float maxpan = x->maxpan; // maxium pan for a grain
	float minamp = x->minamp; // minimum amplitude for a grain
	float maxamp = x->maxamp; // maximum amplitude for a grain
	float transpose = x->transpose; // pitch scalar
	float lowblock_increment = x->lowblock_increment;
	float highblock_increment = x->highblock_increment;
	short steady = x->steady;
	float pitch_deviation = x->pitch_deviation;
	float pdev = 0;
	float pdev_invert = 0;
    //	float pscale;
	float pan;
	int index_min, index_max;
	int steps = x->pitchsteps;
	float *scale = x->pitchscale;
	int windex;
	short inserted = 0;
	short constrain_scale = x->constrain_scale;
	t_grain *grains = x->grains;
	float tmp;
    
	if( steps < 2 ){
		error("scale is undefined");
		return;
	}
	if( pitch_deviation ){
		pdev = 1.0 + pitch_deviation;
		pdev_invert = 1.0 / pdev;
	}
	for( i = 0; i < x->events; i++ ){
		inserted = 0;
		for(j = 0; j < MAXGRAINS; j++ ){
			if(!grains[j].active){
                if(steady){
                    grains[j].delay = (float)(i * horizon) / (float) x->events ;
                } else {
                    grains[j].delay = granulesf_boundrand(0.0,(float) horizon);
    			}
    			grains[j].duration = (long) granulesf_boundrand(mindur, maxdur);
    			grains[j].phase = 0.0;
    			grains[j].ephase = 0.0;
    			pan = granulesf_boundrand(minpan, maxpan);
                
    			grains[j].amplitude = granulesf_boundrand(minamp, maxamp);
    			grains[j].panL = grains[j].amplitude * cos(pan * PIOVERTWO);
    			grains[j].panR = grains[j].amplitude * sin(pan * PIOVERTWO);
    			grains[j].amplitude *= .707;//used directly only for "nopan"
    			
                if(constrain_scale){
                    granulesf_constrain(&index_min,&index_max,min_incr, max_incr, scale, steps);
                    windex = (int) granulesf_boundrand((float)index_min, (float)index_max);
                } else {
                    windex = (int) granulesf_boundrand(0.0, (float)(steps));
                }
    			grains[j].si = transpose * scale[windex];
                //	post("windex %d scale[w] %f transpose %f si %f",windex, scale[windex], transpose, grains[j].si );
    			grainframes = grains[j].duration * grains[j].si;
    			grains[j].esi =  (float) eframes / (float) grains[j].duration;
    			
    			if( pitch_deviation ){
    				grains[j].si *= granulesf_boundrand(pdev_invert,pdev);
    			}
    			// post("new si: %f", grains[j].si);
    			/* must add this code to spray, and also do for high frequencies
                 */
    			if(lowblock_increment > 0.0) {
    				if(grains[j].si < lowblock_increment){
    					post("lowblock: aborted grain with %f frequency",grains[j].si);
    					grains[j].active = 0; // abort grain
    					goto nextgrain;
    				}
    			}
    			if(highblock_increment > 0.0) {
    				if(grains[j].si > highblock_increment){
    					post("highblock: aborted grain with %f frequency, greater than %f",
                             grains[j].si, highblock_increment);
    					grains[j].active = 0; // abort grain
    					goto nextgrain;
    				}
    			}
                /* set skip time into sample */
                if(grainframes >= b_frames ){
				  	error("grain size %.0ld is too long for buffer which is %ld",grainframes, b_frames);
				  	grains[j].active = 0;
				  	goto nextgrain;
                }
                if(minskip > b_frames - grainframes){//bad minskip
				  	error("minskip time is illegal");
				  	grains[j].phase = 0.0;
				  	grains[j].endframe = grainframes - 1;
                } else {
                    if(maxskip > b_frames - grainframes){
					  	grains[j].phase = granulesf_boundrand((float)minskip, (float) (b_frames - grainframes));
					  	//post("1. minskip %d maxskip %d",minskip,b_frames - grainframes);
                    } else {
					  	grains[j].phase = granulesf_boundrand((float)minskip, (float)maxskip);
					  	//post("2. minskip %d maxskip %d",minskip,maxskip);
                    }
                    grains[j].endframe = grains[j].phase + grainframes - 1;
                }
                
                
                if( granulesf_boundrand(0.0,1.0) < retro_odds){//go backwards - make sure to test both boundaries
				  	grains[j].si *= -1.0;
				  	tmp = grains[j].phase;
				  	grains[j].phase = grains[j].endframe;
				  	grains[j].endframe = tmp;
                }
                /*post("grain: grainframes %d phase %f endframe %f amp %f",
                 grainframes, grains[j].phase, grains[j].endframe, grains[j].amplitude);*/
                grains[j].active = 1;
    			inserted = 1;
    			goto nextgrain;
    		}
		}
		if(!inserted){
			error("could not insert grain with increment %f",grains[j].si);
			return;
		}
    nextgrain: ;
	}
}

void granulesf_spray(t_granulesf *x)
{
	int i,j;
	long grainframes;
	long eframes = x->windowbuf->b_frames;
	long b_frames = x->wavebuf->b_frames;
    //	long b_nchans = x->wavebuf->b_nchans;
    //	float sr = x->sr;
	long horizon = x->horizon; // length of block for random events
	float mindur = x->mindur;
	float maxdur = x->maxdur;
	float min_incr = x->min_incr; // minimum incr for a grain (must be positive!)
	float max_incr = x->max_incr; // maximum incr for a grain (must be positive!)
	float minpan = x->minpan; // minimum pan for a grain
	float maxpan = x->maxpan; // maxium pan for a grain
	float minamp = x->minamp; // minimum amplitude for a grain
	float maxamp = x->maxamp; // maximum amplitude for a grain
	float transpose = x->transpose; // pitch scalar
	long minskip = x->minskip;
	long maxskip = x->maxskip;
	short steady = x->steady;
	float retro_odds = x->retro_odds;
	float pan;
	t_grain *grains = x->grains;
	short inserted;
	float tmp;

	for( i = 0; i < x->events; i++ ){
		inserted = 0;
		for(j = 0; j < MAXGRAINS; j++ ){
			if(!grains[j].active){
				grains[j].active = 1;
				if(steady){
					grains[j].delay = (float)(i * horizon) / (float) x->events ;
				} else {
    				grains[j].delay = granulesf_boundrand(0.0,(float) horizon);
    			}
   				grains[j].duration = (long) granulesf_boundrand(mindur, maxdur);//frames for this grain
    			grains[j].ephase = 0.0;
    			pan = granulesf_boundrand(minpan, maxpan);
     			grains[j].amplitude = granulesf_boundrand(minamp, maxamp);
    			grains[j].panL = grains[j].amplitude * cos(pan * PIOVERTWO);
    			grains[j].panR = grains[j].amplitude * sin(pan * PIOVERTWO);
    			grains[j].amplitude *= .707;//used directly only for "nopan"
                grains[j].si = transpose * granulesf_boundrand(min_incr, max_incr);
                
                grainframes = grains[j].duration * grains[j].si;//frames to be read from buffer
                // grains[j].esi =  (float) eframes / (float) grainframes;
                grains[j].esi =  (float) eframes / (float) grains[j].duration;
                if(grainframes >= b_frames ){
				  	error("grain size %.0ld is too long for buffer which is %ld",grainframes, b_frames);
				  	grains[j].active = 0;
				  	goto nextgrain;
                }
                if(minskip > b_frames - grainframes){//bad minskip
				  	error("minskip time is illegal");
				  	grains[j].phase = 0.0;
				  	grains[j].endframe = grainframes - 1;
                } else {
                    if(maxskip > b_frames - grainframes){
					  	grains[j].phase = granulesf_boundrand((float)minskip, (float) (b_frames - grainframes));
					  	//post("1. minskip %d maxskip %d",minskip,b_frames - grainframes);
                    } else {
					  	grains[j].phase = granulesf_boundrand((float)minskip, (float)maxskip);
					  	//post("2. minskip %d maxskip %d",minskip,maxskip);
                    }
                    grains[j].endframe = grains[j].phase + grainframes - 1;
                }
                
                if( granulesf_boundrand(0.0,1.0) < retro_odds){//go backwards - make sure to test both boundaries
				  	grains[j].si *= -1.0;
				  	tmp = grains[j].phase;
				  	grains[j].phase = grains[j].endframe;
				  	grains[j].endframe = tmp;
                }
    			inserted = 1;
                /*	post("startframe %f endframe %f increment %f e-incr %f grainframes %d bframes %d",
                 grains[j].phase,grains[j].endframe,grains[j].si,grains[j].esi,grainframes, b_frames);*/
    			
    			goto nextgrain;
    		}
		}
		if(! inserted){
			error("granulesf~: could not insert grain");
			return;
		}
    nextgrain: ;
	}
}

void *granulesf_grain(t_granulesf *x, t_symbol *msg, short argc, t_atom *argv)
{
	short inserted;
	int j;
	float duration, incr, amplitude, pan;
	t_grain *grains;
	long eframes;
	long frames;
	float sr;
	float skip;

	grains = x->grains;
	eframes = x->windowbuf->b_frames;
	frames = x->wavebuf->b_frames;
	sr = x->sr;
	
	if(argc < 5){
		error("grain takes 5 arguments, not %d",argc);
		post("duration increment amplitude pan skip(in ms)");
		return 0;
	}
	duration = atom_getintarg(0,argc,argv);
	incr = atom_getfloatarg(1,argc,argv); // in ms
	amplitude = atom_getfloatarg(2,argc,argv);
	pan = atom_getfloatarg(3,argc,argv);
	skip = atom_getfloatarg(4,argc,argv) * .001 * sr;
	if(skip < 0){
		error("negative skip is illegal");
		return 0;
	}
	if(skip >= frames){
		error("skip exceeds length of buffer");
		return 0;
	}
	if(incr == 0.0){
		error("zero increment prohibited");
		return 0;
	}
	if(duration <= 0.0){
		error("illegal duration:%f",duration);
		return 0;
	}
	if(pan < 0.0 || pan > 1.0){
		error("illegal pan:%f",pan);
		return 0;
	}
    inserted = 0;
    for(j = 0; j < MAXGRAINS; j++ ){
        if(!grains[j].active){
            grains[j].delay = 0.0;// immediate deployment
            grains[j].duration = (long) (.001 * x->sr * duration);
            grains[j].phase = skip;
            grains[j].ephase = 0.0;
            grains[j].amplitude = amplitude * .707;
            grains[j].panL = amplitude * cos(pan * PIOVERTWO);
            grains[j].panR = amplitude * sin(pan * PIOVERTWO);
            grains[j].esi =  (float)eframes / (float)grains[j].duration;
            grains[j].si = incr;
            grains[j].active = 1;
            return 0;
        }
    }
    
    error("could not insert grain");
    return 0;
    
}

float granulesf_boundrand(float min, float max)
{
	return min + (max-min) * ((float) (rand() % RAND_MAX)/ (float) RAND_MAX);
}


void *granulesf_new(t_symbol *msg, short argc, t_atom *argv)
{

    t_granulesf *x = (t_granulesf *)pd_new(granulesf_class);
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->wavebuf = (t_pdbuffer*)malloc(sizeof(t_pdbuffer));
    x->windowbuf = (t_pdbuffer*)malloc(sizeof(t_pdbuffer));
	srand(time(0)); //need "seed" message
    
	x->pitchscale = (float *) t_getbytes(MAXSCALE * sizeof(float));
	x->grains = (t_grain *) t_getbytes(MAXGRAINS * sizeof(t_grain));
	
    
	// default names
	x->wavename = gensym("waveform");
	x->windowname = gensym("window");

    
    // apparently Pd lacks this Max/MSP bug
	x->wavename = atom_getsymbolarg(0,argc,argv);
	x->windowname = atom_getsymbolarg(1,argc,argv);

    
	x->sr = sys_getsr();
	if(! x->sr )
		x->sr = 44100;
    
	granulesf_init(x,0);
    
    
    return (x);
}

void granulesf_init(t_granulesf *x,short initialized)
{
	int i;
	
	if(!initialized){
		x->pitchsteps = 0; // we could predefine a 12t scale
		x->mute = 0;
		x->steady = 0;
		x->events = 1; // set to 10 LATER
		x->horizon_ms = 1000;
		x->min_incr = 0.5;
		x->max_incr = 2.0;
		x->minpan = .1;
		x->maxpan = .9;
		x->minamp = .1;
		x->maxamp = 1.0;
		x->mindur_ms = 150;
		x->maxdur_ms = 750;
		x->transpose = 1.0;
		x->pitch_deviation = 0.0;
		x->lowblock_increment = 0.0; // by default we do not block any increments
		x->highblock_increment = 0.0; // ditto
		x->constrain_scale = 0;
		x->retro_odds = 0.5;// after testing, set this to zero
		x->maxskip = -1;//flag to reset in setbuf SHOULD BE -1
		x->nopan = 0;//panning is on by default
		x->interpolate = 1;
		x->interpolate_envelope = 0;
	}
	x->horizon = x->horizon_ms * .001 * x->sr;
	x->mindur = x->mindur_ms * .001 * x->sr;
	x->maxdur = x->maxdur_ms * .001 * x->sr;
	for( i = 0; i < MAXGRAINS; i++ ){ // this is what we test for a legal place to insert grain
		x->grains[i].active = 0;
	}
}

void granulesf_info(t_granulesf *x)
{
	int tcount = 0;
	t_grain *grains = x->grains;
    //	long eframes = x->windowbuf->b_frames;
	int i;
	
	for(i = 0; i < MAXGRAINS; i++ ){
		if(grains[i].active)
			++tcount;
	}
	post("%d active grains", tcount);
	post("wavename %s", x->wavename->s_name);
	post("windowname %s", x->windowname->s_name);
	post("sample size: %d",x->wavebuf->b_frames);
}


void *granulesf_grist(t_granulesf *x, t_symbol *msg, short argc, t_atom *argv)
{
	if(argc < 10 ){
		error("grist takes 10 arguments:");
		post("events horizon min_incr max_incr minpan maxpan minamp maxamp mindur maxdur");
		return 0;
	}
	x->events = atom_getintarg(0,argc,argv);
	x->horizon_ms = atom_getfloatarg(1,argc,argv);
	x->min_incr = atom_getfloatarg(2,argc,argv);
	x->max_incr = atom_getfloatarg(3,argc,argv);
	x->minpan = atom_getfloatarg(4,argc,argv);
	x->maxpan = atom_getfloatarg(5,argc,argv);
	x->minamp = atom_getfloatarg(6,argc,argv);
	x->maxamp = atom_getfloatarg(7,argc,argv);
	x->mindur_ms = atom_getfloatarg(8,argc,argv);
	x->maxdur_ms = atom_getfloatarg(9,argc,argv);
	
	x->mindur = .001 * x->sr * x->mindur_ms ;
	x->maxdur = .001 * x->sr * x->maxdur_ms;
	x->horizon = .001 * x->sr * x->horizon_ms;
	
	if(x->min_incr < 0){
		x->min_incr *= -1.0;
	}
	if(x->max_incr < 0){
		x->max_incr *= -1.0;
	}
	if(x->minpan < 0.0) {
		x->minpan = 0.0;
	}
	if(x->maxpan > 1.0) {
		x->maxpan = 1.0;
	}
	if(x->events < 0){
		x->events = 0;
	}
	return 0;
}


void granulesf_reload(t_granulesf *x)
{
	granulesf_setbuf(x, x->wavename, x->windowname);
}


void granulesf_setbuf(t_granulesf *x, t_symbol *wavename, t_symbol *windowname)
{
    t_garray *a;
    int frames;
    
    x->hosed = 0;
    x->wavebuf->b_frames = 0;
    x->windowbuf->b_frames = 0;
    x->wavebuf->b_nchans = 1;//unused, should kill
    x->windowbuf->b_nchans = 1;  //unused, should kill
    x->b_nchans = 1;
    
    /* load up sample array */
    if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class))) {
        if (*wavename->s_name) pd_error(x, "granulesf~: %s: no such array", wavename->s_name);
        x->hosed = 1;
    }
    else if (!garray_getfloatwords(a, &frames, &x->wavebuf->b_samples)) {
        pd_error(x, "%s: bad template for granulesf~", wavename->s_name);
        x->hosed = 1;
    }
    else  {
        x->wavebuf->b_frames = frames;
        x->b_nchans = 1; // Pd buffers are always mono (so far)
        garray_usedindsp(a);
    }
    
    /* load up envelope array*/
    if (!(a = (t_garray *)pd_findbyclass(windowname, garray_class))) {
        if (*wavename->s_name) pd_error(x, "granulesf~: %s: no such array", windowname->s_name);
        x->hosed = 1;
    }
    else if (!garray_getfloatwords(a, &frames, &x->windowbuf->b_samples)) {
        pd_error(x, "%s: bad template for granulesf~", windowname->s_name);
        x->hosed = 1;
    }
    else  {
        x->windowbuf->b_frames = frames;
        garray_usedindsp(a);
    }
    
    x->maxskip = x->wavebuf->b_frames - 1;
}


t_int *granulesf_performhose(t_int *w)
{
    //	t_granulesf *x = (t_granulesf *) (w[1]);
	float *outputL = (t_float *)(w[3]);
	float *outputR = (t_float *)(w[4]);
	int n = (int) w[5];
	while(n--) *outputL++ = *outputR++ = 0;
	return (w+6);
}

t_int *granulesf_perform_no_interpolation(t_int *w)
{
	t_granulesf *x = (t_granulesf *) (w[1]);
    //	float *in = (t_float *)(w[2]); // ignoring input
	float *outputL = (t_float *)(w[3]);
	float *outputR = (t_float *)(w[4]);
	int n = (int) w[5];

	t_pdbuffer *wavebuf = x->wavebuf;
	t_pdbuffer *windowbuf = x->windowbuf;
	t_word *wavetable = wavebuf->b_samples;
	t_word *window = windowbuf->b_samples;
	t_grain *grains = x->grains;
	long b_nchans = x->b_nchans;
	long b_frames = wavebuf->b_frames;
	short nopan = x->nopan;
	float sample1, sample2;
	float envelope;
	float amplitude;
	float panL, panR;
	float si;
	float esi;
	float phase;
	float ephase;
	long delay;
	long eframes = windowbuf->b_frames;
	long current_index;
	int i,j;
	
    
	/* grain parameters */
    
    
	if( x->mute ){
		while(n--) *outputL++ = *outputR++ = 0;
		return (w+6);
	}
    
    // pre-clean buffer
	for( i = 0; i < n; i++ ){
		outputL[i] = outputR[i] = 0;
	}
    
	for (j=0; j<MAXGRAINS; j++) {
        
		if(!grains[j].active){
			goto nextgrain;
		}
		amplitude = grains[j].amplitude;
		si =  grains[j].si;
		esi = grains[j].esi;
		phase =  grains[j].phase;
		ephase = grains[j].ephase;
		delay =  grains[j].delay;
		panL = grains[j].panL;
		panR = grains[j].panR;
        
		
		for(i = 0; i < n; i++ ){
			if( delay > 0 ){
				--delay;
			}
			if( delay <= 0 && ephase < eframes){
                
				envelope = amplitude * window[(int)ephase].w_float;// interpolate please!
				
				if(b_nchans == 1){
					sample1 = wavetable[(int)phase].w_float;
					sample1 *= envelope;
					if(nopan){
						sample1 *= amplitude; // center it
						outputL[i] += sample1;
						outputR[i] += sample1;
					} else {
						outputL[i] += panL * sample1;
						outputR[i] += panR * sample1;
					}
				} else if(b_nchans == 2){
                    if(phase < 0 || phase >= b_frames){
                        error("phase %f is out of bounds",phase);
                        goto nextgrain;
                    }
					current_index = (long)(phase * 2.0);
					sample1 = wavetable[current_index].w_float;
					sample2 = wavetable[current_index+1].w_float;
					sample1 *= envelope;
					sample2 *= envelope;
					if(nopan){
						outputL[i] += sample1 * amplitude;
						outputR[i] += sample2 * amplitude;
					} else {
						outputL[i] += panL * sample1;
						outputR[i] += panR * sample2;
					}
				}
                
                /*this will be gritty so be sure to interpolate both envelope and sample-lookup */
				
                
				phase += si;
				if(phase < 0 || phase >= b_frames){
					error("phase %f out of bounds",phase);
					grains[j].active = 0;
					goto nextgrain;
				}
				ephase += esi;
                
                
				if( ephase >= eframes ){
					grains[j].active = 0;
					goto nextgrain; // must escape loop now
				}
                
			}
		}
		grains[j].phase = phase;
		grains[j].ephase = ephase;
		grains[j].delay = delay;
		
    nextgrain: ;
	}
    
	return (w+6);
}

t_int *granulesf_perform(t_int *w)
{
	t_granulesf *x = (t_granulesf *) (w[1]);
    //	float *in = (t_float *)(w[2]); // ignoring input
	float *outputL = (t_float *)(w[3]);
	float *outputR = (t_float *)(w[4]);
	int n = (int) w[5];

	t_pdbuffer *wavebuf = x->wavebuf;
	t_pdbuffer *windowbuf = x->windowbuf;
	t_word *wavetable = wavebuf->b_samples;
	t_word *window = windowbuf->b_samples;
	t_grain *grains = x->grains;
	long b_nchans = x->b_nchans;
	long b_frames = wavebuf->b_frames;
	short nopan = x->nopan;
	short interpolate_envelope = x->interpolate_envelope;
	float sample1, sample2;
	float envelope;
	float amplitude;
	float panL, panR;
	float si;
	float esi;
	float phase;
	float ephase;
	long delay;
	long eframes = windowbuf->b_frames;
	long current_index;
	float tsmp1, tsmp2;
	float frac;
	int i,j;
	
	if( x->mute ){
		while(n--) *outputL++ = *outputR++ = 0;
		return (w+6);
	}
    
    // pre-clean buffer
	for( i = 0; i < n; i++ ){
		outputL[i] = outputR[i] = 0;
	}
    
	for (j=0; j<MAXGRAINS; j++) {
        
		if(!grains[j].active){
			goto nextgrain;
		}
		amplitude = grains[j].amplitude;
		si =  grains[j].si;
		esi = grains[j].esi;
		phase =  grains[j].phase;
		ephase = grains[j].ephase;
		delay =  grains[j].delay;
		panL = grains[j].panL;
		panR = grains[j].panR;
        
		
		for(i = 0; i < n; i++ ){
			if( delay > 0 ){
				--delay;
			}
			if( delay <= 0 && ephase < eframes){
                
                if(interpolate_envelope){
                    current_index = floor((double)ephase);
                    frac = ephase - current_index;
                    if(current_index == 0 || current_index == eframes - 1 || frac == 0.0){// boundary conditions
                        envelope = window[current_index].w_float;
                    } else {
                        tsmp1 = window[current_index].w_float;
                        tsmp2 = window[current_index + 1].w_float;
                        envelope = tsmp1 + frac * (tsmp2 - tsmp1);
                    }
                } else {
					// envelope = amplitude * window[(int)ephase];
					envelope = window[(int)ephase].w_float;// amplitude built into panL and panR
				}
				if(b_nchans == 1){
                    if(phase < 0 || phase >= b_frames){
                        error("phase %f is out of bounds",phase);
                        goto nextgrain;
                    }
                    current_index = floor((double)phase);
                    frac = phase - current_index;
                    if(current_index == 0 || current_index == b_frames - 1 || frac == 0.0){// boundary conditions
                        sample1 = wavetable[current_index].w_float;
                    } else {
                        tsmp1 = wavetable[current_index].w_float;
                        tsmp2 = wavetable[current_index + 1].w_float;
                        sample1 = tsmp1 + frac * (tsmp2 - tsmp1);
                    }
					sample1 *= envelope;
					if(nopan){
						sample1 *= amplitude; // center it
						/* accumulate grain samples into output buffer */
						outputL[i] += sample1;
						outputR[i] += sample1;
					} else {
						outputL[i] += panL * sample1;
						outputR[i] += panR * sample1;
					}
				} else if(b_nchans == 2){
                    if(phase < 0 || phase >= b_frames){
                        error("phase %f is out of bounds",phase);
                        goto nextgrain;
                    }
                    current_index = floor((double)phase);
                    frac = phase - current_index;
                    current_index <<= 1;// double it
					if(current_index == 0 || current_index == b_frames - 1 || frac == 0.0){
						sample1 = wavetable[current_index].w_float;
						sample2 = wavetable[current_index+1].w_float;
					} else {
                        tsmp1 = wavetable[current_index].w_float;
                        tsmp2 = wavetable[current_index + 2].w_float;
                        sample1 = tsmp1 + frac * (tsmp2 - tsmp1);
                        tsmp1 = wavetable[current_index + 1].w_float;
                        tsmp2 = wavetable[current_index + 3].w_float;
                        sample2 = tsmp1 + frac * (tsmp2 - tsmp1);
					}
                    
					sample1 *= envelope;
					sample2 *= envelope;
					
					if(nopan){
						outputL[i] += sample1 * amplitude;
						outputR[i] += sample2 * amplitude;
					} else {
						outputL[i] += panL * sample1;
						outputR[i] += panR * sample2;
					}
				}
				
				phase += si;
				if(phase < 0 || phase >= b_frames){
					error("phase %f out of bounds",phase);
					grains[j].active = 0;
					goto nextgrain;
				}
				ephase += esi;
                
                
				if( ephase >= eframes ){
					grains[j].active = 0;
					goto nextgrain; // must escape loop now
				}
                
			} 
		} 		
		grains[j].phase = phase;
		grains[j].ephase = ephase;
		grains[j].delay = delay;
		
    nextgrain: ;
	}
	return (w+6);
}

void granulesf_dsp_free(t_granulesf *x)
{
	t_freebytes(x->grains, MAXGRAINS * sizeof(t_grain));
	t_freebytes(x->pitchscale, MAXSCALE * sizeof(float));
}

void granulesf_dsp(t_granulesf *x, t_signal **sp)
{
    
	granulesf_reload(x);
	
	if( x->hosed ){
		post("You need some valid buffers");
		dsp_add(granulesf_performhose, 5, x, 
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, (t_int)sp[0]->s_n);
		return;
	}
	if( x->sr != sp[0]->s_sr){
		x->sr = sp[0]->s_sr;
		if( !x->sr ){
			post("warning: zero sampling rate!");
			x->sr = 44100;
		}
		granulesf_init(x,1);
	} 
	if(x->interpolate){
		dsp_add(granulesf_perform, 5, x, 
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, (t_int)sp[0]->s_n);
	} else {
		dsp_add(granulesf_perform_no_interpolation, 5, x, 
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, (t_int)sp[0]->s_n);
	}
}

