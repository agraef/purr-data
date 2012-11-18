#include "MSPd.h"
#include "fftease.h"

#if MSP
void *residency_class;
#endif 

#if PD
static t_class *residency_class;
#endif

#define OBJECT_NAME "residency~"

typedef struct _residency
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
	float **loveboat;
	float current_frame;
	int framecount;
	//
	float frame_increment ;
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
	int frames_read;
	//  int MAXFRAMES;
	short mute;
	short virgin;
	short playthrough;
	short in2_connected;
	short in3_connected;
	int overlap;
	int winfac;
	int hopsize;
	// int windowsize;
	float duration;
	short lock;
	short verbose;
	short override;
	float *input_vec;
	float sync;
} t_residency;

void *residency_new(t_symbol *s, int argc, t_atom *argv);
t_int *residency_perform(t_int *w);
void residency_dsp(t_residency *x, t_signal **sp, short *count);
void residency_assist(t_residency *x, void *b, long m, long a, char *s);
void residency_bangname(t_residency *x) ;
void residency_fftinfo(t_residency *x) ;
void residency_playthrough( t_residency *x, t_floatarg tog) ;
void residency_float(t_residency *x, double f) ;
void residency_mute(t_residency *x, t_floatarg tog);
void residency_free(t_residency *x);
void residency_init(t_residency *x, short initialized);
void residency_size(t_residency *x, t_floatarg newsize);
void residency_winfac(t_residency *x, t_floatarg factor);
void residency_overlap(t_residency *x, t_floatarg o);
void residency_verbose(t_residency *x, t_floatarg t);
void residency_acquire_sample(t_residency *x);
void residency_meminfo( t_residency *x );

