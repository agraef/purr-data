#ifndef PD

#include "MSPd.h"
#include "fftease.h"
/* #include "buffer.h" */

/* Not ported to Pd due to array/buffer difference */
// #define FLEN 1024

#if MSP
void *resident_class;
#endif 

#if PD
static t_class *resident_class;
#endif

#define OBJECT_NAME "residency_buffer~"

typedef struct _resident
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
	int MAXFRAMES;
	short mute;
	short in2_connected;
	short in3_connected;
	int buffer_frame_count;
	// buffer
	t_symbol *l_sym;
	t_buffer *l_buf;
	short initialized;
	float *tmpframe;
	int hopsize;
	int overlap;
	int winfac;
	short playthrough;
	float sync;
	short buffer_is_hosed;
} t_resident;

void *resident_new(t_symbol *msg, short argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *resident_perform(t_int *w);
void resident_dsp(t_resident *x, t_signal **sp, short *count);
void resident_assist(t_resident *x, void *b, long m, long a, char *s);
void resident_bangname ( t_resident *x ) ;
void resident_meminfo( t_resident *x ) ;
void resident_float(t_resident *x, double f) ;
void resident_mute(t_resident *x, long toggle);
void resident_calcbuf(t_resident *x, double desired_duration);
void resident_dsp_free( t_resident *x );
void resident_fftinfo(t_resident *x);
void resident_winfac(t_resident *x, t_floatarg f);
void resident_playthrough(t_resident *x, t_floatarg f);
void resident_overlap(t_resident *x, t_floatarg f);
resident_init(t_resident *x, short initialized);


void main(void)
{
	setup((t_messlist **)&resident_class, (method)resident_new, (method)resident_dsp_free, (short)sizeof(t_resident), 0L, A_GIMME,0);
	addmess((method)resident_dsp, "dsp", A_CANT, 0);
	addmess((method)resident_assist,"assist",A_CANT,0);
	addfloat((method) resident_float);
	addbang( (method) resident_bangname );
	addmess ((method)resident_mute, "mute", A_LONG, 0);
	addmess ((method)resident_fftinfo, "meminfo", 0);
	addmess ((method)resident_calcbuf, "calcbuf", A_FLOAT, 0);
	addmess ((method)resident_winfac, "winfac", A_FLOAT, 0);
	addmess ((method)resident_overlap, "overlap", A_FLOAT, 0);
	addmess ((method)resident_playthrough, "playthrough", A_FLOAT, 0);
	addmess ((method)resident_fftinfo, "fftinfo", 0);
	dsp_initclass();
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}

void resident_meminfo( t_resident *x )
{
    post("%d frames in buffer", x->buffer_frame_count);
    post("frame_duration: %f, actual time in buffer: %f", x->tadv, (float)(x->buffer_frame_count) * x->tadv);
    post("actual time in buffer: %f", (float)(x->buffer_frame_count) * x->tadv);   
}



void resident_fftinfo(t_resident *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void resident_dsp_free( t_resident *x ){
	dsp_free( (t_pxobject *) x);
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
	freebytes(x->tmpframe,0);
	
}

void resident_calcbuf(t_resident *x, double desired_duration)
{
	float ms_calc;
	float frames_needed;
	float seconds;
	float frames;
	float samples;
	
	seconds = desired_duration / 1000.0;
	frames = seconds / x->tadv;
	samples = frames * (float) (x->N + 2);
	ms_calc = (samples / x->R) * 1000.0;
	post("you need %.0f milliseconds in buffer to get %.0f frames", ms_calc, frames);
	
}

void resident_assist (t_resident *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal/bang) Input, Sampling Trigger");
				break;
			case 1:
				sprintf(dst,"(signal/float) Frame Increment");
				break;
			case 2:
				sprintf(dst,"(signal/float) Frame Position [0-1]");
				break;
				
		}
	} else if (msg==2) {
		switch(arg) {
			case 1: sprintf(dst,"(signal) Output"); break;
			case 2: sprintf(dst,"(signal) Record Sync"); break;
		}
	}
}

void *resident_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_resident *x = (t_resident *)newobject(resident_class);
	
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");
	outlet_new((t_pxobject *)x, "signal");
	
	x->D = sys_getblksize();
	x->R = sys_getsr();
	
	/* args: bufname, overlap, winfac */
	
	x->l_sym = atom_getsymarg(0, argc, argv);
	x->overlap = atom_getintarg(1, argc, argv);
	x->winfac = atom_getintarg(2, argc, argv);
	
//	post("argc is %d", argc); it is the number of arguments, not including name of external
	if(argc < 1){
		error("%s: you must provide the name of a valid buffer.",OBJECT_NAME);
		x->x_obj.z_disabled = 1;
		return 0; // kills object for good
	} else {
		x->x_obj.z_disabled = 0;
	}
	
	resident_init(x,0);  
	return (x);
}

