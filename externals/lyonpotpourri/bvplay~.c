#include "MSPd.h"

static t_class *bvplay_class;


#define OBJECT_NAME "bvplay~"
typedef struct {
	t_word *b_samples;
	long b_valid;
	long b_nchans;
	long b_frames;
} t_guffer; // stuff we care about from garrays and buffers


typedef struct _bvplay
{
    
    t_object x_obj;
	float x_f;
    t_symbol *sfname; // name of soundfile
	t_guffer *wavebuf; // store needed buffer or garray data
    
	long object_chans; // number of channels for a given instantiation
    float taper_dur;
    int R;
    int framesize;
    float *notedata;
    int active;
    float buffer_duration;
    int taper_frames;
    float amp;
    int start_frame;
    int note_frames;
    int end_frame;
    float increment;
    float findex;
    int index ;
	short verbose;
	short mute;
} t_bvplay;

t_int *bvplay_perform_mono(t_int *w);
t_int *bvplay_perform_stereo(t_int *w);
void bvplay_dsp(t_bvplay *x, t_signal **sp);
void bvplay_set(t_bvplay *x, t_symbol *s);
void *bvplay_new(t_symbol *s, t_floatarg taperdur);
void bvplay_notelist(t_bvplay *x, t_symbol *msg, short argc, t_atom *argv );
void bvplay_verbose(t_bvplay *x, t_floatarg t);
void bvplay_mute(t_bvplay *x, t_floatarg t);
void bvplay_taper(t_bvplay *x, t_floatarg t);
void bvplay_dsp_free(t_bvplay *x);

void bvplay_tilde_setup(void)
{
	bvplay_class = class_new(gensym("bvplay~"),(t_newmethod)bvplay_new,
                             (t_method)bvplay_dsp_free, sizeof(t_bvplay), 0, A_SYMBOL, A_FLOAT,0);
	CLASS_MAINSIGNALIN(bvplay_class,t_bvplay, x_f);
	class_addmethod(bvplay_class,(t_method)bvplay_dsp,gensym("dsp"),A_CANT,0);
	class_addmethod(bvplay_class,(t_method)bvplay_notelist,gensym("list"),A_GIMME,0);
	class_addmethod(bvplay_class,(t_method)bvplay_verbose,gensym("verbose"),A_FLOAT,0);
	class_addmethod(bvplay_class,(t_method)bvplay_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(bvplay_class,(t_method)bvplay_taper,gensym("taper"),A_FLOAT,0);
    
	potpourri_announce(OBJECT_NAME);
}

void bvplay_taper(t_bvplay *x, t_floatarg t)
{
	if(t>0){
		x->taper_dur = (float)t/1000.0;
		x->taper_frames = x->R * x->taper_dur;
	}
}


void bvplay_mute(t_bvplay *x, t_floatarg f)
{
    x->mute = (short)f;
}

void bvplay_verbose(t_bvplay *x, t_floatarg f)
{
    x->verbose = (short)f;
}


void bvplay_notelist(t_bvplay *x, t_symbol *msg, short argc, t_atom *argv)
{
    
	if( x->active ){
		if( x->verbose )
			error("object still playing - cannot add note!");
		return;
	}
	bvplay_set(x, x->sfname);
	if(! x->wavebuf->b_valid){
		post("%s: no valid buffer yet",OBJECT_NAME);
		return;
	}
    
	// read note data
	if( argc != 4 ){
		if( x->verbose ){
			post("improper note data");
			post("notelist parameters: skiptime, duration, increment, amplitude");
		}
	}
    
	x->notedata[0] = atom_getfloatarg(0,argc,argv) / 1000.0;
	x->notedata[1] = atom_getfloatarg(1,argc,argv) / 1000.0;
	x->notedata[2] = atom_getfloatarg(2,argc,argv);
	x->notedata[3] = atom_getfloatarg(3,argc,argv);
    
	x->start_frame = x->notedata[0] * x->R;
	x->increment = x->notedata[2];
	x->index = x->findex = x->start_frame;
    
	if( x->increment == 0.0 ){
		if( x->verbose )
			post("zero increment!");
		return;
	}
	x->note_frames =  x->notedata[1] * x->increment  * x->R;
	x->end_frame = x->start_frame + x->note_frames;
    
    x->amp = x->notedata[3];
	if( x->start_frame < 0 || x->start_frame >= x->wavebuf->b_frames){
		if( x->verbose )
			post("%s: bad start time",OBJECT_NAME);
		return;
	}
	if( x->end_frame < 0 || x->end_frame >= x->wavebuf->b_frames){
		if( x->verbose )
			post("%s: bad end time",OBJECT_NAME);
		return;
	}
    
	x->active = 1;
}

t_int *bvplay_perform_mono(t_int *w)
{
    t_bvplay *x = (t_bvplay *)(w[1]);
    t_float *out = (t_float *)(w[2]);
    int n = (int) w[3];
	t_word *tab;
	long iindex = x->index;
	float findex = x->findex;
	int end_frame = x->end_frame;
	float increment = x->increment;
	int start_frame = x->start_frame;
	int taper_frames = x->taper_frames;
	float noteamp = x->amp;
	float frac, amp;
    /**********************/
    bvplay_set(x,x->sfname);
    
	if(!x->wavebuf->b_valid) {
		post("invalid buffer");
		memset(out, 0, sizeof(float) * n);
		return (w+4);
	}
	tab = x->wavebuf->b_samples;
	
	if(x->active){
		while(n--){
            // post("index: %d endframe %d", iindex, end_frame);
			if((increment > 0 && iindex < end_frame) || (increment < 0 && iindex > end_frame)) {
				// envelope
				if( increment > 0 ){
					if( findex < start_frame + taper_frames ){
						amp = noteamp * ((findex - (float) start_frame) / (float) taper_frames );
					} else if ( findex > end_frame - taper_frames) {
						amp = noteamp * (((float)end_frame - findex) / (float) taper_frames);
					} else {
						amp = noteamp;
					}
				} else { // negative increment case
					if( findex > start_frame - taper_frames ){
						amp =  noteamp * ( (start_frame - findex) / taper_frames );
					} else if ( findex < end_frame + taper_frames) {
						amp = noteamp * (( findex - end_frame ) / taper_frames) ;
					} else {
						amp = noteamp;
					}
                    
				}
				frac = findex - iindex ;
				*out++ = amp * (tab[iindex].w_float + frac * (tab[iindex + 1].w_float - tab[iindex].w_float));
				findex += increment;
				iindex = findex ;
			} else {
				*out++ = 0;
				x->active = 0;
			}
		}
        
	}
	else{
		while(n--){
			*out++ = 0;
		}
	}
    
	x->index = iindex;
	x->findex = findex;
    
	return (w+4);
}

void bvplay_set(t_bvplay *x, t_symbol *wavename)
{
    
	t_garray *a;
	int b_frames;
	t_word *b_samples;
	if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class))) {
		if (*wavename->s_name) pd_error(x, "%s: %s: no such array",OBJECT_NAME,wavename->s_name);
		x->wavebuf->b_valid = 0;
    }
	else if (!garray_getfloatwords(a, &b_frames, &b_samples)) {
		pd_error(x, "%s: bad array for %s", wavename->s_name,OBJECT_NAME);
		x->wavebuf->b_valid = 0;
    }
	else  {
		x->wavebuf->b_valid = 1;
		x->wavebuf->b_frames = b_frames;
		x->wavebuf->b_nchans = 1;
		x->wavebuf->b_samples = b_samples;
		garray_usedindsp(a);
	}
    
}