#if MSP
void main(void)
{
	setup((t_messlist **)&residency_class, (method)residency_new, (method)residency_free, 
		  (short)sizeof(t_residency), 0L, A_GIMME,0);
	addmess((method)residency_dsp, "dsp", A_CANT, 0);
	addmess((method)residency_assist,"assist",A_CANT,0);
	addfloat((method)residency_float);
	addbang((method)residency_bangname);
	addmess((method)residency_mute, "mute", A_FLOAT, 0);
	addmess((method)residency_fftinfo, "fftinfo",  0);
	addmess((method)residency_meminfo, "meminfo",  0);
	addmess((method)residency_playthrough, "playthrough", A_DEFFLOAT, 0);
	addmess((method)residency_size, "size", A_DEFFLOAT, 0);
	addmess((method)residency_overlap, "overlap", A_DEFFLOAT, 0);
	addmess((method)residency_winfac, "winfac", A_DEFFLOAT, 0);
	addmess((method)residency_verbose, "verbose", A_DEFFLOAT, 0);
	dsp_initclass();
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void residency_tilde_setup(void)
{
	residency_class = class_new(gensym("residency~"), (t_newmethod)residency_new, 
								(t_method)residency_free ,sizeof(t_residency), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(residency_class, t_residency, x_f);
	class_addmethod(residency_class,(t_method)residency_dsp,gensym("dsp"),0);
	class_addmethod(residency_class,(t_method)residency_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(residency_class,(t_method)residency_fftinfo,gensym("fftinfo"),0);
	class_addmethod(residency_class,(t_method)residency_meminfo,gensym("meminfo"),0);
	class_addmethod(residency_class,(t_method)residency_playthrough,gensym("playthrough"),A_FLOAT,0); 
	class_addmethod(residency_class,(t_method)residency_size,gensym("size"),A_FLOAT,0);
	class_addmethod(residency_class,(t_method)residency_overlap,gensym("overlap"),A_FLOAT,0); 
	class_addmethod(residency_class,(t_method)residency_winfac,gensym("winfac"),A_FLOAT,0);  
	class_addmethod(residency_class,(t_method)residency_verbose,gensym("verbose"),A_FLOAT,0);
	class_addmethod(residency_class,(t_method)residency_acquire_sample,gensym("acquire_sample"),0);
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void residency_meminfo( t_residency *x )
{
    post("%d frames in buffer", x->framecount);
    post("frame_duration: %f, actual time in buffer: %f", x->tadv, (float)(x->framecount) * x->tadv);
	
}

void residency_overlap(t_residency *x, t_floatarg f)
{
	int i = (int) f;
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->overlap = i;
	residency_init(x,1);
}

void residency_winfac(t_residency *x, t_floatarg f)
{
	int i = (int)f;
	
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = i;
	//  post("called winfac");
	residency_init(x,2);
}

void residency_fftinfo(t_residency *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void residency_verbose(t_residency *x, t_floatarg t)
{
	x->verbose = t;
}

void residency_size(t_residency *x, t_floatarg newsize)
{
	//protect with DACs off?
	
	if(newsize > 0.0){//could be horrendous size, but that's the user's problem
		x->duration = newsize/1000.0;
		residency_init(x,1);  	
	}
}

void residency_playthrough (t_residency *x, t_floatarg tog)
{
	x->playthrough = tog;
}

void residency_free(t_residency *x){
	int i ;
#if MSP
	dsp_free((t_pxobject *)x);
#endif
	for(i = 0; i < x->framecount; i++){
		freebytes(x->loveboat[i],0) ;
	}
	freebytes(x->loveboat,0);
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
	freebytes(x->output,0);
}



void residency_init(t_residency *x, short initialized)
{
	int i;
	
	int last_framecount = x->framecount;
	x->lock = 1;
	x->virgin = 1;
	
	if(!x->winfac)
		x->winfac = 1;
	if(!x->overlap)
		x->overlap = 4;
	if(!x->R)
		x->R = 44100;
	if(!x->D)
		x->D = 256;
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->inCount = -(x->Nw);
	x->mult = 1. / (float) x->N;
	x->current_frame = 0;
	x->fpos = x->last_fpos = 0;
	x->tadv = (float)x->D/(float)x->R;
	x->c_fundamental = (float)x->R/((x->N2)<<1);
	x->c_factor_in = (float) x->R/((float)x->D * TWOPI);
	x->c_factor_out = TWOPI * (float)x->D / (float)x->R;
	
	if( x->duration <= 0 ){
		x->duration = 1.0;
	}
	
	x->framecount =  x->duration / x->tadv ;
	x->hopsize = x->N / x->overlap;
	x->read_me = 0;
	
	if(!initialized){
		x->sync = 0;
		x->mute = 0;
		x->in2_connected = 0;
		x->in3_connected = 0;
		x->playthrough = 0;
		x->frame_increment = 1.0;
		x->verbose = 0;
		
		
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
		
		x->input_vec = (float *) getbytes(8192  * sizeof(float));
		x->loveboat = (float **) getbytes(x->framecount * sizeof(float *));
		for(i=0;i<x->framecount;i++){
			x->loveboat[i] = (float *) getbytes(((x->N)+2) * sizeof(float));
			if(x->loveboat[i] == NULL){
				error("memory error");
				return;
			}
			memset((char *)x->loveboat[i],0,(x->N+2)*sizeof(float));
		}
	} else if(initialized == 1){
		
		for(i = 0; i < last_framecount; i++){
			freebytes(x->loveboat[i],0) ;
		}
		freebytes(x->loveboat,0);
		x->loveboat = (float **) getbytes(x->framecount * sizeof(float *));  
		for(i=0;i<x->framecount;i++){
			x->loveboat[i] = (float *) getbytes((x->N+2) * sizeof(float));
			if(x->loveboat[i] == NULL){
				error("memory error");
				return;
			}
			memset((char *)x->loveboat[i],0,(x->N+2)*sizeof(float));
		}
	} 
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
	memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));
	
	makewindows( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D);
	init_rdft( x->N, x->bitshuffle, x->trigland);
	
	x->lock = 0;
}

void *residency_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
	t_residency *x = (t_residency *)newobject(residency_class);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");
	outlet_new((t_pxobject *)x, "signal");
#endif
	
#if PD
	t_residency *x = (t_residency *)pd_new(residency_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
#endif
	//  x->x_obj.z_misc |= Z_NO_INPLACE; 
	x->duration = atom_getfloatarg(0,argc,argv)/1000.0;
	x->overlap = atom_getfloatarg(1,argc,argv);
	x->winfac = atom_getfloatarg(2,argc,argv);
	
	x->D = sys_getblksize();
	x->R = sys_getsr();
	
	residency_init(x,0);
	
	return (x);
}

t_int *residency_perform(t_int *w)
{	
	int i, j;
	float sample;
	
	////////////////////////////////////////////// 
	t_residency *x = (t_residency *) (w[1]);
	t_float *in = (t_float *)(w[2]);
	t_float *increment = (t_float *)(w[3]);
	t_float *position = (t_float *)(w[4]);
	t_float *out = (t_float *)(w[5]);
	t_float *vec_sync = (t_float *) (w[6]);
	t_int n = w[7];
	
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
	float *input_vec = x->input_vec;
	float fframe = x->current_frame ;
	float last_fpos = x->last_fpos ;
	int framecount = x->framecount;
	float fincr = x->frame_increment;
	float fpos = x->fpos;
	float mult = x->mult;
	int *bitshuffle = x->bitshuffle;
	float *trigland = x->trigland ;
	float *c_lastphase_in = x->c_lastphase_in;
	float *c_lastphase_out = x->c_lastphase_out;
	float c_fundamental = x->c_fundamental;
	float c_factor_in = x->c_factor_in;
	float c_factor_out = x->c_factor_out;
	float sync = x->sync;
	
	if(x->lock || x->mute){
		while(n--){
			*out++ = 0.0;
			*vec_sync++ = sync;
		}
		return (w+8);
	}
	
#if MSP
	if (x->in2_connected) {
		fincr = *increment; 
	} 
	if (x->in3_connected) {
		fpos = *position; 
	} 
#endif
	
#if PD
	fincr = *increment;
	fpos = *position;
#endif
	
	inCount += D;
	
	for(i = 0; i < D; i++){
		input_vec[i] = in[i];
	}
	if(x->read_me) {
		for ( j = 0 ; j < Nw - D ; j++ ){
			input[j] = input[j+D];
		}
		for (i = 0,j = Nw - D; j < Nw; j++, i++) {
			input[j] = input_vec[i];
		}
		if(framecount > 0)
			sync = (float)x->frames_read/(float)framecount;
		
		
		if( x->playthrough ){
			for ( i = 0, j = Nw - D; j < Nw; j++, i++ ) {
				out[i] = input_vec[i] * 0.5;
				vec_sync[i] = sync;
			}
		}
		else { 
			for ( j = 0; j < D; j++ ){
				out[j] = 0.0;
				vec_sync[j] = sync;
			}
		}
		
		fold(input, Wanal, Nw, buffer, N, inCount);	
		rdft(N, 1, buffer, bitshuffle, trigland);
		
		
		if(x->frames_read >= framecount){
			x->read_me = 0;
			if(x->verbose){
				post("residency: data acquisition completed");
			}
		} else {
			convert(buffer, x->loveboat[(x->frames_read)++], N2, c_lastphase_in, c_fundamental, c_factor_in);
		}
		x->virgin = 0;
	} 
	else if(x->playthrough && x->virgin){
		for(i=0;i<D;i++){
			sync = 0;
			sample = input_vec[i] * 0.5;
			out[i] = sample;
			vec_sync[i] = sync;
		}
	}
	else {
		//sync = 1.0;
		
		if(fpos < 0)
			fpos = 0;
		if(fpos > 1)
			fpos = 1;
		if(fpos != last_fpos){
			fframe =  fpos * (float) framecount ;
			last_fpos = fpos;
		}
		
		
		fframe += fincr;
		while(fframe >= framecount) {
			fframe -= framecount;
		} 
		while( fframe < 0. ) {
			fframe += framecount ;
		}
		
		unconvert(x->loveboat[(int) fframe ], buffer, N2, c_lastphase_out, c_fundamental, c_factor_out);
		
		rdft( N, -1, buffer, bitshuffle, trigland );
		overlapadd( buffer, N, Wsyn, output, Nw, inCount );
		
		for(i = 0; i < D; i++){
			vec_sync[i] = sync;
		}
		for ( j = 0; j < D; j++ ){
			out[j] = output[j] * mult;
		}
		for ( j = 0; j < Nw - D; j++ ){
			output[j] = output[j+D];
		}
		
		for ( j = Nw - D; j < Nw; j++ ){
			output[j] = 0.;
		}
		
	}
	
	/* restore state variables */
	
	x->inCount = inCount % Nw;
	x->current_frame = fframe;
	x->frame_increment = fincr;
	x->fpos = fpos;
	x->last_fpos = last_fpos;
	x->sync = sync;
	return (w+8);
}		

#if MSP
void residency_float(t_residency *x, double f) // Look at floats at inlets
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

void residency_acquire_sample(t_residency *x)
{
	x->read_me = 1;
	x->frames_read = 0;
	post("beginning spectral data acquisition");
	return;
}

void residency_bangname (t_residency *x)
{
	x->read_me = 1;
	x->frames_read = 0;
	if(x->verbose)
		post("beginning spectral data acquisition");
	return;
}

void residency_mute(t_residency *x, t_floatarg tog)
{
	x->mute = tog;	
}

void residency_dsp(t_residency *x, t_signal **sp, short *count)
{
#if MSP
	x->in2_connected = count[1];
	x->in3_connected = count[2];
#endif
	
	if(x->R != sp[0]->s_sr || x->D != sp[0]->s_n){
		x->R = sp[0]->s_sr;
		x->D = sp[0]->s_n;
		if(x->verbose)
			post("new vsize: %d, new SR:%d",x->D,x->R);
		residency_init(x,1);
	}
	
	dsp_add(residency_perform, 7, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,  
			sp[3]->s_vec, sp[4]->s_vec,  sp[0]->s_n);
}

void residency_assist(t_residency *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal/bang) Input, Sampling Trigger"); break;
			case 1: sprintf(dst,"(signal/float) Frame Increment");break;
			case 2:sprintf(dst,"(signal/float) Frame Position [0-1]");break;
				
		}
	} else if (msg==2) {
		switch(arg){
			case 0: sprintf(dst,"(signal) Output"); break;
			case 1: sprintf(dst,"(signal) Recording Sync"); break;
		}
	}
}
