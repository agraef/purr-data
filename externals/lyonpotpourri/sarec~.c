#include "MSPd.h"

static t_class *sarec_class;

#define MAXBEATS (256)
#define OBJECT_NAME "sarec~"
#define SAREC_RECORD 1
#define SAREC_OVERDUB 2
#define SAREC_PUNCH 3
#define SAREC_LOCK 4

// update time in samples
#define WAVEFORM_UPDATE (22050)

typedef struct _sarec
{
	t_object x_obj;
    t_float x_f;
    long b_valid;
    long b_frames;
    t_word *b_samples;
	long display_counter;
	int valid; // status of recording buffer (not yet implemented)
	int status; // idle? 0, recording? 1
	int mode; // record, overdub or punch
	int overdub; // 0 ? write over track, 1: overdub into track
	int *armed_chans; // 1, armed, 0, protected
	long counter; // sample counter
	float sync; // position in recording
	long start_frame; // start time in samples
	long end_frame; // end time in samples
	long fadesamps; // number of samples for fades on PUNCH mode
	long regionsamps; // use for fade
	int channel_count; // number of channels (hopefully!) in buffer
	float sr;
    float syncphase;
	t_symbol *bufname; // name of recording buffer
    t_garray *recbuf; 
} t_sarec;


void *sarec_new(t_symbol *bufname);

// void sarec_region(t_sarec *x, t_symbol *msg, short argc, t_atom *argv);
void sarec_regionsamps(t_sarec *x, t_floatarg start_frame, t_floatarg end_frame);
void sarec_region(t_sarec *x, t_floatarg start_time, t_floatarg end_time);

t_int *sarec_perform(t_int *w);
void sarec_dsp(t_sarec *x, t_signal **sp);


void sarec_arm(t_sarec *x, t_floatarg chan);
void sarec_disarm(t_sarec *x, t_floatarg chan);
void sarec_attach_buffer(t_sarec *x);
void sarec_overdub(t_sarec *x);
void sarec_record(t_sarec *x);
void sarec_punch(t_sarec *x);
void sarec_punchfade(t_sarec *x, t_floatarg fadetime);
void sarec_tilde_setup(void);
void sarec_lock(t_sarec *x);


void sarec_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("sarec~"),(t_newmethod)sarec_new,0, sizeof(t_sarec),0, A_SYMBOL,0);
    CLASS_MAINSIGNALIN(c,t_sarec,x_f);
    class_addmethod(c,(t_method)sarec_dsp,gensym("dsp"),A_CANT,0);
    class_addmethod(c,(t_method)sarec_arm,gensym("arm"),A_FLOAT,0);
    class_addmethod(c,(t_method)sarec_disarm,gensym("disarm"),A_FLOAT,0);
    class_addmethod(c,(t_method)sarec_overdub,gensym("overdub"),0);
    class_addmethod(c,(t_method)sarec_lock,gensym("lock"),0);
    class_addmethod(c,(t_method)sarec_punch,gensym("punch"),0);
    class_addmethod(c,(t_method)sarec_punchfade,gensym("punchfade"),A_FLOAT,0);
    class_addmethod(c,(t_method)sarec_record,gensym("record"),0);
    class_addmethod(c,(t_method)sarec_regionsamps,gensym("regionsamps"),A_FLOAT, A_FLOAT,0);
    class_addmethod(c,(t_method)sarec_region,gensym("region"),A_FLOAT, A_FLOAT,0);
    sarec_class = c;
    potpourri_announce(OBJECT_NAME);
}


void sarec_lock(t_sarec *x)
{
	x->mode = SAREC_LOCK;
}


void *sarec_new(t_symbol *bufname)
{
	int i;
	int chans;
    
    chans = 1; // mono processing only in Pd
    
	t_sarec *x = (t_sarec *)pd_new(sarec_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));

	
    x->status = 0;
	x->counter = 0;
    x->b_valid = 0;
    x->bufname = bufname;
	x->channel_count = chans;
	x->armed_chans = (int *) malloc(x->channel_count * sizeof(int));
	x->display_counter = 0;
	x->start_frame = 0;
	x->end_frame = 0;
	x->overdub = 0;
	x->mode = SAREC_RECORD;
	x->sr = sys_getsr();
	x->fadesamps = x->sr * 0.05; // 50 ms fade time
	x->start_frame = -1; // initialize to a bad value
	x->end_frame = -1;
	x->regionsamps = 0;
	for(i = 0; i < x->channel_count; i++){
		x->armed_chans[i] = 1; // all channels armed by default
	}
	return x;
}

