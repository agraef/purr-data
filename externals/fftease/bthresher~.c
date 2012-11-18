#include "MSPd.h"
#include "fftease.h"

#if MSP
void *bthresher_class;
#endif 

#if PD
static t_class *bthresher_class;
#endif

#define OBJECT_NAME "bthresher~"

typedef struct _bthresher
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
  float *buffer;
  float *channel;
  float *output;
  /* bthresher vars */
  float *move_threshold;
  float *composite_frame ;
  int *frames_left;
  int max_hold_frames;
  float max_hold_time;
  int first_frame;
  float *damping_factor ;
  float thresh_scalar;
  float damp_scalar;
  short thresh_connected;
  short damping_connected;
  void *list_outlet;
  void *misc_outlet;
  t_atom *list_data;

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
  float init_thresh;
  float init_damping;
  int overlap;
  int winfac;
  float tadv;
  short inf_hold;
} t_bthresher;

  
void *bthresher_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *bthresher_perform(t_int *w);
void bthresher_dsp(t_bthresher *x, t_signal **sp, short *count);
void bthresher_assist(t_bthresher *x, void *b, long m, long a, char *s);
void bthresher_float(t_bthresher *x, double f);
void bthresher_mute(t_bthresher *x, t_floatarg f);
void bthresher_bypass(t_bthresher *x, t_floatarg f);
void bthresher_overlap(t_bthresher *x, t_floatarg f);
void bthresher_winfac(t_bthresher *x, t_floatarg f);
void bthresher_fftinfo(t_bthresher *x);
void bthresher_free(t_bthresher *x);
void bthresher_bin(t_bthresher *x, t_floatarg bin_num, t_floatarg threshold, t_floatarg damper);
void bthresher_rdamper(t_bthresher *x, t_floatarg min, t_floatarg max );
void bthresher_rthreshold(t_bthresher *x, t_floatarg min, t_floatarg max);
void bthresher_dump(t_bthresher *x );
void bthresher_list (t_bthresher *x, t_symbol *msg, short argc, t_atom *argv);
void bthresher_init(t_bthresher *x, short initialized);
float bthresher_boundrand(float min, float max);
void bthresher_allthresh(t_bthresher *x, t_floatarg f);
void bthresher_alldamp(t_bthresher *x, t_floatarg f);
void bthresher_inf_hold(t_bthresher *x, t_floatarg f);
void bthresher_max_hold(t_bthresher *x, t_floatarg f);