resident_init(t_resident *x, short initialized)
{
	int i;
	if(!power_of_two(x->overlap))
		x->overlap = 4;
	if(!power_of_two(x->winfac))
		x->winfac = 1;
	
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->hopsize = x->D;
	x->tadv = (float) x->D / (float) x->R;	
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->inCount = -(x->Nw);
	x->mult = 1. / (float) x->N;
	x->c_fundamental =  (float) x->R/( (x->N2)<<1 );
	x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
	x->c_factor_out =TWOPI * (float)  x->D / (float) x->R;
	
	if(!initialized){
		x->mute = 0;
		x->sync = 0;
		x->initialized = 1;
		x->current_frame = x->framecount = 0;
		x->frame_increment = 1.0 ;
		x->fpos = x->last_fpos = 0;
		x->Wanal = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->Wsyn = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->Hwin = (float *) getbytes( MAX_Nw * sizeof(float) );
		x->input = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->output = (float *) getbytes( MAX_Nw * sizeof(float) );
		x->buffer = (float *) getbytes( MAX_N * sizeof(float) );
		x->channel = (float *) getbytes( (MAX_N+2) *  sizeof(float) );
		x->tmpframe = (float *) getbytes( (MAX_N+2) * sizeof(float) );
		x->bitshuffle = (int *) getbytes( (MAX_N * 2) * sizeof( int ) );
		x->trigland = (float *) getbytes( (MAX_N * 2) * sizeof( float ) );
		x->c_lastphase_in = (float *) getbytes( (MAX_N2+1) * sizeof(float) );
		x->c_lastphase_out = (float *) getbytes( (MAX_N2+1) * sizeof(float) );
	}
	
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
	memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));
	
	init_rdft( x->N, x->bitshuffle, x->trigland); 	
	makewindows( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D);
}

