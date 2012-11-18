#include "MSPd.h"
#include "fftease.h"


#define THRESHOLD_MIN (.000001)

#if MSP
void *reanimator_class;
#endif 

#if PD
static t_class *reanimator_class;
#endif


#define OBJECT_NAME "reanimator~"

typedef struct _reanimator
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
	float **framebank;
	float *normalized_frame;
	float current_frame;
	int framecount;
	//
	float frame_increment ;
	float last_frame ;
	
	float fpos;
	float last_fpos;
	float tadv;
	// for convert
	float *c_lastphase_in;
	float *c_lastphase_out;
	float c_fundamental;
	float c_factor_in;
	float c_factor_out;
	// faster fft
	float mult; 
	float *trigland;
	int *bitshuffle;
	int read_me;
	
	int total_frames;
	short mute;
	short initialized;
	float threshold;
	short inverse;
	int top_comparator_bin;
	short residency_mode;
	int vs;// vector size
		int winfac;//window factor
			int overlap;//overlap factor
				float sample_len;//duration of texture sample
					float sync;
					int megs;
} t_reanimator;

void *reanimator_new(t_symbol *msg, short argc, t_atom *argv);
// t_int *offset_perform(t_int *w);
t_int *reanimator_perform(t_int *w);
void reanimator_dsp(t_reanimator *x, t_signal **sp, short *count);
void reanimator_assist(t_reanimator *x, void *b, long m, long a, char *s);
void reanimator_analyze (t_reanimator *x);
void reanimator_float(t_reanimator *x, double f) ;
void reanimator_mute(t_reanimator *x, t_floatarg flag);
void reanimator_inverse(t_reanimator *x, t_floatarg toggle);
void reanimator_topbin(t_reanimator *x, t_floatarg bin);
void reanimator_startframe(t_reanimator *x, t_floatarg start);
void reanimator_endframe(t_reanimator *x, t_floatarg end);
void reanimator_framerange(t_reanimator *x, t_floatarg start, t_floatarg end);
void reanimator_inverse(t_reanimator *x, t_floatarg toggle);
void reanimator_size(t_reanimator *x, t_floatarg size_ms);
void reanimator_freeze_and_march(t_reanimator *x, t_floatarg f);
void reanimator_resume( t_reanimator *x );
void reanimator_threshold(t_reanimator *x, t_floatarg threshold);
void reanimator_dsp_free( t_reanimator *x );
void reanimator_framecount ( t_reanimator *x );
void reanimator_init(t_reanimator *x, short initialized);
void reanimator_fftinfo(t_reanimator *x);
void reanimator_overlap(t_reanimator *x, t_floatarg f);
void reanimator_winfac(t_reanimator *x, t_floatarg f);
void reanimator_meminfo(t_reanimator *x);