#if MSP
void main(void)
{
  setup((t_messlist **)&bthresher_class, (method)bthresher_new, (method)bthresher_free, 
  	(short)sizeof(t_bthresher), 0, A_GIMME, 0);
  addmess((method)bthresher_dsp, "dsp", A_CANT, 0);
  addmess((method)bthresher_assist,"assist",A_CANT,0);
  addmess((method)bthresher_mute,"mute",A_FLOAT,0);
  addmess((method)bthresher_bypass,"bypass",A_FLOAT,0);
  addmess((method)bthresher_overlap,"overlap",A_FLOAT,0);
  addmess((method)bthresher_winfac,"winfac",A_FLOAT,0);
  addmess((method)bthresher_fftinfo,"fftinfo",0);
  addmess ((method)bthresher_bin, "bin", A_FLOAT, A_FLOAT, A_FLOAT, 0);
  addmess ((method)bthresher_rdamper, "rdamper", A_DEFFLOAT, A_DEFFLOAT, 0);
  addmess ((method)bthresher_rthreshold, "rthreshold", A_DEFFLOAT, A_DEFFLOAT, 0);
  addmess((method)bthresher_dump,"dump",0);
  addmess((method)bthresher_list,"list",A_GIMME,0);
  addmess((method)bthresher_alldamp,"alldamp",A_FLOAT,0);
  addmess((method)bthresher_allthresh,"allthresh",A_FLOAT,0);
  addmess((method)bthresher_inf_hold,"inf_hold",A_FLOAT,0);
  addmess((method)bthresher_max_hold,"max_hold",A_FLOAT,0);
  addfloat((method)bthresher_float);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif
#if PD
void bthresher_tilde_setup(void){
  bthresher_class = class_new(gensym("bthresher~"), (t_newmethod)bthresher_new, 
      (t_method)bthresher_free ,sizeof(t_bthresher), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(bthresher_class, t_bthresher, x_f);
  class_addmethod(bthresher_class,(t_method)bthresher_dsp,gensym("dsp"),0);
  class_addmethod(bthresher_class,(t_method)bthresher_mute,gensym("mute"),A_FLOAT,0);
  class_addmethod(bthresher_class,(t_method)bthresher_bypass,gensym("bypass"),A_FLOAT,0);
  class_addmethod(bthresher_class,(t_method)bthresher_overlap,gensym("overlap"),A_FLOAT,0);
  class_addmethod(bthresher_class,(t_method)bthresher_winfac,gensym("winfac"),A_FLOAT,0);
  class_addmethod(bthresher_class,(t_method)bthresher_fftinfo,gensym("fftinfo"),0);
  class_addmethod(bthresher_class,(t_method)bthresher_rdamper,gensym("rdamper"),A_FLOAT,A_FLOAT,0);
  class_addmethod(bthresher_class,(t_method)bthresher_rthreshold,gensym("rthreshold"),A_FLOAT,A_FLOAT,0);
  class_addmethod(bthresher_class,(t_method)bthresher_dump,gensym("dump"),0);
  class_addmethod(bthresher_class,(t_method)bthresher_list,gensym("list"),A_GIMME,0);
  class_addmethod(bthresher_class,(t_method)bthresher_alldamp,gensym("alldamp"),A_FLOAT,0);
  class_addmethod(bthresher_class,(t_method)bthresher_allthresh,gensym("allthresh"),A_FLOAT,0);
  class_addmethod(bthresher_class,(t_method)bthresher_inf_hold,gensym("inf_hold"),A_FLOAT,0);
  class_addmethod(bthresher_class,(t_method)bthresher_max_hold,gensym("max_hold"),A_FLOAT,0); 
  class_addmethod(bthresher_class,(t_method)bthresher_bin,gensym("bin"),A_FLOAT,A_FLOAT,A_FLOAT,0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void bthresher_fftinfo( t_bthresher *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}

void bthresher_free( t_bthresher *x ){
#if MSP
  dsp_free( (t_pxobject *) x);
#endif


  freebytes(x->Wanal,0); 
  freebytes(x->Wsyn,0);
  freebytes(x->Hwin,0);
  freebytes(x->buffer,0);
  freebytes(x->channel,0);
  freebytes(x->input,0);
  freebytes(x->output,0);
  freebytes(x->trigland,0); 
  freebytes(x->bitshuffle,0); 
  /* full phase vocoder */
  freebytes(x->c_lastphase_in,0); 
  freebytes(x->c_lastphase_out,0); 
  /* external-specific memory */
  freebytes(x->composite_frame,0);
  freebytes(x->frames_left,0);
  freebytes(x->move_threshold,0);
  freebytes(x->damping_factor,0);
  freebytes(x->list_data,0);
}

void bthresher_max_hold(t_bthresher *x, t_floatarg f)
{
	if(f<=0)
		return;
	x->max_hold_time = f * .001;
	x->max_hold_frames = x->max_hold_time / x->tadv;
}

void bthresher_inf_hold(t_bthresher *x, t_floatarg f)
{
	x->inf_hold = (int)f;
}

void bthresher_allthresh(t_bthresher *x, t_floatarg f)
{
int i;
//post("thresh %f",f);
	for(i=0;i<x->N2+1;i++)
		x->move_threshold[i] = f;
}

void bthresher_alldamp(t_bthresher *x, t_floatarg f)
{
int i;
//post("damp %f",f);
	for(i=0;i<x->N2+1;i++)
		x->damping_factor[i] = f;
}

void bthresher_overlap(t_bthresher *x, t_floatarg f)
{
int i = (int) f;
  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
	x->overlap = i;
	bthresher_init(x,1);
}

void bthresher_winfac(t_bthresher *x, t_floatarg f)
{
int i = (int)f;

  if(!power_of_two(i)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = i;
	bthresher_init(x,2);
}

void bthresher_mute(t_bthresher *x, t_floatarg f){
  x->mute = f;
}
void bthresher_bypass(t_bthresher *x, t_floatarg f){
  x->bypass = f;
}

void bthresher_assist (t_bthresher *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:sprintf(dst,"(signal) Input");break;
    case 1:sprintf(dst,"(signal/float) Threshold Scalar");break;
    case 2:sprintf(dst,"(signal/float) Damping Factor Scalar");break;
    }
  } else if (msg==2) {
    switch (arg) {
    case 0:sprintf(dst,"(signal) Output");break;
    case 1:sprintf(dst,"(list) Current State");break;
    }
  }
}

void bthresher_list (t_bthresher *x, t_symbol *msg, short argc, t_atom *argv) {
  int i, bin, idiv;
  float fdiv;
  float *damping_factor = x->damping_factor;
  float *move_threshold = x->move_threshold;

  //	post("reading %d elements", argc);
  idiv = fdiv = (float) argc / 3.0 ;
  if( fdiv - idiv > 0.0 ) {
    post("list must be in triplets");
    return;
  }
/*  for( i = 0; i < x->N2+1; i++) {
    move_threshold[i] = 0.0 ;
  }*/

  for( i = 0; i < argc; i += 3 ) {
    bin = atom_getintarg(i,argc,argv);
    damping_factor[bin] = atom_getfloatarg(i+1,argc,argv);
    move_threshold[bin] = atom_getfloatarg(i+2,argc,argv);
/*    bin = argv[i].a_w.w_long ;
    damping_factor[bin] = argv[i + 1].a_w.w_float;
    move_threshold[bin] = argv[i + 2].a_w.w_float;*/
  }
}

void bthresher_dump (t_bthresher *x) {

  t_atom *list_data = x->list_data;
  float *damping_factor = x->damping_factor;
  float *move_threshold = x->move_threshold;

  int i,j, count;
#if MSP
  for( i = 0, j = 0; i < x->N2 * 3 ; i += 3, j++ ) {
    SETLONG(list_data+i,j);
    SETFLOAT(list_data+(i+1),damping_factor[j]);
    SETFLOAT(list_data+(i+2),move_threshold[j]);		
  }	
#endif

#if PD
  for( i = 0, j = 0; i < x->N2 * 3 ; i += 3, j++ ) {
    SETFLOAT(list_data+i,(float)j);
    SETFLOAT(list_data+(i+1),damping_factor[j]);
    SETFLOAT(list_data+(i+2),move_threshold[j]);		
  }
#endif

  count = x->N2 * 3;
  outlet_list(x->list_outlet,0,count,list_data);

  return;
}

void *bthresher_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_bthresher *x = (t_bthresher *)newobject(bthresher_class);
  x->list_outlet = listout((t_pxobject *)x);
  dsp_setup((t_pxobject *)x,3);
  outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
    t_bthresher *x = (t_bthresher *)pd_new(bthresher_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->list_outlet = outlet_new(&x->x_obj,gensym("list"));
#endif


  x->D = sys_getblksize();
  x->R = sys_getsr();
  
  x->init_thresh = atom_getfloatarg(0,argc,argv);
  x->init_damping = atom_getfloatarg(1,argc,argv);
  x->overlap = atom_getintarg(2,argc,argv);
  x->winfac = atom_getintarg(3,argc,argv);
  
  bthresher_init(x,0);
  


  return (x);
}

void bthresher_init(t_bthresher *x, short initialized)
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

  x->mult = 1. / (float) x->N;
  x->N2 = (x->N)>>1;
  x->Nw2 = (x->Nw)>>1;
  x->in_count = -(x->Nw);
  x->c_fundamental =  (float) x->R/((x->N2)<<1 );
  x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
  x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
	      
	if(!initialized){
	  x->first_frame = 1;
	  x->max_hold_time = 60.0 ;
	  x->thresh_connected = 0;
	  x->damping_connected = 0;
	  x->thresh_scalar = 1;
	  x->damp_scalar = 1;
	  x->mute = 0;
	  x->bypass = 0;
	  x->inf_hold = 0;
	  x->Wanal = (float *) getbytes((MAX_Nw) * sizeof(float));	
	  x->Wsyn = (float *) getbytes((MAX_Nw) * sizeof(float));	
	  x->Hwin = (float *) getbytes((MAX_Nw) * sizeof(float));
	  x->input = (float *) getbytes((MAX_Nw) * sizeof(float));	
	  x->buffer = (float *) getbytes((MAX_N) * sizeof(float));
	  x->channel = (float *) getbytes(((MAX_N+2)) * sizeof(float));
	  x->output = (float *) getbytes((MAX_Nw) * sizeof(float));
	  x->bitshuffle = (int *) getbytes((MAX_N * 2) * sizeof(int));
	  x->trigland = (float *) getbytes((MAX_N * 2) * sizeof(float));

	  x->c_lastphase_in = (float *) getbytes((MAX_N2+1)* sizeof(float));
	  x->c_lastphase_out = (float *) getbytes((MAX_N2+1)* sizeof(float));
	  	  	  
	  x->composite_frame = (float *) getbytes( (MAX_N+2)* sizeof(float));
	  x->frames_left = (int *) getbytes((MAX_N+2)* sizeof(int));

	  // TRIPLETS OF bin# damp_factor threshold
	  x->list_data = (t_atom *) getbytes((MAX_N2 + 1) * 3 * sizeof(t_atom));

	  x->move_threshold = (float *) getbytes((MAX_N2+1)* sizeof(float));
	  x->damping_factor = (float *) getbytes((MAX_N2+1)* sizeof(float));
	  

	} 
  if(initialized == 0 || initialized == 1){
	  for(i = 0; i < x->N2+1; i++) {
	    x->move_threshold[i] = x->init_thresh;
	    x->damping_factor[i] = x->init_damping;
	  }
  }

	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));
	memset((char *)x->buffer,0,x->N * sizeof(float));
	memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
	memset((char *)x->c_lastphase_out,0,(x->N2+1) * sizeof(float));


  x->tadv = (float) x->D / (float) x->R;
  x->max_hold_frames = x->max_hold_time / x->tadv;
  init_rdft(x->N, x->bitshuffle, x->trigland);
  makehanning(x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D, 0);
}

