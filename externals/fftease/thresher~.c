#include "MSPd.h"
#include "fftease.h"

#if MSP
void *thresher_class;
#endif 

#if PD
static t_class *thresher_class;
#endif

#define OBJECT_NAME "thresher~"

#define DEFAULT_HOLD (40.0)



typedef struct _thresher
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
	int	in_count;
	float *Wanal;	
	float *Wsyn;	
	float *input;	
	float *Hwin;
	//  float *winput;
	float *buffer;
	float *channel;
	float *output;
	/* thresher vars */
	float move_threshold;
	float *composite_frame ;
	int *frames_left;
	int max_hold_frames;
	float max_hold_time;
	int first_frame;
	float damping_factor ;
	short thresh_connected;
	short damping_connected;
	// for convert
	float *c_lastphase_in;
	float *c_lastphase_out;
	float c_fundamental;
	float c_factor_in;
	float c_factor_out;
	// for fast fft
	float mult; 
	float *trigland;
	int *bitshuffle;
	short mute;
	short bypass;
	int winfac;
	int overlap;
	float tadv;
} t_thresher;

void *thresher_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *thresher_perform(t_int *w);
void thresher_dsp(t_thresher *x, t_signal **sp, short *count);
void thresher_assist(t_thresher *x, void *b, long m, long a, char *s);
void thresher_float(t_thresher *x, double f);
void thresher_mute(t_thresher *x, t_floatarg f);
void thresher_bypass(t_thresher *x, t_floatarg f);
void thresher_free( t_thresher *x );
void thresher_overlap(t_thresher *x, t_floatarg f);
void thresher_winfac(t_thresher *x, t_floatarg f);
void thresher_fftinfo(t_thresher *x);
void thresher_init(t_thresher *x, short initialized);

#if MSP
void main(void)
{
	setup((t_messlist **)&thresher_class, (method)thresher_new, (method)thresher_free, 
		  (short)sizeof(t_thresher), 0L, A_GIMME, 0);
	addmess((method)thresher_dsp, "dsp", A_CANT, 0);
	addmess((method)thresher_assist,"assist",A_CANT,0);
	addmess((method)thresher_mute,"mute",A_FLOAT,0);
	addmess((method)thresher_bypass,"bypass",A_FLOAT,0);
	addmess((method)thresher_overlap,"overlap",A_DEFFLOAT,0);
	addmess((method)thresher_winfac,"winfac",A_DEFFLOAT,0);
	addmess((method)thresher_fftinfo,"fftinfo",0);
	addfloat((method)thresher_float);
	dsp_initclass();
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
	
}
#endif