t_int *resident_perform(t_int *w)
{
	float sample, outsamp ;	
	int index_offset;
	int i, j;
	
	////////////////////////////////////////////// 
	t_resident *x = (t_resident *) (w[1]);
	t_float *in = (t_float *)(w[2]);
	t_float *increment = (t_float *)(w[3]);
	t_float *position = (t_float *)(w[4]);
	t_float *out = (t_float *)(w[5]);
	t_float *vec_sync = (t_float *)(w[6]);
	t_int n = w[7];
	
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
	float fframe = x->current_frame ;
	float fincr = x->frame_increment;
	float fpos = x->fpos;
	float last_fpos = x->last_fpos ;
	int framecount = x->framecount;
	float sync = x->sync;
	
	
	float mult = x->mult ;
	int *bitshuffle = x->bitshuffle;
	float *trigland = x->trigland ;
	
	float *c_lastphase_in = x->c_lastphase_in;
	float *c_lastphase_out = x->c_lastphase_out;
	float c_fundamental = x->c_fundamental;
	float c_factor_in = x->c_factor_in;
	float c_factor_out = x->c_factor_out;
	
	float *tmpframe = x->tmpframe;
	
	t_buffer *l_buf = x->l_buf;
	
	int frames_read = x->frames_read;
	int buffer_frame_count = x->buffer_frame_count;
	
	if (x->in2_connected) {
		fincr = *increment++; 
	} 
	
	if (x->in3_connected) {
		fpos = *position++; 
	}
	
	inCount += D;

	if( (! x->initialized) || x->mute || x->x_obj.z_disabled ) {
	    for ( j = 0; j < D; j++ ){
      		*out++ = 0.0;
      		*vec_sync++ = sync;
		}
		return (w+8); // must be index of "n" + 1
	}
	if( x->read_me ) {
		for ( j = 0 ; j < Nw - D ; j++ ){
			input[j] = input[j+D];
		}
		for ( j = Nw - D; j < Nw; j++ ) {
			input[j] = *in++;
		}
		
		fold( input, Wanal, Nw, buffer, N, inCount );	
		rdft( N, 1, buffer, bitshuffle, trigland );
		
		convert( buffer, tmpframe, N2, c_lastphase_in, c_fundamental, c_factor_in );
		
		index_offset = (N+2) * frames_read;
		
		for( i = index_offset, j = 0; i < index_offset + N + 2; i++, j++ ){
			l_buf->b_samples[i] = tmpframe[j];
		}
		
		++frames_read;
		// output empty buffers while reading
		sync = (float)frames_read/(float)(x->buffer_frame_count);
		
		if(x->playthrough){
			for ( i=0, j = Nw - D; j < Nw; j++, i++ ) {
				out[i] = input[j];
				vec_sync[j] = sync;
			}    	
		} else {
			for ( j = 0; j < D; j++ ){
				out[j] = 0.0;
				vec_sync[j] = sync;
			}
		}
		
		if( frames_read >= x->buffer_frame_count){
			x->read_me = 0;
			//     post("resident_buffer: data acquisition completed");
		} 
		
	} else if (x->mute ) {
		// Process Muted
		for ( j = 0; j < D; j++ ){
			out[j] = 0.0;
			vec_sync[j] = sync;
		}
	}
	else {
		
		if( fpos < 0 )
			fpos = 0;
		if( fpos > 1 )
			fpos = 1;
		if( fpos != last_fpos ){
			fframe =  fpos * (float) buffer_frame_count ;
			last_fpos = fpos;
		}
		
		
		fframe += fincr;
		while( fframe >= buffer_frame_count ) {
			fframe -= buffer_frame_count;
		} 
		while( fframe < 0. ) {
			fframe += buffer_frame_count ;
		}
		
		index_offset = (N+2) * (int) fframe;
		
		for( i = index_offset, j = 0; i < index_offset + N + 2; i++, j++ ){
			tmpframe[j] = l_buf->b_samples[i];
		}
		// REPLACE loveboat with buffer
		unconvert(  tmpframe, buffer, N2, c_lastphase_out, c_fundamental, c_factor_out );
		
		rdft( N, -1, buffer, bitshuffle, trigland );
		overlapadd( buffer, N, Wsyn, output, Nw, inCount );
		
		for ( j = 0; j < D; j++ ){
			*out++ = output[j] * mult;
		}
		for ( j = 0; j < Nw - D; j++ ){
			output[j] = output[j+D];
		}
		
		for ( j = Nw - D; j < Nw; j++ ){
			output[j] = 0.;
			vec_sync[0] = sync;
		}
	}
	
	/* restore state variables */
	
	x->inCount = inCount % Nw;
	x->current_frame = fframe;
	x->frame_increment = fincr;
	x->fpos = fpos;
	x->last_fpos = last_fpos;
	x->frames_read = frames_read;
	x->sync = sync;
	
	return (w+8);
}		
void resident_float(t_resident *x, double f) // Look at floats at inlets
{
	//  int inlet = ((t_pxobject*)x)->z_in;
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
void resident_bangname ( t_resident *x )
{
	// int i, j;
	
	x->read_me = 1;
	x->frames_read = 0;
	//  post("resident_buffer: beginning spectral data acquisition");
	return;
	
}

void resident_mute(t_resident *x, long toggle)
{
	x->mute = (short)toggle;	
}

void resident_playthrough(t_resident *x, t_floatarg toggle)
{
	x->playthrough = (short)toggle;	
}

void resident_winfac(t_resident *x, t_floatarg f)
{
	int i = (int)f;
	
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = i;
	resident_init(x,2);
}

void resident_overlap(t_resident *x, t_floatarg f)
{
	int i = (int) f;
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->overlap = i;
	resident_init(x,1);
}


void resident_dsp(t_resident *x, t_signal **sp, short *count)
{
	long i;
	int buffer_samples;
	t_buffer *b;
  	
	x->in2_connected = count[1];
	x->in3_connected = count[2];
	
	if(x->R != sp[0]->s_sr || x->D != sp[0]->s_n){
		x->R = sp[0]->s_sr;
		x->D = sp[0]->s_n;
		resident_init(x,1);
	}
  	/*	
  	post("value of disabled is: %d",x->x_obj.z_disabled);
  	post("value of initialized is: %d",x->initialized);
  	*/
/*  	if(x->l_sym->s_name == ""){
  		post("this buffer was not even dignified with a name");
  	} */
  	  	/*
  	if(!x->l_sym->s_thing){
  		error("residency_buffer~: not linked to a valid buffer");
  	} */
//	else if(!x->initialized){
		
		if ((b = (t_buffer *)(x->l_sym->s_thing)) && ob_sym(b) == gensym("buffer~")) {
			x->l_buf = b;
			x->initialized = 1;
			if( x->l_buf->b_nchans != 1 ){
				error("resident_buffer~: buffer \"%s\" must have 1 channel, not %d", x->l_sym->s_name, x->l_buf->b_nchans);
				x->x_obj.z_disabled = 1;
			}
		}	
		else {
			error("%s: buffer \"%s\" not found",OBJECT_NAME, x->l_sym->s_name);
			x->x_obj.z_disabled = 1;
		}	
	
		if( ! x->x_obj.z_disabled ){
			x->buffer_frame_count = (float) (x->l_buf->b_frames) / (float)(x->N + 2 );
		}
		
//	}
	
	
	dsp_add(resident_perform, 7, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,  
			sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
			
}



#endif /* PD */
