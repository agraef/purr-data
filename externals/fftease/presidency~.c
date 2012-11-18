#include "MSPd.h"
#include "fftease.h"

#if MSP
void *presidency_class;
#endif 

#if PD
static t_class *presidency_class;
#endif

#define OBJECT_NAME "presidency~"

typedef struct _presidency
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
  float *local_frame; // needs mem allocation in init
  int framecount;
  float lo_freq;
  float hi_freq;
  float topfreq;
  float curfreq;
  float P;
  int lo_bin;
  int hi_bin;
  float i_vector_size;
  float vector_size;
  float synthesis_threshold;
  float pitch_increment;
  // oscillator
  float *table;
  float *bindex;
  float *lastamp;// stores last amplitudes for each bin
  float *lastfreq;// stores last frequencies
  float table_si;
  int table_length;
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
  short in4_connected;
  int overlap;
  int winfac;
  int hopsize;
  float duration;
  short lock;
  short verbose;
  short override;
  float sync;
} t_presidency;

void *presidency_new(t_symbol *s, int argc, t_atom *argv);
t_int *presidency_perform(t_int *w);
void presidency_dsp(t_presidency *x, t_signal **sp, short *count);
void presidency_assist(t_presidency *x, void *b, long m, long a, char *s);
void presidency_bangname(t_presidency *x) ;

void presidency_playthrough( t_presidency *x, t_floatarg tog) ;
void presidency_float(t_presidency *x, double f) ;
void presidency_int(t_presidency *x, long i);
void presidency_mute(t_presidency *x, t_floatarg tog);
void presidency_free(t_presidency *x);
void presidency_init(t_presidency *x, short initialized);
void presidency_size(t_presidency *x, t_floatarg newsize);
void presidency_winfac(t_presidency *x, t_floatarg factor);
void presidency_overlap(t_presidency *x, t_floatarg o);
void presidency_fftinfo(t_presidency *x) ;
void presidency_verbose(t_presidency *x, t_floatarg t);
void presidency_acquire_sample(t_presidency *x);
void presidency_low_freq(t_presidency *x, t_floatarg f);
void presidency_high_freq(t_presidency *x, t_floatarg f);
void presidency_calc_bins_from_freqs(t_presidency *x);
void presidency_abs_thresh(t_presidency *x, t_floatarg t);

