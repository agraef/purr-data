#include "MSPd.h"


static t_class *samm_class;

#define MAXBEATS (256)
#define OBJECT_NAME "samm~"
// #define DATE "prerelease"

typedef struct _samm
{
	t_object x_obj;
	float x_f;
	double tempo; /* current tempo */
	double onebeat_samps; /* number of samples for a single beat */
	double *beats; /* amount of beats for each active tempo outlet */
	double *metro_samps;/* number of samples to count down for each time interval */
	double *metro_beatdurs;/* number of beats for each metro time interval */
	double *metro;/* current countdown for each time interval */
	int metro_count; /* number of metronomes to keep track of */
	float sr; /* current sampling rate */
	short pause;
	short mute;
    // Pd only */
	float *trigger_vec;
	short vs;
} t_samm;

void *samm_new(t_symbol *msg, short argc, t_atom *argv);
void *samm_beatlist(t_samm *x, t_symbol *msg, short argc, t_atom *argv);
t_int *samm_perform(t_int *w);
void samm_dsp(t_samm *x, t_signal **sp);
void samm_tempo(t_samm *x, t_floatarg f);
void samm_divbeats(t_samm *x, t_symbol *msg, short argc, t_atom *argv);
void samm_msbeats(t_samm *x, t_symbol *msg, short argc, t_atom *argv);
void samm_sampbeats(t_samm *x, t_symbol *msg, short argc, t_atom *argv);
void samm_ratiobeats(t_samm *x, t_symbol *msg, short argc, t_atom *argv);
void samm_free(t_samm *x);
void samm_beatinfo(t_samm *x);
void samm_init(t_samm *x,short initialized);
void samm_mute(t_samm *x, t_floatarg f);
void samm_pause(t_samm *x);
void samm_arm(t_samm *x);
void samm_resume(t_samm *x);
void samm_beats(t_samm *x, t_symbol *msg, short argc, t_atom *argv);


void samm_beatinfo(t_samm *x)
{
	int i;
	post("tempo %.10f",x->tempo);
	post("samples in one beat: %.10f",x->onebeat_samps);
	for(i = 0; i < x->metro_count; i++){
		post("%d: relative duration %.10f, samples %.10f samples ratio to 1 beat: %.10f", i,
             x->metro_beatdurs[i], x->metro_samps[i], x->onebeat_samps / x->metro_samps[i]  );
	}
}