void bthresher_rdamper(t_bthresher *x,  t_floatarg min, t_floatarg max)
{
  int i;	

  for( i = 0; i < x->N2; i++ ) {
    x->damping_factor[i] = bthresher_boundrand(min, max);
  }
}

void bthresher_rthreshold( t_bthresher *x,  t_floatarg min, t_floatarg max )
{
  int i;
  for( i = 0; i < x->N2; i++ ) {
    x->move_threshold[i] = bthresher_boundrand(min, max);
  }
}


void bthresher_bin(t_bthresher *x, t_floatarg bin_num, t_floatarg damper, t_floatarg threshold)
{
int bn = (int) bin_num;
  if( bn >= 0 && bn < x->N2 ){
//    post("setting %d to %f %f",bn,threshold,damper);
    x->move_threshold[bn] = threshold;
    x->damping_factor[bn] = damper;
  } else {
    post("bthresher~: %d is out of range", bn);
  }
}


t_int *bthresher_perform(t_int *w)
{
	
  float sample, outsamp ;
  int	i, j, on;
  t_bthresher *x = (t_bthresher *) (w[1]);
  float *in = (t_float *)(w[2]);
  float *inthresh = (t_float *)(w[3]);
  float *damping = (t_float *)(w[4]);
  float *out = (t_float *)(w[5]);
  t_int n = w[6];
	

  int *bitshuffle = x->bitshuffle;
  float *trigland = x->trigland;
  float mult = x->mult;	

  int in_count = x->in_count;
  int R = x->R;
  int N = x->N;
  int N2 = x->N2;
  int D = x->D;
  int Nw = x->Nw;
  float *Wanal = x->Wanal;
  float *Wsyn = x->Wsyn;
  float *damping_factor = x->damping_factor;
  float *move_threshold = x->move_threshold;
  float *input = x->input;
  float *output = x->output;
  float *buffer = x->buffer;
  float *channel = x->channel;
  float *composite_frame = x->composite_frame;
  int max_hold_frames = x->max_hold_frames;
  int *frames_left = x->frames_left;
  float thresh_scalar = x->thresh_scalar;
  float damp_scalar = x->damp_scalar;
  short inf_hold = x->inf_hold;
  
  if( x->mute ) {
    for( j = 0; j < D; j++) {
      *out++ = 0.0 ;
    }
  } else if ( x->bypass ) {
    for( j = 0; j < D; j++) {
      *out++ = *in++ * 0.5;
    }
  } else {
#if MSP
    if( x->thresh_connected ) {
      thresh_scalar = *inthresh++;
    }
    if( x->damping_connected ) {
      damp_scalar = *damping++;
    }
#endif

#if PD
      thresh_scalar = *inthresh++;
      damp_scalar = *damping++;
#endif	

    in_count += D;


    for ( j = 0 ; j < Nw - D ; j++ )
      input[j] = input[j+D];

    for ( j = Nw-D; j < Nw; j++ ) {
      input[j] = *in++;
    }

    fold( input, Wanal, Nw, buffer, N, in_count );
    rdft( N, 1, buffer, bitshuffle, trigland );
    convert( buffer, channel, N2, x->c_lastphase_in, x->c_fundamental, x->c_factor_in  );
	
    if( x->first_frame ){
      for ( i = 0; i < N+2; i++ ){
        composite_frame[i] = channel[i];
        x->frames_left[i] = max_hold_frames;
      }
      x->first_frame = 0;
    } else {
      if( thresh_scalar < .999 || thresh_scalar > 1.001 || damp_scalar < .999 || damp_scalar > 1.001 ) {
				for(i = 0, j = 0; i < N+2; i += 2, j++ ){
				  if( fabs( composite_frame[i] - channel[i] ) > move_threshold[j] * thresh_scalar|| frames_left[j] <= 0 ){
				    composite_frame[i] = channel[i];
				    composite_frame[i+1] = channel[i+1];
				    frames_left[j] = max_hold_frames;
				  } else {
				    if(!inf_hold){
				      --(frames_left[j]);
				    }
				    composite_frame[i] *= damping_factor[j] * damp_scalar;
				  }
				}

      } else {
				for( i = 0, j = 0; i < N+2; i += 2, j++ ){
				  if( fabs( composite_frame[i] - channel[i] ) > move_threshold[j] || frames_left[j] <= 0 ){
				    composite_frame[i] = channel[i];
				    composite_frame[i+1] = channel[i+1];
				    frames_left[j] = max_hold_frames;
				  } else {
				    if(!inf_hold){
				      --(frames_left[j]);
				    }
				    composite_frame[i] *= damping_factor[j];
				  }
				}
      }
    }

    unconvert(x->composite_frame, buffer, N2, x->c_lastphase_out, x->c_fundamental, x->c_factor_out);
    rdft(N, -1, buffer, bitshuffle, trigland);

    overlapadd(buffer, N, Wsyn, output, Nw, in_count);

    for ( j = 0; j < D; j++ )
      *out++ = output[j] * mult;

    for ( j = 0; j < Nw - D; j++ )
      output[j] = output[j+D];
			
    for ( j = Nw - D; j < Nw; j++ )
      output[j] = 0.;
  }
  x->in_count = in_count % Nw;
  x->thresh_scalar = thresh_scalar;
  x->damp_scalar = damp_scalar;
	
  return (w+7);
}		

#if MSP
void bthresher_float(t_bthresher *x, double f) // Look at floats at inlets
{
  int inlet = x->x_obj.z_in;
  int i;	
  if (inlet == 1)
    {
      x->thresh_scalar = f;
    } else if (inlet == 2)  {
      x->damp_scalar = f;
    }
}
#endif

void bthresher_dsp(t_bthresher *x, t_signal **sp, short *count)
{
#if MSP
  x->thresh_connected = count[1];
  x->damping_connected = count[2];
#endif
  if(sp[0]->s_n != x->D || x->R != sp[0]->s_sr){
    x->D = sp[0]->s_n;
    x->R = sp[0]->s_sr;
    bthresher_init(x,1);
  }
  dsp_add(bthresher_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
	  sp[0]->s_n);
}

float bthresher_boundrand( float min, float max) {
  float frand;
  frand = (float) (rand() % 32768)/ 32768.0;
  return (min + frand * (max-min) );
}