#if MSP
void main(void)
{
  setup((t_messlist **)&presidency_class, (method)presidency_new, (method)presidency_free, 
	(short)sizeof(t_presidency), 0, A_GIMME,0);
  addmess((method)presidency_dsp, "dsp", A_CANT, 0);
  addmess((method)presidency_assist,"assist",A_CANT,0);
  addfloat((method)presidency_float);
  addint((method)presidency_int);
  addbang((method)presidency_bangname);
  addmess((method)presidency_mute, "mute", A_FLOAT, 0);
  addmess((method)presidency_fftinfo, "fftinfo",  0);
  addmess((method)presidency_playthrough, "playthrough", A_DEFFLOAT, 0);
  addmess((method)presidency_size, "size", A_DEFFLOAT, 0);
  addmess((method)presidency_overlap, "overlap", A_DEFFLOAT, 0);
  addmess((method)presidency_winfac, "winfac", A_DEFFLOAT, 0);
  addmess((method)presidency_verbose, "verbose", A_DEFFLOAT, 0);
  addmess((method)presidency_low_freq, "low_freq", A_DEFFLOAT, 0);
  addmess((method)presidency_high_freq, "high_freq", A_DEFFLOAT, 0);
  addmess((method)presidency_acquire_sample, "acquire_sample", 0);
  addmess((method)presidency_abs_thresh, "abs_thresh", A_FLOAT, 0);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void presidency_tilde_setup(void)
{
  presidency_class = class_new(gensym("presidency~"), (t_newmethod)presidency_new, 
			 (t_method)presidency_free ,sizeof(t_presidency), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(presidency_class, t_presidency, x_f);
  class_addmethod(presidency_class,(t_method)presidency_dsp,gensym("dsp"),0);
  class_addmethod(presidency_class,(t_method)presidency_mute,gensym("mute"),A_FLOAT,0);
  class_addmethod(presidency_class,(t_method)presidency_fftinfo,gensym("fftinfo"),0);
  class_addmethod(presidency_class,(t_method)presidency_playthrough,gensym("playthrough"),A_FLOAT,0); 
  class_addmethod(presidency_class,(t_method)presidency_size,gensym("size"),A_FLOAT,0);
  class_addmethod(presidency_class,(t_method)presidency_overlap,gensym("overlap"),A_FLOAT,0); 
  class_addmethod(presidency_class,(t_method)presidency_winfac,gensym("winfac"),A_FLOAT,0);  
  class_addmethod(presidency_class,(t_method)presidency_verbose,gensym("verbose"),A_FLOAT,0);
  class_addmethod(presidency_class,(t_method)presidency_acquire_sample,gensym("acquire_sample"),0);
  class_addmethod(presidency_class,(t_method)presidency_low_freq,gensym("low_freq"),A_FLOAT,0);
  class_addmethod(presidency_class,(t_method)presidency_high_freq,gensym("high_freq"),A_FLOAT,0);
  class_addmethod(presidency_class,(t_method)presidency_abs_thresh,gensym("abs_thresh"),A_FLOAT,0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void presidency_abs_thresh(t_presidency *x, t_floatarg f)
{
	if(f < 0){
		error("illegal value for threshold");
		return;
	}
	x->synthesis_threshold = f;
}

void presidency_low_freq(t_presidency *x, t_floatarg f)
{
	if(f > x->hi_freq || f < 0){
		error("illegal value for low freq");
		return;
	}
	x->lo_freq = f;
	presidency_calc_bins_from_freqs(x);
}

void presidency_high_freq(t_presidency *x, t_floatarg f)
{
	if(f < x->lo_freq || f > x->R/2.0){
		error("illegal value for high freq");
		return;
	}
	x->hi_freq = f;
	presidency_calc_bins_from_freqs(x);
}

void presidency_calc_bins_from_freqs(t_presidency *x)
{
  x->hi_bin = 1;  
  x->curfreq = 0;
  
  if( x->c_fundamental <= 0){
	error("we're not yet fully initialized. Try turning on DACs first");
	return;
  }
  
  while( x->curfreq < x->hi_freq ) {
    ++(x->hi_bin);
    x->curfreq += x->c_fundamental ;
  }

  x->lo_bin = 0;  
  x->curfreq = 0;
  while( x->curfreq < x->lo_freq ) {
    ++(x->lo_bin);
    x->curfreq += x->c_fundamental ;
  }

  if( x->hi_bin > x->N2)
    x->hi_bin = x->N2 ;
  if(x->lo_bin > x->hi_bin)
	x->lo_bin = x->hi_bin;
//  post("hibin: %d lobin %d",x->hi_bin, x->lo_bin);
}


void presidency_overlap(t_presidency *x, t_floatarg f)
{
int i = (int) f;
  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
	x->overlap = i;
	presidency_init(x,1);
}

void presidency_winfac(t_presidency *x, t_floatarg f)
{
int i = (int)f;

  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = i;
	presidency_init(x,2);
}

void presidency_fftinfo(t_presidency *x)
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, winfac %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void presidency_verbose(t_presidency *x, t_floatarg t)
{
  x->verbose = t;
}

void presidency_size(t_presidency *x, t_floatarg newsize)
{
//protect with DACs off?

  if(newsize > 0.0){//could be horrendous size, but that's the user's problem
  	x->duration = newsize/1000.0;
	presidency_init(x,1);  	
  }
}

void presidency_playthrough (t_presidency *x, t_floatarg tog)
{
  x->playthrough = tog;
}

void presidency_free(t_presidency *x){
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
  freebytes(x->local_frame,0);
}



void presidency_init(t_presidency *x, short initialized)
{
	int i;
	long oldsize,newsize;
	int oldN = x->N;
	int oldN2 = x->N2;
	int oldNw = x->Nw;
	int last_framecount = x->framecount;
	x->lock = 1;
	x->virgin = 1;
	
	if(!power_of_two(x->winfac))
		x->winfac = 1;
	if(!power_of_two(x->overlap))
		x->overlap = 4;
	if(!x->R)
		x->R = 44100;
	if(!x->D){
		x->D = 256;
		x->vector_size = x->D;
	}
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N, &x->Nw, OBJECT_NAME);
	
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->inCount = -(x->Nw);
	x->mult = 1. / (float) x->N;
//	post("mult %f N %d",x->mult,x->N);
	x->current_frame = 0;
	x->fpos = x->last_fpos = 0;
	x->tadv = (float)x->D/(float)x->R;
	x->c_fundamental = (float)x->R/((x->N2)<<1);
	x->c_factor_in = (float) x->R/((float)x->D * TWOPI);
	x->c_factor_out = TWOPI * (float)x->D / (float)x->R;
	x->table_length = 8192;
	x->table_si = (float) x->table_length / (float) x->R;
	x->pitch_increment = 1.0 * x->table_si;
	
	if( x->duration <= 0 ){
		x->duration = 1.0;
	}
	
	x->framecount =  x->duration / x->tadv ;
	x->hopsize = (float)x->N / x->overlap;
	x->read_me = 0;
	
	if(!initialized){
		x->mute = 0;
		x->in2_connected = 0;
		x->in3_connected = 0;
		x->sync = 0;
		x->playthrough = 0;
		x->frame_increment = 1.0;
		x->verbose = 0;
		x->table = (float *) getbytes(x->table_length * sizeof(float));
		x->Wanal = (float *) getbytes(MAX_Nw*sizeof(float));	
		x->Wsyn = (float *) getbytes(MAX_Nw*sizeof(float));	
		x->input = (float *) getbytes(MAX_Nw*sizeof(float));	
		x->Hwin = (float *) getbytes(MAX_Nw*sizeof(float));
		x->bindex = (float *) getbytes( (MAX_N+1) * sizeof(float) );
		x->buffer = (float *) getbytes(MAX_N*sizeof(float));
		x->channel = (float *) getbytes((MAX_N+2)*sizeof(float));
		x->output = (float *) getbytes(MAX_Nw*sizeof(float));
		x->bitshuffle = (int *) getbytes((MAX_N*2)*sizeof(int));
		x->trigland = (float *) getbytes((MAX_N*2)*sizeof(float));
		x->c_lastphase_in = (float *) getbytes((MAX_N2+1)*sizeof(float));
		x->c_lastphase_out = (float *) getbytes((MAX_N2+1)*sizeof(float));
		x->lastamp = (float *) getbytes((MAX_N+1) * sizeof(float));
		x->lastfreq = (float *) getbytes((MAX_N+1) * sizeof(float));
		x->local_frame = (float *) getbytes((MAX_N+2)*sizeof(float));
		x->loveboat = (float **) getbytes(x->framecount*sizeof(float *));
		

		/* here we stay with old reallocation approach and pray */
		for(i=0;i<x->framecount;i++){
			x->loveboat[i] = (float *) getbytes(((x->N)+2)*sizeof(float));
			if(x->loveboat[i] == NULL){
				error("memory error");
				return;
			}
			memset((char *)x->loveboat[i],0,(x->N+2)*sizeof(float));
		}
	} else if(initialized == 1) {
		//free and allocate
		oldsize = (oldN+2)*sizeof(float);
		for(i = 0; i < last_framecount; i++){
			freebytes(x->loveboat[i],oldsize) ;
		}
		oldsize = last_framecount*sizeof(float *);
		freebytes(x->loveboat,oldsize);
		x->loveboat = (float **) getbytes(x->framecount*sizeof(float *));  
		for(i=0;i<x->framecount;i++){
			x->loveboat[i] = (float *) getbytes((x->N+2)*sizeof(float));
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
	memset((char *)x->lastamp,0,(x->N+1)*sizeof(float));
	memset((char *)x->lastfreq,0,(x->N+1)*sizeof(float));
	memset((char *)x->bindex,0,(x->N+1)*sizeof(float));
	memset((char *)x->buffer,0,x->N * sizeof(float));
	if(!x->vector_size){
		post("zero vector size - something is really screwed up here!");
		return;
	}
	for ( i = 0; i < x->table_length; i++ ) {
		x->table[i] = (float) x->N * cos((float)i * TWOPI / (float)x->table_length);
	}	
	x->c_fundamental =  (float) x->R/(float)x->N ;
	x->c_factor_in =  (float) x->R/((float)x->vector_size * TWOPI);
	
	if( x->hi_freq < x->c_fundamental ) {
		x->hi_freq = x->topfreq ;
	}
	x->hi_bin = 1;  
	x->curfreq = 0;
	while( x->curfreq < x->hi_freq ) {
		++(x->hi_bin);
		x->curfreq += x->c_fundamental ;
	}
	
	x->lo_bin = 0;  
	x->curfreq = 0;
	while( x->curfreq < x->lo_freq ) {
		++(x->lo_bin);
		x->curfreq += x->c_fundamental ;
	}
	
	if( x->hi_bin > x->N2)
		x->hi_bin = x->N2 ;
	if(x->lo_bin > x->hi_bin)
		x->lo_bin = x->hi_bin;
	
	x->i_vector_size = 1.0/x->vector_size;
	x->pitch_increment = x->P*x->table_length/x->R;
	
	makewindows( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D);
	init_rdft( x->N, x->bitshuffle, x->trigland);

		  
	x->lock = 0;
}

void *presidency_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_presidency *x = (t_presidency *)newobject(presidency_class);
  dsp_setup((t_pxobject *)x,4);
  outlet_new((t_pxobject *)x, "signal");
  outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
  t_presidency *x = (t_presidency *)pd_new(presidency_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif

  x->duration = atom_getfloatarg(0,argc,argv)/1000.0;
  x->lo_freq = atom_getfloatarg(1,argc,argv);
  x->hi_freq = atom_getfloatarg(2,argc,argv);
  x->overlap = atom_getfloatarg(3,argc,argv);
  x->winfac = atom_getfloatarg(4,argc,argv);

  x->D = sys_getblksize();
  x->R = sys_getsr();
  x->vector_size = x->D;

  x->topfreq = 3000.; // default top freq
  if(!x->lo_freq){
	x->lo_freq = 0;
  }
  if(!x->hi_freq)
	x->hi_freq = 4000.0;
	
  presidency_init(x,0);

  return (x);
}

t_int *presidency_perform(t_int *w)
{	
  int i, j;
  float sample;

  ////////////////////////////////////////////// 
  t_presidency *x = (t_presidency *) (w[1]);
  t_float *in = (t_float *)(w[2]);
  t_float *speed = (t_float *)(w[3]);
  t_float *position = (t_float *)(w[4]);
  t_float *pinc = (t_float *)(w[5]);
  t_float *out = (t_float *)(w[6]);
  t_float *sync_vec = (t_float *)(w[7]);
  t_int n = w[8];
	
  int inCount = x->inCount;
  int R = x->R;
  int N = x->N;
  int N2 = x->N2;
  int D = x->D;
  int vector_size = x->D;
  int Nw = x->Nw;
  float *Wanal = x->Wanal;
  float *Wsyn = x->Wsyn;		
  float *input = x->input;
  float *output = x->output;
  float *buffer = x->buffer;
  float *channel = x->channel;
  float *local_frame = x->local_frame;
  float fframe = x->current_frame ;
  float last_fpos = x->last_fpos ;
  int framecount = x->framecount;
  float fincr = x->frame_increment;
  float fpos = x->fpos;
  float pitch_increment = x->pitch_increment;
  float mult = x->mult;
  int *bitshuffle = x->bitshuffle;
  float *trigland = x->trigland ;
  float *c_lastphase_in = x->c_lastphase_in;
  float *c_lastphase_out = x->c_lastphase_out;
  float c_fundamental = x->c_fundamental;
  float c_factor_in = x->c_factor_in;
  float c_factor_out = x->c_factor_out;
  float table_si = x->table_si;
  int lo_bin = x->lo_bin;
  int hi_bin = x->hi_bin;
  float i_vector_size = x->i_vector_size;
  float synthesis_threshold = x->synthesis_threshold;
  float *lastfreq = x->lastfreq;
  float *lastamp = x->lastamp;
  float *bindex = x->bindex;
  float *table = x->table;
  int table_length = x->table_length;
  float sync = x->sync;


  if(x->lock || x->mute){
  	while(n--){
  	  *out++ = 0.0;
  	  *sync_vec++ = sync;
  	}
  	return (w+9);
  }
  
#if MSP
  if (x->in2_connected) {
    fincr = *speed; 
  } 
  if (x->in3_connected) {
    fpos = *position; 
  } 
  if (x->in4_connected) {
    pitch_increment = *pinc * table_si; 
  } 
#endif

#if PD
  fincr = *speed;
  fpos = *position;
  pitch_increment = *pinc * table_si;
#endif

  inCount += D;

  if(x->read_me) {
    for ( j = 0 ; j < Nw - D ; j++ ){
      input[j] = input[j+D];
    }
    for (j = Nw - D; j < Nw; j++) {
      input[j] = *in++;
    }

    if( x->playthrough ){
      for ( j = N-D; j < N; j++ ) {
	    *out++ = input[j] * 0.5;
      }
    }
    else { 
      for ( j = 0; j < D; j++ ){
	    *out++ = 0.0;
      }
    }

    fold(input, Wanal, Nw, buffer, N, inCount);	
    rdft(N, 1, buffer, bitshuffle, trigland);
    sync = (float) x->frames_read / (float) framecount;
    if(x->frames_read >= framecount){
      sync = 1.0;
      x->read_me = 0;
      if(x->verbose){
      	post("presidency: data acquisition completed");
      }
    } else {
      convert(buffer, x->loveboat[(x->frames_read)++], N2, c_lastphase_in, c_fundamental, c_factor_in);
    }
    x->virgin = 0;
    for ( j = 0; j < D; j++ ){
	    *sync_vec++ = sync;
    }

  } 
  else if(x->playthrough && x->virgin){
      while(n--){
	  	 sample = *in++ * 0.5;
		   *out++ = sample;
		   *sync_vec++ = sync;
      }
  }
  /* synthesis section */
  else {
    if(fpos < 0)
      fpos = 0;
    if(fpos > 1)
      fpos = 1;
    if(fpos != last_fpos){
      fframe =  fpos * (float) framecount;
      last_fpos = fpos;
    }


    fframe += fincr;
    while(fframe >= framecount) {
      fframe -= framecount;
    } 
    while( fframe < 0. ) {
      fframe += framecount ;
    }
	// copy stored frame to local for possible modifications
    for(i = 0; i < N; i++){
		local_frame[i] = x->loveboat[(int) fframe ][i];
	}

	for( i = lo_bin * 2 + 1; i < hi_bin * 2 + 1; i += 2 ){
	  local_frame[i] *= pitch_increment;
    }

  bloscbank(local_frame, output, D, i_vector_size, lastfreq, lastamp, 
  	bindex, table, table_length, synthesis_threshold, lo_bin, hi_bin);

    for ( j = 0; j < D; j++ ){
      *out++ = output[j] * mult;
      *sync_vec++ = sync;
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
  x->pitch_increment = pitch_increment;
  x->sync = sync;
  return (w+9);
}		

#if MSP
void presidency_int(t_presidency *x, long i)
{
	presidency_float(x,(float)i);
}

void presidency_float(t_presidency *x, double f) // Look at floats at inlets
{
  int inlet = ((t_pxobject*)x)->z_in;
	
  if (inlet == 1) {
      x->frame_increment = f;
  }
  else if (inlet == 2){
      if (f < 0 ){
      	f = 0;
      } else if(f > 1) {
       	f = 1.;
      }
      x->fpos = f;
  } 
  else if( inlet == 3 ){
  	  x->pitch_increment = f * x->table_si;
  }
}
#endif

void presidency_acquire_sample(t_presidency *x)
{
  x->read_me = 1;
  x->frames_read = 0;
  if(x->verbose)
	post("%: beginning spectral data acquisition",OBJECT_NAME);
}

void presidency_bangname (t_presidency *x)
{
  x->read_me = 1;
  x->frames_read = 0;
  if(x->verbose)
  	post("%s: beginning spectral data acquisition",OBJECT_NAME);
}

void presidency_mute(t_presidency *x, t_floatarg tog)
{
  x->mute = (short)tog;	
}

void presidency_dsp(t_presidency *x, t_signal **sp, short *count)
{
#if MSP
  x->in2_connected = count[1];
  x->in3_connected = count[2];
  x->in4_connected = count[3];
#endif
		
  if(x->R != sp[0]->s_sr || x->D != sp[0]->s_n){
    x->R = sp[0]->s_sr;
    x->D = sp[0]->s_n;
  	x->vector_size = x->D;
    if(x->verbose)
      post("new vsize: %d, new SR:%d",x->D,x->R);
    presidency_init(x,1);
  }
  
  dsp_add(presidency_perform, 8, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,  
	  sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec, sp[0]->s_n);
}

void presidency_assist(t_presidency *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0: sprintf(dst,"(signal) Input"); break;
    case 1: sprintf(dst,"(signal/float) Frame Increment"); break;
    case 2: sprintf(dst,"(signal/float) Frame Position [0-1]"); break;
	  case 3: sprintf(dst,"(signal/float) Transposition Factor"); break;
    }
  } else if (msg==2) {
    switch (arg) {
    case 0: sprintf(dst,"(signal) Output"); break;
    case 1: sprintf(dst,"(signal/float) Record Sync"); break;
    }
  }
}