t_int *sarec_perform(t_int *w)
{
	int i, j;
	t_sarec *x = (t_sarec *) (w[1]);
	t_float *click_inlet = (t_float *) (w[2]);
	t_float *record_inlet;
	t_float *sync;
	t_int channel_count = x->channel_count;
	int n;
	int next_pointer = channel_count + 5;
	int status = x->status;
	int counter = x->counter;
	int *armed_chans = x->armed_chans;
	t_word *samples = x->b_samples;
	long b_frames = x->b_frames;
	long start_frame = x->start_frame;
	long end_frame = x->end_frame;
    long b_valid = x->b_valid;
	//int overdub = x->overdub;
	int mode = x->mode;
	long fadesamps = x->fadesamps;
	long regionsamps = x->regionsamps;
	int clickval;
	float frak;
	float goin_up, goin_down;
	long counter_msf;
	sync = (t_float *) (w[3 + channel_count]);
	n = (int) w[4 + channel_count];
    if(! b_valid)
        goto escape;
	if(! regionsamps ){
		x->regionsamps = regionsamps = end_frame - start_frame;
	}
	for(i = 0; i < n; i++){
		// could be record (1) or overdub (2)
		if( click_inlet[i] ){
			clickval = (int) click_inlet[i];
			if(clickval == -2) {
				status = 0;
				counter = 0;
                garray_redraw(x->recbuf);
			}
			else {
				// arm all channels
				if(clickval == -1) {
					// just use all armed channels
				}
				// arm only one channel - protect against fatal bad index
				else if(clickval <= channel_count && clickval > 0) {
					for(j=0; j < channel_count; j++){
						armed_chans[j] = 0;
					};
					armed_chans[clickval - 1] = 1;
					// post("arming channel %d", clickval);
				}
				
				counter = start_frame;
				if(!end_frame){
					end_frame = b_frames;
				}
				status = 1;
			}
			
		}		
		if(counter >= end_frame) {
			counter = 0;
			status = 0;
            garray_redraw(x->recbuf);
		}
		// write over track
		if(status && (mode == SAREC_RECORD) ){
			for(j=0; j < channel_count; j++){
				if( armed_chans[j] ){
					record_inlet = (t_float *) (w[3 + j]);
					samples[ (counter * channel_count) + j].w_float = record_inlet[i];
				}
			}
			counter++;
		}
		// overdub
		else if(status && (mode == SAREC_OVERDUB) ){
			for(j=0; j < channel_count; j++){
				if( armed_chans[j] ){
					record_inlet = (t_float *) (w[3 + j]);
					samples[ (counter * channel_count) + j].w_float += record_inlet[i];
				}
			}
			counter++;
		}
		// punch
		
		else if(status && (mode == SAREC_PUNCH)) {
			counter_msf = counter - start_frame;
			for(j=0; j < channel_count; j++){
				if( armed_chans[j] ){
					record_inlet = (t_float *) (w[3 + j]);
					// frak is multiplier for NEW STUFF, (1-frak) is MULTIPLIER for stuff to fade out
					// do power fade though
					
					if( counter_msf < fadesamps ){
						
						// fade in
						frak = (float)counter_msf / (float)fadesamps;
						goin_up = sin(PIOVERTWO * frak);
						goin_down = cos(PIOVERTWO * frak);
						//post("fadein: %d, up: %f, down: %f", counter_msf, goin_up, goin_down);
						samples[ (counter * channel_count) + j].w_float =
						(samples[ (counter * channel_count) + j].w_float * goin_down)
						+ (record_inlet[i] * goin_up);
					} else if ( counter_msf >= (regionsamps - fadesamps) ){
						frak = (float) (regionsamps - counter_msf) / (float) fadesamps;
						// fade out
                        
						// frak = (float)counter_msf / (float)fadesamps;
						goin_up = cos(PIOVERTWO * frak);
						goin_down = sin(PIOVERTWO * frak);
						//post("fadeout: %d, up: %f, down: %f", counter_msf, goin_up, goin_down);
						samples[ (counter * channel_count) + j].w_float =
						(samples[ (counter * channel_count) + j].w_float * goin_up)
						+ (record_inlet[i] * goin_down);
					} else {
						// straight replace
						samples[ (counter * channel_count) + j].w_float = record_inlet[i];
					}
					
				}
			}
			counter++;
		}
		
		sync[i] = (float) counter / (float) b_frames;
	}
	if(status){
		x->display_counter += n;
		if(x->display_counter > WAVEFORM_UPDATE){
            garray_redraw(x->recbuf);
			x->display_counter = 0;
		}
	}
	x->end_frame = end_frame;
	x->status = status;
	x->counter = counter;
escape:
	return w + next_pointer;
}