#if MSP
void main(void)
{
	setup((t_messlist **)&reanimator_class, (method)reanimator_new, 
		  (method)reanimator_dsp_free, (short)sizeof(t_reanimator), 0L, A_GIMME,0);
	addmess((method)reanimator_dsp, "dsp", A_CANT, 0);
	addmess((method)reanimator_assist,"assist",A_CANT,0);
	addfloat((method)reanimator_float);
	addmess ((method)reanimator_mute, "mute", A_FLOAT, 0);
	addmess ((method)reanimator_inverse, "inverse", A_FLOAT, 0);
	addmess ((method)reanimator_topbin, "topbin", A_FLOAT, 0);
	addmess ((method)reanimator_threshold, "threshold", A_FLOAT, 0);
	addmess ((method)reanimator_analyze, "analyze", 0);
	addmess ((method)reanimator_framecount, "framecount", 0);
	addmess ((method)reanimator_freeze_and_march, "freeze_and_march", A_FLOAT, 0);
	addmess ((method)reanimator_resume, "resume", 0);
	addmess((method)reanimator_overlap,"overlap",A_DEFFLOAT,0);
	addmess((method)reanimator_winfac,"winfac",A_DEFFLOAT,0);
	addmess((method)reanimator_fftinfo,"fftinfo",0);
	addmess((method)reanimator_meminfo,"meminfo",0);
	dsp_initclass();
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif
#if PD
/* Pd Initialization */
void reanimator_tilde_setup(void)
{
	reanimator_class = class_new(gensym("reanimator~"), (t_newmethod)reanimator_new, 
								 (t_method)reanimator_dsp_free ,sizeof(t_reanimator), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(reanimator_class, t_reanimator, x_f);
	class_addmethod(reanimator_class, (t_method)reanimator_dsp, gensym("dsp"), 0);
	class_addmethod(reanimator_class, (t_method)reanimator_mute, gensym("mute"), A_FLOAT,0);
	class_addmethod(reanimator_class, (t_method)reanimator_inverse, gensym("inverse"), A_FLOAT,0);
	class_addmethod(reanimator_class, (t_method)reanimator_topbin, gensym("topbin"), A_FLOAT,0);
	class_addmethod(reanimator_class, (t_method)reanimator_threshold, gensym("threshold"), A_FLOAT,0);
	
	class_addmethod(reanimator_class, (t_method)reanimator_freeze_and_march, gensym("freeze_and_march"), A_FLOAT, 0);
	class_addmethod(reanimator_class, (t_method)reanimator_analyze, gensym("analyze"), 0);
	class_addmethod(reanimator_class, (t_method)reanimator_framecount, gensym("framecount"), 0);
	class_addmethod(reanimator_class, (t_method)reanimator_overlap, gensym("overlap"), A_FLOAT, 0);
	class_addmethod(reanimator_class, (t_method)reanimator_winfac, gensym("winfac"), A_FLOAT, 0);
	class_addmethod(reanimator_class, (t_method)reanimator_fftinfo, gensym("fftinfo"), 0);
	class_addmethod(reanimator_class, (t_method)reanimator_meminfo, gensym("meminfo"), 0);
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void reanimator_overlap(t_reanimator *x, t_floatarg f)
{
	int i = (int) f;
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->overlap = i;
	reanimator_init(x,1);
}

void reanimator_winfac(t_reanimator *x, t_floatarg f)
{
	int i = (int)f;
	
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = i;
	reanimator_init(x,2);
}

void reanimator_meminfo(t_reanimator *x)
{
	post("%s currently stores %d FFT Frames, duration: %f sec., size: %.2f MB",
		 OBJECT_NAME, x->framecount, x->tadv * x->framecount, (float)x->megs/1000000.0);
}

void reanimator_fftinfo(t_reanimator *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void reanimator_framecount ( t_reanimator *x )
{	
	post("%d frames stored", x->total_frames);
}

void reanimator_freeze_and_march(t_reanimator *x, t_floatarg f)
{	
	x->frame_increment = f;
	x->residency_mode = 1;
}

void reanimator_resume( t_reanimator *x )
{
	x->residency_mode = 0;
}

void reanimator_dsp_free( t_reanimator *x ){
	int i ;
#if MSP
	dsp_free( (t_pxobject *) x);
#endif
	for( i = 0; i < x->framecount; i++ ){
		freebytes(x->framebank[i],0) ;
	}
	
	freebytes(x->c_lastphase_in,0);
	freebytes(x->c_lastphase_out,0);
	freebytes(x->trigland,0);
	freebytes(x->bitshuffle,0);
	freebytes(x->Wanal,0);
	freebytes(x->Wsyn,0);
	freebytes(x->input,0);
	freebytes(x->Hwin,0);
	freebytes(x->buffer,0);
	freebytes(x->channel,0);
	freebytes(x->normalized_frame,0);
	freebytes(x->output,0);
}


void reanimator_assist (t_reanimator *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Driver Sound ");
				break;
			case 1:
				sprintf(dst,"(signal) Texture Sound");
				break;
				
		}
	} else if (msg==2) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Output");
				break;
			case 1:
				sprintf(dst,"(signal) Matched Frame");
				break;
			case 2:
				sprintf(dst,"(signal) Sync");
				break;
				
		}
	}
}