#if PD
void thresher_tilde_setup(void)
{
	thresher_class = class_new(gensym("thresher~"), (t_newmethod)thresher_new, 
							   (t_method)thresher_free ,sizeof(t_thresher), 0,A_GIMME,0);
	
	CLASS_MAINSIGNALIN(thresher_class, t_thresher, x_f );
	class_addmethod(thresher_class, (t_method)thresher_dsp, gensym("dsp"), 0);
	class_addmethod(thresher_class, (t_method)thresher_mute, gensym("mute"), A_DEFFLOAT,0);
	class_addmethod(thresher_class, (t_method)thresher_bypass, gensym("bypass"), A_DEFFLOAT,0);
	class_addmethod(thresher_class, (t_method)thresher_assist, gensym("assist"), 0);
	class_addmethod(thresher_class,(t_method)thresher_overlap,gensym("overlap"),A_FLOAT,0);
	class_addmethod(thresher_class,(t_method)thresher_winfac,gensym("winfac"),A_FLOAT,0);
	class_addmethod(thresher_class,(t_method)thresher_fftinfo,gensym("fftinfo"),0);
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void thresher_overlap(t_thresher *x, t_floatarg f)
{
	int i = (int) f;
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->overlap = i;
	thresher_init(x,1);
}

void thresher_winfac(t_thresher *x, t_floatarg f)
{
	int i = (int)f;
	
	if(!power_of_two(i)){
		error("%f is not a power of two",f);
		return;
	}
	x->winfac = i;
	thresher_init(x,2);
}

void thresher_fftinfo(t_thresher *x)
{
	if( ! x->overlap ){
		post("zero overlap!");
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void thresher_free(t_thresher *x){
#if MSP
	dsp_free( (t_pxobject *) x);
#endif
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
	freebytes(x->composite_frame,0);
	freebytes(x->frames_left,0);
}
void thresher_mute(t_thresher *x, t_floatarg f){
	x->mute = (short)f;
}

void thresher_bypass(t_thresher *x, t_floatarg f){
	x->bypass = (short)f;
}

void thresher_assist (t_thresher *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Input");
				break;
			case 1:
				sprintf(dst,"(signal/float) Threshold");
				break;
			case 2:
				sprintf(dst,"(signal/float) Damping Factor");
				break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}


void *thresher_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
	t_thresher *x = (t_thresher *)newobject(thresher_class);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");
#endif
	
#if PD
    t_thresher *x = (t_thresher *)pd_new(thresher_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
#endif
	
	
	x->move_threshold = atom_getfloatarg(0, argc, argv);
	x->damping_factor = atom_getfloatarg(1, argc, argv);
	x->overlap = atom_getfloatarg( 2, argc, argv );
	x->winfac = atom_getfloatarg( 3, argc, argv );
	
	x->D = sys_getblksize();
	x->R = sys_getsr();
	
	thresher_init(x,0);
	return (x);
}

void thresher_init(t_thresher *x, short initialized)
{
	int i;
	
	if(!x->D)
		x->D = 256;
	if(!x->R)
		x->R = 44100;
	if(!power_of_two(x->overlap))
		x->overlap = 4;
	if(!power_of_two(x->winfac))
		x->winfac = 1;
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->in_count = -(x->Nw);
	x->mult = 1. / (float) x->N;
	x->tadv = (float) x->D / (float) x->R ;
	
	
	
	if(!initialized){
		x->mute = 0;
		x->bypass = 0;
		if(!x->damping_factor){
			x->damping_factor = .95;
		}
		x->first_frame = 1;
		x->move_threshold = .00001 ;
		x->max_hold_time = DEFAULT_HOLD ;
		x->max_hold_frames = x->max_hold_time / x->tadv;
		x->c_fundamental =  (float) x->R/( (x->N2)<<1 );
		x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
		x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
		
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
		x->composite_frame = (float *) getbytes( (MAX_N+2) * sizeof(float) );
		x->frames_left = (int *) getbytes( (MAX_N+2) * sizeof(int) );
		
	}
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
	memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));  
	memset((char *)x->frames_left,0,(x->N+2) * sizeof(float));
	
	init_rdft(x->N, x->bitshuffle, x->trigland);
	makehanning(x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
}

t_int *thresher_perform(t_int *w)
{
	float sample, outsamp ;
	int		    i,j;
	
	t_thresher *x = (t_thresher *) (w[1]);
	
	float *in = (t_float *)(w[2]);
	float *inthresh = (t_float *)(w[3]);
	float *damping = (t_float *)(w[4]);
	float *out = (t_float *)(w[5]);
	int n = (int)(w[6]);
	
	float	*input = x->input;
	float *output = x->output;
	float *buffer = x->buffer;
	float *Wanal = x->Wanal;
	float *Wsyn = x->Wsyn;
	float *channel = x->channel;
	float damping_factor = x->damping_factor;
	int max_hold_frames = x->max_hold_frames;
	int *frames_left = x->frames_left;
	float *composite_frame = x->composite_frame;
	float *c_lastphase_in = x->c_lastphase_in;
	float *c_lastphase_out = x->c_lastphase_out;
	float c_fundamental = x->c_fundamental;
	float c_factor_in = x->c_factor_in;
	float c_factor_out = x->c_factor_out;
	int *bitshuffle = x->bitshuffle;
	float *trigland = x->trigland;
	float mult = x->mult;	
	int in_count = x->in_count;
	int R = x->R;
	int N = x->N;
	int N2 = x->N2;
	int D = x->D;
	int Nw = x->Nw;
	float move_threshold = x->move_threshold;
	
	
	if( x->mute ) {
		for( j = 0; j < D; j++) {
			*out++ = 0.0 ;
		}
		return (w+7);
	} 
	
	if ( x->bypass ) {
		for( j = 0; j < D; j++) {
			*out++ = *in++ ;
		}
		return (w+7);
	} 
	
	
	
	
    if( x->thresh_connected ) {
		move_threshold = *inthresh ;
    }
    if( x->damping_connected ) {
		damping_factor = *damping ;
    }
	
	in_count += D;
	
	for ( j = 0 ; j < Nw - D ; j++ )
		input[j] = input[j+D];
	
	for ( j = Nw - D; j < Nw; j++ ) {
		input[j] = *in++;
	}
	
	fold( input, Wanal, Nw, buffer, N, in_count );
	
	rdft( N, 1, buffer, bitshuffle, trigland );
	
	convert( buffer, channel, N2, c_lastphase_in, c_fundamental, c_factor_in  );
	
	if( x->first_frame ){
		for ( i = 0; i < N+2; i++ ){
			composite_frame[i] = channel[i];
			frames_left[i] = max_hold_frames;
		}
		x->first_frame = 0;
	} else {
		for( i = 0; i < N+2; i += 2 ){
			if(fabs( composite_frame[i] - channel[i] ) > move_threshold ||
			   frames_left[i] <= 0 ){
				composite_frame[i] = channel[i];
				composite_frame[i+1] = channel[i+1];
				frames_left[i] = max_hold_frames;
			} else {
				--(frames_left[i]);
				composite_frame[i] *= damping_factor;
			}
		}
	}
	
	
    unconvert( composite_frame, buffer, N2, c_lastphase_out, c_fundamental, c_factor_out  );
    rdft( N, -1, buffer, bitshuffle, trigland );
	
    overlapadd( buffer, N, Wsyn, output, Nw, in_count );
	
    for ( j = 0; j < D; j++ )
		*out++ = output[j] * mult;
	
    for ( j = 0; j < Nw - D; j++ )
		output[j] = output[j+D];
	
    for ( j = Nw - D; j < Nw; j++ )
		output[j] = 0.;
	
    x->in_count = in_count;
    x->damping_factor = damping_factor;
	
    return (w+7);
}

#if MSP
void thresher_float(t_thresher *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		x->move_threshold = f;
    } else if (inlet == 2)  {
		x->damping_factor = f;
    }
}
#endif

void thresher_dsp(t_thresher *x, t_signal **sp, short *count)
{
#if MSP
	x->thresh_connected = count[1];
	x->damping_connected = count[2];
#endif
	
#if PD
	x->thresh_connected = 1;
	x->damping_connected = 1;
#endif
	
	if(sp[0]->s_n != x->D || x->R != sp[0]->s_sr){
		x->D = sp[0]->s_n;
		x->R = sp[0]->s_sr;
		thresher_init(x,1);
	}
	dsp_add(thresher_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
			sp[0]->s_n);
}