void sarec_overdub(t_sarec *x)
{
	x->mode = SAREC_OVERDUB;
}


void sarec_record(t_sarec *x)
{
	x->mode = SAREC_RECORD;
}

void sarec_punchfade(t_sarec *x, t_floatarg fadetime)
{
	x->fadesamps = x->sr * fadetime * 0.001; // read fade in ms.
	//post("punch mode");
}

void sarec_punch(t_sarec *x)
{
	x->mode = SAREC_PUNCH;
	// post("punch mode");
}

void sarec_disarm(t_sarec *x, t_floatarg chan)
{
    //	int i;
	int ichan = (int) chan;
	if(chan <= x->channel_count && chan > 0) {
		x->armed_chans[ichan - 1] = 0;
	}
}

void sarec_arm(t_sarec *x, t_floatarg chan)
{
	int i;
	int ichan = (int) chan;
	if(ichan == -1){
		for(i = 0; i < x->channel_count; i++){
			x->armed_chans[i] = 1;
		}
	} else if(ichan == 0)
	{
		for(i = 0; i < x->channel_count; i++){
			x->armed_chans[i] = 0;
		}
	} else if(chan <= x->channel_count && chan > 0) {
		x->armed_chans[ichan - 1] = 1;
	}
}


void sarec_region(t_sarec *x, t_floatarg start_time, t_floatarg end_time)
{
	long b_frames = x->b_frames;
	long start_frame, end_frame;
	float sr = x->sr;
	// convert milliseconds to samples:
	start_frame = (long) (sr * 0.001 * start_time );
	end_frame = (long) (sr * 0.001 * end_time );
	start_frame = start_frame < 0 || start_frame > b_frames - 1 ? 0 : start_frame;
	end_frame = end_frame > b_frames ? b_frames : end_frame;
	x->end_frame = end_frame;
	x->start_frame = start_frame;
	x->regionsamps = end_frame - start_frame;
}

void sarec_regionsamps(t_sarec *x, t_floatarg start_frame, t_floatarg end_frame)
{
	long b_frames = x->b_frames;
	// long start_frame, end_frame;
	//start_frame = (long) atom_getfloatarg(0, argc, argv);
	//end_frame = (long) atom_getfloatarg(1, argc, argv);
	start_frame = start_frame < 0 || start_frame > b_frames - 1 ? 0 : start_frame;
	end_frame = end_frame > b_frames ? b_frames : end_frame;
	x->end_frame = (long)end_frame;
	x->start_frame = (long)start_frame;
	x->regionsamps = end_frame - start_frame;
}


void sarec_attach_buffer(t_sarec *x)
{
	t_garray *a; 
	t_symbol *bufname = x->bufname;
	int b_frames;
	float *b_samples = x->b_samples;
	if (!(a = (t_garray *)pd_findbyclass(bufname, garray_class))) {
		if (*bufname->s_name) pd_error(x, "%s: %s: no such array",OBJECT_NAME,bufname->s_name);
        x->b_valid = 0;
    }
	else if (!garray_getfloatwords(a, &b_frames, &b_samples)) {
		pd_error(x, "%s: bad array for %s", bufname->s_name,OBJECT_NAME);
        x->b_valid = 0;
    }
	else  {
        x->recbuf = a;
        x->b_valid = 1; // a->a_valid; ???
        x->b_frames = b_frames;
        x->b_samples = b_samples;
		garray_usedindsp(a);
	}
}

void sarec_dsp(t_sarec *x, t_signal **sp)
{
	long i;
	t_int **sigvec;
	int pointer_count;
	pointer_count = x->channel_count + 4;
    /* all channels, 1 inlet, 1 sync outlet,  the object pointer, vector size N */
	
	sarec_attach_buffer(x);
	if( x->start_frame < 0 && x->end_frame < 0){
		x->start_frame = 0;
		x->end_frame = x->b_frames;
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
	x->sr = sp[0]->s_sr;
    
	dsp_addv(sarec_perform, pointer_count, (t_int *) sigvec);
	free(sigvec);	
}