void *reanimator_new(t_symbol *msg, short argc, t_atom *argv)
{
#if MSP
	t_reanimator *x = (t_reanimator *)newobject(reanimator_class);
	dsp_setup((t_pxobject *)x,2);
	outlet_new((t_pxobject *)x, "signal");
	outlet_new((t_pxobject *)x, "signal");
	outlet_new((t_pxobject *)x, "signal");
#endif
#if PD
	int i;
	t_reanimator *x = (t_reanimator *)pd_new(reanimator_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	for(i=0;i<3;i++)
		outlet_new(&x->x_obj, gensym("signal"));
#endif
	
	x->vs = sys_getblksize();
	x->R = sys_getsr();
	
	x->sample_len = atom_getfloatarg(0,argc,argv) * .001;// convert to seconds
		x->overlap = atom_getintarg(1,argc,argv);
		x->winfac = atom_getintarg(2,argc,argv);
		
		if(!x->overlap)
			x->overlap = 4;
		if(!x->winfac)
			x->winfac = 1;
		if(x->sample_len <= .05){
			x->sample_len = 1.0;
			post("%s: sample length set to default 1000 ms.",OBJECT_NAME);
		}
		
		reanimator_init(x,0);
		return (x);
}

void reanimator_init(t_reanimator *x, short initialized)
{
	int i;
	x->D = x->vs;
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->tadv = (float)x->D/(float)x->R;
	x->mult = 1. / (float) x->N;
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->inCount = -(x->Nw);	
	x->current_frame = x->framecount = 0;
	x->fpos = x->last_fpos = 0;
	x->framecount = 0;
	x->total_frames =  x->sample_len / x->tadv;	
	
	if(!initialized){
		x->sync = 0.0;
		x->inverse = 0;
		x->initialized = 0;
		x->threshold = .0001;
		x->top_comparator_bin = 10 ;
		x->residency_mode = 0;
		x->frame_increment = 1.0 ;
		x->mute = 0;
		x->read_me = 0;
		x->framecount = 0;
		x->Wanal = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->Wsyn = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->Hwin = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->input = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->output = (float *) getbytes( MAX_Nw * sizeof(float) );
		x->buffer = (float *) getbytes( MAX_N * sizeof(float) );
		x->channel = (float *) getbytes( (MAX_N+2) * sizeof(float) );
		x->bitshuffle = (int *) getbytes( MAX_N * 2 * sizeof( int ) );
		x->trigland = (float *) getbytes( MAX_N * 2 * sizeof( float ) );
		x->c_lastphase_in = (float *) getbytes( (MAX_N2+1) * sizeof(float) );
		x->c_lastphase_out = (float *) getbytes( (MAX_N2+1) * sizeof(float) );
		
		x->total_frames =  x->sample_len / x->tadv;
		x->normalized_frame = (float *) getbytes( (MAX_N+2) * sizeof(float));
		x->framebank = (float **) getbytes(x->total_frames * sizeof(float *));
		/* taking a risk here */
		while(x->framecount < x->total_frames ){
			x->framebank[(x->framecount)] = (float *) getbytes((x->N+2) * sizeof(float));
			memset((char *)x->framebank[(x->framecount)],0,(x->N+2) * sizeof(float));
			++(x->framecount);
		}
		
	} else {
		while(x->framecount < x->total_frames ){
			x->framebank[x->framecount] = 
				(float *) resizebytes((char *)x->framebank[x->framecount],0,(x->N+2) * sizeof(float));
			memset((char *)x->framebank[(x->framecount)],0,(x->N+2) * sizeof(float));
			++(x->framecount);
		}
	}
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
	memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));

	
	x->c_fundamental =  (float) x->R/( (x->N2)<<1 );
	x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
	x->c_factor_out =TWOPI * (float)  x->D / (float) x->R;
	init_rdft(x->N, x->bitshuffle, x->trigland);
	makewindows( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D);
	
	x->megs = sizeof(float) * x->framecount * (x->N+2);
}