void *bvplay_new(t_symbol *s, t_floatarg taperdur)
{
    int ichan = 1;
    
	t_bvplay *x = (t_bvplay *)pd_new(bvplay_class);

    outlet_new(&x->x_obj, gensym("signal"));

    
	x->object_chans = ichan;
    taperdur /= 1000.0; // convert to seconds
	if(taperdur <= 0)
		taperdur = .005;
	x->sfname = s;
	x->R = sys_getsr();
	if(! x->R){
		error("zero sampling rate - set to 44100");
		x->R = 44100;
	}
	x->notedata = (float *) calloc(4, sizeof(float));
	x->wavebuf = (t_guffer *) calloc(1, sizeof(t_guffer));
	x->taper_dur = taperdur;
	x->taper_frames = x->R * x->taper_dur;
	x->buffer_duration = 0.0 ;
	x->framesize = 0;
	x->active = 0;
	x->verbose = 0;
	x->mute = 0;
	// post("channels %f, taper duration %.4f, taperframes %d", chan, taperdur, x->taper_frames );
    
	// post("arguments: channels, taper_duration(secs.)");
	return x;
}

void bvplay_dsp_free(t_bvplay *x)
{
    
	free(x->notedata);
	free(x->wavebuf);
}

void bvplay_dsp(t_bvplay *x, t_signal **sp)
{
    bvplay_set(x,x->sfname);
    
    if(x->R != sp[0]->s_sr){
    	x->R = sp[0]->s_sr;
    	x->taper_frames = x->R * x->taper_dur;
    }
    // Pd buffers always mono ...
    dsp_add(bvplay_perform_mono, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);
    /*
    if( x->wavebuf->b_nchans == 1 ) {
    	// post("initiating mono processor");
   	 	dsp_add(bvplay_perform_mono, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);
    } else if( x->wavebuf->b_nchans == 2) {
    	// post("initiating stereo processor");
   	 	dsp_add(bvplay_perform_stereo,4,x,sp[0]->s_vec,sp[1]->s_vec,(t_int)sp[0]->s_n);
    } else {
     	post("%s: bad channel spec: %d, cannot initiate dsp code",OBJECT_NAME,  x->wavebuf->b_nchans);
    }*/
}