void samm_tilde_setup(void){
	samm_class = class_new(gensym("samm~"), (t_newmethod)samm_new,
						   (t_method)samm_free,sizeof(t_samm), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(samm_class, t_samm, x_f);
	class_addmethod(samm_class,(t_method)samm_dsp,gensym("dsp"),0);
	class_addmethod(samm_class,(t_method)samm_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(samm_class,(t_method)samm_tempo,gensym("tempo"),A_FLOAT,0);
	class_addmethod(samm_class,(t_method)samm_beatinfo,gensym("beatinfo"),0);
	class_addmethod(samm_class,(t_method)samm_beats,gensym("beats"),A_GIMME,0);
	class_addmethod(samm_class,(t_method)samm_divbeats,gensym("divbeats"),A_GIMME,0);
	class_addmethod(samm_class,(t_method)samm_msbeats,gensym("msbeats"),A_GIMME,0);
	class_addmethod(samm_class,(t_method)samm_sampbeats,gensym("sampbeats"),A_GIMME,0);
	class_addmethod(samm_class,(t_method)samm_ratiobeats,gensym("ratiobeats"),A_GIMME,0);
	class_addmethod(samm_class,(t_method)samm_pause,gensym("pause"),A_GIMME,0);
	class_addmethod(samm_class,(t_method)samm_arm,gensym("arm"),A_GIMME,0);
	class_addmethod(samm_class,(t_method)samm_resume,gensym("resume"),A_GIMME,0);
    potpourri_announce(OBJECT_NAME);
}



void samm_pause(t_samm *x)
{
	x->pause = 1;
}

void samm_mute(t_samm *x, t_floatarg f)
{
	x->mute = (short) f;
}

void samm_arm(t_samm *x)
{
	int i;
	
	x->pause = 1;
	for( i = 0; i < x->metro_count; i++){
		x->metro[i] = 1.0;
	}
	
}

void samm_resume(t_samm *x)
{
	x->pause = 0;
}

void samm_beats(t_samm *x, t_symbol *msg, short argc, t_atom *argv)
{
    int i;
    double beatdur;
    
	if(argc != x->metro_count){
		error("%s: arguments did not match metro count %d",OBJECT_NAME,x->metro_count);
		return;
	}
    
	for(i = 0; i < argc; i++){
		beatdur = (double)atom_getfloatarg(i,argc,argv);
    	if(!beatdur){
			error("%s: zero divisor given for beat stream %d",OBJECT_NAME,i+1);
			beatdur = 1.0;
		}
        
		x->metro_beatdurs[i] = beatdur;
		x->metro_samps[i] = x->metro_beatdurs[i] * x->onebeat_samps;
		x->metro[i] = 1.0; // initialize for instantaneous beat
	}
}

void samm_divbeats(t_samm *x, t_symbol *msg, short argc, t_atom *argv)
{
    int i;
    double divisor;
    
	if(argc != x->metro_count){
		error("%s: arguments did not match metro count %d",OBJECT_NAME,x->metro_count);
		return;
	}
    
	for(i = 0; i < argc; i++){
		divisor = (double)atom_getfloatarg(i,argc,argv);
    	if(!divisor){
			error("%s: zero divisor given for beat stream %d",OBJECT_NAME,i+1);
			divisor = 1.0;
		}
        
		x->metro_beatdurs[i] = 1.0 / divisor; // argument is now DIVISOR of beat
		x->metro_samps[i] = x->metro_beatdurs[i] * x->onebeat_samps;
		x->metro[i] = 1.0; // initialize for instantaneous beat
	}
}

void samm_msbeats(t_samm *x, t_symbol *msg, short argc, t_atom *argv)
{
    int i;
    double msecs;
	if(argc != x->metro_count){
		error("%s: arguments did not match metro count %d",OBJECT_NAME,x->metro_count);
		return;
	}
	for(i = 0; i < argc; i++){
		msecs = (double)atom_getfloatarg(i,argc,argv);
    	if(msecs <= 0){
			error("%s: illegal duration for beat stream %d",OBJECT_NAME,i+1);
			msecs = 1000.0;
		}
		x->metro_samps[i] = x->sr * .001 * msecs;
		x->metro_beatdurs[i] = x->metro_samps[i] / x->onebeat_samps; // just in case tempo changes
		x->metro[i] = 1.0; // initialize for instantaneous beat
	}
    
}

void samm_sampbeats(t_samm *x, t_symbol *msg, short argc, t_atom *argv)
{
    int i;
    double samples;
	if(argc != x->metro_count){
		error("%s: arguments did not match metro count %d",OBJECT_NAME,x->metro_count);
		return;
	}
	for(i = 0; i < argc; i++){
		samples = (double)atom_getfloatarg(i,argc,argv);
    	if(samples <= 0){
			error("%s: illegal duration for beat stream %d",OBJECT_NAME,i+1);
			samples = x->sr;
		}
		x->metro_samps[i] = samples;
		x->metro_beatdurs[i] = x->metro_samps[i] / x->onebeat_samps; // just in case tempo changes
		x->metro[i] = 1.0; // initialize for instantaneous beat
	}
}

void samm_ratiobeats(t_samm *x, t_symbol *msg, short argc, t_atom *argv)
{
    int i,j;
    double num,denom;
    
	if(argc != x->metro_count * 2){
		error("%s: arguments did not match metro count %d",OBJECT_NAME,x->metro_count);
		return;
	}
    
	for(i = 0, j= 0; i < argc; i += 2, j++){
		num = (double)atom_getfloatarg(i,argc,argv);
		denom = (double)atom_getfloatarg(i+1,argc,argv);
    	if(!denom){
			error("%s: zero divisor given for beat stream %d",OBJECT_NAME,(i/2)+1);
			denom = 1.0;
		}
        
		x->metro_beatdurs[j] = 4.0 * (num / denom);
        //		post("beat duration %f",4.0 * (num/denom));
		x->metro_samps[j] = x->metro_beatdurs[j] * x->onebeat_samps;
		x->metro[j] = 1.0; // initialize for instantaneous beat
	}
}

void samm_tempo(t_samm *x, t_floatarg f)
{
	int i;
	double last_tempo;
	double tempo_fac;
	
	if( f <= 0.0) {
		error("illegal tempo: %f", f);
		return;
	}
	last_tempo = x->tempo;
	x->tempo = f;
	tempo_fac = last_tempo / x->tempo; // shrink or stretch factor for beats
	x->onebeat_samps = (60.0/x->tempo) * x->sr;
	for(i = 0; i < x->metro_count; i++){
		x->metro_samps[i] = x->metro_beatdurs[i] * x->onebeat_samps;
		x->metro[i] *= tempo_fac;
	}
}


void *samm_new(t_symbol *msg, short argc, t_atom *argv)
{
	int i,j, default_tempo = 120;
	double divisor;
	t_samm *x;
    t_atom sane_defaults[2];
	
    if(argc < 2){
        // If no args, set some sane defaults
        if (!argc){
            post("%s: warning: no tempo or beat streams provided: setting "
                 "tempo to 120 BPM and 1 stream to '1'",OBJECT_NAME);
            SETFLOAT(sane_defaults, default_tempo);
            SETFLOAT(sane_defaults+1, 1.);
            argc = 2;
            argv = sane_defaults;
        }
        else
        {
            error("%s: there must be at least 1 beat stream",OBJECT_NAME);
            return (void *)NULL;
        }
    }
    if(argc > MAXBEATS + 1)
    {
        error("%s: exceeded maximum of %d beat values",OBJECT_NAME, MAXBEATS);
        return (void *)NULL;
    }
    
    x = (t_samm *)pd_new(samm_class);
    x->metro_count = argc - 1;
    for(i=0;i< x->metro_count;i++)
        outlet_new(&x->x_obj, gensym("signal"));
	x->sr = sys_getsr();
	x->vs = sys_getblksize();
    
    x->pause = 0;
    x->mute = 0;
    
	x->beats = (double *) calloc(x->metro_count, sizeof(double));
	x->metro_samps = (double *) calloc(x->metro_count, sizeof(double));
	x->metro_beatdurs = (double *) calloc(x->metro_count, sizeof(double));
	x->metro = (double *) calloc(x->metro_count, sizeof(double));
    
	if(! x->sr ){
		x->sr = 44100;
		post("sr autoset to 44100");
	}
	
	if(argc > 0) {
		x->tempo = (double) atom_getfloatarg(0,argc,argv);
	}
    
	if( x->tempo <= 0.0 ){
		x->tempo = default_tempo;
		post("tempo autoset to %d BPM", default_tempo);
	}
	
	x->onebeat_samps = (60.0/x->tempo) * x->sr;
	
    
    
    
	for(i = 1,j = 0; i < argc; i++, j++){
        divisor = (double)atom_getfloatarg(i,argc,argv);
    	if(!divisor){
			error("%s: zero divisor given for beat stream %d",OBJECT_NAME,i);
			divisor = 1.0;
		}
        
		x->metro_beatdurs[j] = 1.0 / divisor; // argument is now DIVISOR of beat
		x->metro_samps[j] = x->metro_beatdurs[j] * x->onebeat_samps;
		x->metro[j] = 1.0; // initialize for instantaneous beat
		
	}
    //    post("there are %d beat streams",x->metro_count);
    samm_init(x,0);
    
    return (x);
}

void samm_free(t_samm *x)
{
    free(x->trigger_vec);
    free(x->beats);
    free(x->metro_samps);
    free(x->metro_beatdurs);
    free(x->metro);
    
}
void samm_init(t_samm *x,short initialized)
{
    if(!initialized){
        x->trigger_vec = (float*)calloc(x->vs, sizeof(float));
        
        
    } else {
        x->trigger_vec = (float*) realloc(x->trigger_vec,x->vs*sizeof(float));
    }
}


t_int *samm_perform(t_int *w)
{
	int i, j, k;
    //	float outval;
	t_samm *x = (t_samm *) (w[1]);
	t_float *inlet = (t_float *) (w[2]);
	t_float *beat_outlet;
	
	int n;
	
	int metro_count = x->metro_count;
	double *metro = x->metro;
	float *trigger_vec = x->trigger_vec;
	short pause = x->pause;
    
    n = (int) w[metro_count + 3];
    
    if(x->mute){
		for(i = 0, j=3; i < metro_count; i++, j++){
			beat_outlet = (t_float *) (w[j]);
			for(k=0; k < n; k++){
				beat_outlet[k] = 0.0; // use memset
			}
		}
		return (w + metro_count + 4);
    }
    /* main loop */
	for(i=0;i<n;i++)
		trigger_vec[i] = inlet[i];
    
	for(i = 0, j=3; i < metro_count; i++, j++){
		beat_outlet = (t_float *) (w[j]);
		for(k = 0; k < n; k++){
			if(trigger_vec[k]) {
				metro[i] = 1.0; // instant reset from an impulse
			}
			
			beat_outlet[k] = 0.0;
			if(! pause ){
				metro[i] -= 1.0;
				if( metro[i] <= 0){
					beat_outlet[k] = 1.0;
					metro[i] += x->metro_samps[i];
				}
			}
		}
	}
    
	return (w + metro_count + 4);
}


void samm_dsp(t_samm *x, t_signal **sp)
{
	long i;
    t_int **sigvec;
    int pointer_count;
    
    pointer_count = x->metro_count + 3; // all metros, plus 1 inlet, plus the object pointer, plus N
	
	if(x->vs != sp[0]->s_n){
		x->vs = sp[0]->s_n;
		samm_init(x,1);
	}
	if(x->sr != sp[0]->s_sr) {
		x->sr = sp[0]->s_sr;
		x->onebeat_samps = (60.0/x->tempo) * x->sr;
		for(i = 0; i < x->metro_count; i++){
			x->metro_samps[i] = x->metro_beatdurs[i] * x->onebeat_samps;
			x->metro[i] = 0;
		}
	}
	sigvec  = (t_int **) calloc(pointer_count, sizeof(t_int *));
	for(i = 0; i < pointer_count; i++){
		sigvec[i] = (t_int *) calloc(sizeof(t_int),1);
	}
	sigvec[0] = (t_int *)x; // first pointer is to the object
    
	sigvec[pointer_count - 1] = (t_int *)sp[0]->s_n; // last pointer is to vector size (N)
	
	for(i = 1; i < pointer_count - 1; i++){ // now attach the inlet and all outlets
		sigvec[i] = (t_int *)sp[i-1]->s_vec;
	}
    //	post("attached %d pointers",pointer_count);

	dsp_addv(samm_perform, pointer_count, (t_int *) sigvec);     
    
	free(sigvec);
    
}