t_int *reanimator_perform(t_int *w)
{
	float sample, outsamp ;
	float ampsum, new_ampsum, rescale;
	float min_difsum, difsum;
	int	
		i,j, match_frame=0;
	
	
	////////////////////////////////////////////// 
	t_reanimator *x = (t_reanimator *) (w[1]);
	t_float *driver = (t_float *)(w[2]);
	t_float *texture = (t_float *)(w[3]);
	t_float *soundout = (t_float *)(w[4]);
	t_float *matchout = (t_float *)(w[5]);
	t_float *sync_vec = (t_float *)(w[6]);
	int n = (int)(w[7]);
	
	/* dereference structure */	
	
	int inCount = x->inCount;
	int R = x->R;
	int N = x->N;
	int N2 = x->N2;
	int D = x->D;
	int Nw = x->Nw;
	float *Wanal = x->Wanal;
	float *Wsyn = x->Wsyn;		
	float *input = x->input;
	float *output = x->output;
	float *buffer = x->buffer;
	float *channel = x->channel;
	float *normalized_frame = x->normalized_frame;
	int framecount = x->framecount;
	int total_frames = x->total_frames;
	
	float threshold = x->threshold;
	int top_comparator_bin = x->top_comparator_bin ;
	
	float mult = x->mult ;
	int *bitshuffle = x->bitshuffle;
	float *trigland = x->trigland ;
	
	float *c_lastphase_in = x->c_lastphase_in;
	float *c_lastphase_out = x->c_lastphase_out;
	float c_fundamental = x->c_fundamental;
	float c_factor_in = x->c_factor_in;
	float c_factor_out = x->c_factor_out;
	float **framebank = x->framebank;
	// for residency mode
	float fframe = x->current_frame ;
	float last_fpos = x->last_fpos ;
	float fincr = x->frame_increment;
	float fpos = x->fpos ;
	float sync = x->sync;
	/***********************************/
	inCount += D;
	
	/* SAMPLE MODE */
	if( x->read_me ) {
		for ( j = 0 ; j < Nw - D ; j++ ){
			input[j] = input[j+D];
		}
		for ( j = Nw - D; j < Nw; j++ ) {
			input[j] = *texture++;
		}
		
		fold( input, Wanal, Nw, buffer, N, inCount );	
		rdft( N, 1, buffer, bitshuffle, trigland );
		
		sync = (float) framecount / (float) total_frames;
		for ( j = 0; j < n; j++ ){     
			*sync_vec++ = sync;
			*matchout++ = 0.0;
			*soundout++ = 0.0;
		}
		convert( buffer, framebank[framecount], N2, c_lastphase_in, c_fundamental, c_factor_in );
		
		new_ampsum = ampsum = 0;
		for( j = 0; j < N; j+= 2 ){
			ampsum += framebank[framecount][j];
		}
		
		if( ampsum > .00000001 ){
			rescale = 1.0 / ampsum ;
			for( i = 0; i < N; i+= 2 ){
				framebank[framecount][i] *= rescale;
			}
			
			++framecount;
			
		} else {
			post("amplitude for frame %d is too low\n", framecount);
		}
		
		// output empty buffers while reading
		
		
		if( framecount >= total_frames ){
			sync = 1.0;
			x->read_me = 0;
			//    post("reanimator~: data acquisition completed");
			x->initialized = 1;
			// clear input buffer
			for( i = 0; i < Nw; i++ )
				input[i] = 0.0;
		} 
		
	} else if (x->mute || ! x->initialized) {
		for ( j = 0; j < D; j++ ){
			*soundout++ = 0.0;
			*matchout++ = 0.0;
			*sync_vec++ = sync;
		}
	} 
	/* RESIDENCY RESYNTHESIS */
	else if( x->residency_mode ) {   
		if( fpos < 0 )
			fpos = 0;
		if( fpos > 1 )
			fpos = 1;
		if( fpos != last_fpos ){
			fframe =  fpos * (float) framecount ;
			last_fpos = fpos;
		}
		
		
		fframe += fincr;
		while( fframe >= framecount ) {
			fframe -= framecount;
		} 
		while( fframe < 0. ) {
			fframe += framecount ;
		}
		
		unconvert(framebank[(int) fframe ], buffer, N2, c_lastphase_out, c_fundamental, c_factor_out );
		
		rdft( N, -1, buffer, bitshuffle, trigland );
		overlapadd( buffer, N, Wsyn, output, Nw, inCount );
		
		for ( j = 0; j < D; j++ ){
			*soundout++ = output[j] * mult;
			*matchout++ = (int) fframe;
			*sync_vec++ = sync;
		}
		for ( j = 0; j < Nw - D; j++ ){
			output[j] = output[j+D];
		}
		
		for ( j = Nw - D; j < Nw; j++ ){
			output[j] = 0.;
		}
		x->current_frame = fframe;
		x->frame_increment = fincr;
		x->fpos = fpos;
		x->sync = sync;
	}
	/* REANIMATION HERE */
	else {
		for ( j = 0 ; j < Nw - D ; j++ ){
			input[j] = input[j+D];
		}
		for ( j = Nw - D; j < Nw; j++ ) {
			input[j] = *driver++;
		}
		
		fold( input, Wanal, Nw, buffer, N, inCount );	
		rdft( N, 1, buffer, bitshuffle, trigland );
		convert( buffer, channel, N2, c_lastphase_in, c_fundamental, c_factor_in );
		ampsum = 0;
		// NORMALIZE INPUT FRAME
		for( i = 0; i < N+2; i += 2 ){
			ampsum += channel[i];
		}
		
		if( ampsum > threshold ){
			rescale = 1.0 / ampsum;
			for( i = 0; i < N; i += 2 ){
				channel[i] *= rescale;
			}
		} else {
			// AMPLITUDE OF INPUT WAS TOO LOW - OUTPUT SILENCE AND RETURN
			for ( j = 0; j < D; j++ ){
				*soundout++ = 0.0;
				*matchout++ = 0.0;
				*sync_vec++ = sync;
				
			}
			x->inCount = inCount;
			x->sync = sync;
			return (w+8);
			
		}
		// NOW COMPARE TO STORED FRAMES
		if( x->inverse ){ // INVERSE CASE
			min_difsum = 0.0 ;
			
			for( j = 0; j < framecount; j++ ){
				difsum = 0;
				for( i = 0; i < top_comparator_bin * 2; i += 2 ){
					difsum += fabs( channel[i] - framebank[j][i] ); 
				}
				//      fprintf(stderr,"bin 20: in %f compare %f\n", channel[40], frames[j][40]);
				if( difsum > min_difsum ){
					match_frame = j;
					min_difsum = difsum;
				}
			}
		} else { // NORMAL CASE
			min_difsum = 1000000.0 ;
			
			for( j = 0; j < framecount; j++ ){
				difsum = 0;
				for( i = 0; i < top_comparator_bin * 2; i += 2 ){
					difsum += fabs( channel[i] - framebank[j][i] ); 
				}
				//      fprintf(stderr,"bin 20: in %f compare %f\n", channel[40], frames[j][40]);
				if( difsum < min_difsum ){
					match_frame = j;
					min_difsum = difsum;
				}
			}
		}
		
		unconvert(  framebank[match_frame], buffer, N2, c_lastphase_out, c_fundamental, c_factor_out );
		
		rdft( N, -1, buffer, bitshuffle, trigland );
		overlapadd( buffer, N, Wsyn, output, Nw, inCount );
		
		// scale back to match
		for ( j = 0; j < D; j++ ){
			*soundout++ = output[j] * (mult / rescale);
			*matchout++ = (float) match_frame;
			*sync_vec++ = sync;
		}
		for ( j = 0; j < Nw - D; j++ ){
			output[j] = output[j+D];
		}
		
		for ( j = Nw - D; j < Nw; j++ ){
			output[j] = 0.;
		}
		x->current_frame = match_frame; // for residency
		x->sync = sync;
	}
	
	/* restore state variables */
	x->framecount = framecount;
	x->inCount = inCount % Nw;
	x->sync = sync;
	
	return (w+8);
}

#if MSP		
void reanimator_float(t_reanimator *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		x->frame_increment = f;
    }
	else if (inlet == 2)
    {
		if (f < 0 ){
			f = 0;
		} else if(f > 1) {
			f = 1.;
		}
		x->fpos = f;
		
    }
}
#endif

void reanimator_analyze ( t_reanimator *x )
{
	// int i, j;
	
	x->read_me = 1;
	x->framecount = 0;
	//  post("reanimator: beginning spectral data acquisition");
	return;
	
}

void reanimator_mute(t_reanimator *x, t_floatarg flag)
{
	x->mute = flag;	
	//  post ("mute set to %d", flag);
}

void reanimator_topbin(t_reanimator *x, t_floatarg bin)
{
	if( bin > 1 && bin < x->N2 )
		x->top_comparator_bin = bin;
}


void reanimator_inverse(t_reanimator *x, t_floatarg toggle)
{
	x->inverse = toggle;	
	//  post ("inverse set to %d", toggle);
}

void reanimator_threshold(t_reanimator *x, t_floatarg threshold)
{
	if( threshold > THRESHOLD_MIN )
		x->threshold = threshold;
	else
		x->threshold = THRESHOLD_MIN;	
}

void reanimator_dsp(t_reanimator *x, t_signal **sp, short *count)
{
	//  long i;
	
	if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr){
		x->vs = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		reanimator_init(x,1);
	}
	dsp_add(reanimator_perform, 7, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,  
			sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
}


