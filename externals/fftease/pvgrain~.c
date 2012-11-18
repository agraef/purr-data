#include "MSPd.h"
#include "fftease.h"

#if MSP
void *pvgrain_class;
#endif 

#if PD
static t_class *pvgrain_class;
#endif

#define OBJECT_NAME "pvgrain~"

typedef struct _pvgrain
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
  // for convert
  float *c_lastphase_in;
  float c_fundamental;
  float c_factor_in;
  float c_factor_out;
float synt;
  // for oscbank
  int NP;
  float P;
  int L;
  int first;
  float Iinv;
  float *lastamp;
  float *lastfreq;
  float *index;
  float *table;
  short *binsort;
  float myPInc;
  float ffac;
  //
  int lo_bin;
  int hi_bin;
  float topfreq;
  float bottomfreq;
  // for fast fft
  float mult; 
  float *trigland;
  int *bitshuffle;
  //
  int mute;
  float grain_probability;
  float sample_basefreq;
  int grains_per_frame ;
  void *list_outlet;
  float *listdata;
  short list_count;
  void *m_clock;
  int overlap;//overlap factor
  int winfac;//window factor
  int vs;//vector size

} t_pvgrain;

void *pvgrain_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *pvgrain_perform(t_int *w);
void pvgrain_dsp(t_pvgrain *x, t_signal **sp, short *count);
void pvgrain_assist(t_pvgrain *x, void *b, long m, long a, char *s);
void pvgrain_mute(t_pvgrain *x, t_floatarg state);
void pvgrain_float(t_pvgrain *x, double f);
void pvgrain_tick(t_pvgrain *x);
void pvgrain_printchan(t_pvgrain *x);
void pvgrain_probability (t_pvgrain *x, t_floatarg prob);
void pvgrain_framegrains (t_pvgrain *x, t_floatarg grains);
void pvgrain_topfreq (t_pvgrain *x, t_floatarg top);
void pvgrain_bottomfreq (t_pvgrain *x, t_floatarg f);
void pvgrain_basefreq (t_pvgrain *x, t_floatarg base);
float pvgrain_randf(float min, float max);
void pvgrain_init(t_pvgrain *x, short initialized);
void pvgrain_free(t_pvgrain *x);
void pvgrain_winfac(t_pvgrain *x, t_floatarg factor);
void pvgrain_overlap(t_pvgrain *x, t_floatarg o);
void pvgrain_fftinfo(t_pvgrain *x) ;

#if MSP
void main(void)
{
  setup((t_messlist **)&pvgrain_class, (method)pvgrain_new, (method)pvgrain_free, 
	(short)sizeof(t_pvgrain), 0, A_GIMME, 0);
  addmess((method)pvgrain_dsp, "dsp", A_CANT, 0);
  addmess((method)pvgrain_assist,"assist",A_CANT,0);
  addmess((method)pvgrain_mute,"mute",A_DEFFLOAT,0);
  addmess((method)pvgrain_printchan,"printchan",A_DEFFLOAT,0);
  addmess((method)pvgrain_probability,"probability",A_DEFFLOAT,0);
  addmess((method)pvgrain_framegrains,"framegrains",A_DEFFLOAT,0);
  addmess((method)pvgrain_topfreq,"topfreq",A_DEFFLOAT,0);
  addmess((method)pvgrain_basefreq,"basefreq",A_DEFFLOAT,0);
  addmess((method)pvgrain_overlap, "overlap",  A_DEFFLOAT, 0);
  addmess((method)pvgrain_winfac, "winfac",  A_DEFFLOAT, 0);
  addmess((method)pvgrain_fftinfo, "fftinfo", 0);
  dsp_initclass();
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);}
#endif
#if PD
void pvgrain_tilde_setup(void)
{
  pvgrain_class = class_new(gensym("pvgrain~"), (t_newmethod)pvgrain_new, 
			 (t_method)pvgrain_free ,sizeof(t_pvgrain), 0, A_GIMME, 0);
  CLASS_MAINSIGNALIN(pvgrain_class, t_pvgrain, x_f);
  class_addmethod(pvgrain_class, (t_method)pvgrain_dsp, gensym("dsp"), 0);
  class_addmethod(pvgrain_class, (t_method)pvgrain_mute, gensym("mute"), A_FLOAT, 0);
  class_addmethod(pvgrain_class, (t_method)pvgrain_topfreq, gensym("topfreq"), A_FLOAT, 0);
  class_addmethod(pvgrain_class, (t_method)pvgrain_bottomfreq, gensym("bottomfreq"), A_FLOAT, 0);
  class_addmethod(pvgrain_class, (t_method)pvgrain_printchan, gensym("printchan"), A_FLOAT, 0);
  class_addmethod(pvgrain_class, (t_method)pvgrain_probability, gensym("probability"), A_FLOAT, 0);
  class_addmethod(pvgrain_class, (t_method)pvgrain_framegrains, gensym("framegrains"), A_FLOAT, 0);
  class_addmethod(pvgrain_class, (t_method)pvgrain_basefreq, gensym("basefreq"), A_FLOAT, 0);
  class_addmethod(pvgrain_class, (t_method)pvgrain_overlap, gensym("overlap"), A_DEFFLOAT,0);
  class_addmethod(pvgrain_class, (t_method)pvgrain_winfac, gensym("winfac"), A_DEFFLOAT,0);
  class_addmethod(pvgrain_class, (t_method)pvgrain_fftinfo, gensym("fftinfo"), 0);
  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void pvgrain_printchan(t_pvgrain *x)
{
  int i;
  post("***");
  for( i = 0 ; i < 30; i+= 2 ){
    post("amp %f freq %f", x->channel[i*2], x->channel[i*2 + 1]);
  }
  post("***");
}

void pvgrain_basefreq (t_pvgrain *x, t_floatarg base)
{
  if( base < 0.0 )
    base = 0. ;
  if( base > x->R / 2 )
    base = x->R / 2 ;
  x->sample_basefreq = base;
}

void pvgrain_topfreq (t_pvgrain *x, t_floatarg top)
{
  float curfreq ;
  if( top < 50. )
    top = 50.;
  if( top > x->R / 2 )
    top = x->R / 2;
  x->topfreq = top ;
  curfreq = 0;
  x->hi_bin = 0;
  while( curfreq < x->topfreq ) {
    ++(x->hi_bin);
    curfreq += x->c_fundamental ;
  }
}

void pvgrain_bottomfreq (t_pvgrain *x, t_floatarg f)
{
  float curfreq;
  if( f >= x->topfreq || f >= x->R/2){
	post("%f is too high a bottom freq",f);
	return;
  }

  x->bottomfreq = f;
  curfreq = 0;
  x->lo_bin = 0;
  while( curfreq < x->bottomfreq ) {
    ++(x->lo_bin);
    curfreq += x->c_fundamental ;
  }
//  post("low bin: %d high bin: %d",x->lo_bin,x->hi_bin);
}

void pvgrain_probability (t_pvgrain *x, t_floatarg prob)
{
	
  if( prob < 0. )
    prob = 0.;
  if( prob > 1. )
    prob = 1.;
  x->grain_probability = prob ;
}

void pvgrain_framegrains (t_pvgrain *x, t_floatarg grains)
{
	
  if( grains < 1 )
    grains = 1;
  if( grains > x->N2 - 1 )
    grains = x->N2 - 1;
  x->grains_per_frame = grains ;
	
}

void pvgrain_tick(t_pvgrain *x)
{
  t_atom myList[2];
  float *listdata = x->listdata;
  int i;

  for (i=0; i < 2; i++) {
    SETFLOAT(myList+i,listdata[i]); /* macro for setting a t_atom */
     
  }
  outlet_list(x->list_outlet,0L,2,myList);
  
}

void pvgrain_assist (t_pvgrain *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:
      sprintf(dst,"(signal) Input");
      break;

    }
  } else if (msg==2) {
    sprintf(dst,"(list) Amplitude/Frequency Pairs");
  }
}

void *pvgrain_new(t_symbol *s, int argc, t_atom *argv)
{
#if MSP
  t_pvgrain *x = (t_pvgrain *)newobject(pvgrain_class);
  dsp_setup((t_pxobject *)x,1);
  x->list_outlet = listout(x);
#endif
#if PD
  t_pvgrain *x = (t_pvgrain *)pd_new(pvgrain_class);
  x->list_outlet = outlet_new(&x->x_obj,gensym("list"));
#endif
  x->vs = sys_getblksize();
  x->R = sys_getsr();
  
  x->grains_per_frame = atom_getfloatarg(0,argc,argv);
  x->grain_probability = atom_getfloatarg(1,argc,argv);
  x->topfreq = atom_getfloatarg(2,argc,argv);
  x->overlap = atom_getfloatarg(3,argc,argv);
  x->winfac = atom_getfloatarg(4,argc,argv);
  
  if(!x->grains_per_frame)
  	x->grains_per_frame = 4;
  if(!x->grain_probability)
  	x->grain_probability = .0001;
  if(!x->topfreq)
  	x->topfreq = 1000;
  if(!x->overlap)
  	x->overlap = 2;
  if(!x->winfac)
  	x->winfac = 4;
  	
  pvgrain_init(x,0);
  return (x);
}

void pvgrain_init(t_pvgrain *x, short initialized)
{
	float curfreq;
	
	x->D = x->vs;
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;
	limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->inCount = -(x->Nw);
	x->mult = 1. / (float) x->N;
	x->Iinv = 1./x->D;
	x->myPInc = x->P*x->L/x->R;
	x->ffac = x->P * PI/x->N;
#if MSP
	x->m_clock = clock_new(x,(method)pvgrain_tick);
#endif
#if PD
	x->m_clock = clock_new(x,(void *)pvgrain_tick);
#endif
	x->c_fundamental =  (float) x->R/(float)( (x->N2)<<1);
	x->c_factor_in =  (float) x->R/((float)x->D * TWOPI);
	x->c_factor_out = TWOPI * (float)  x->D / (float) x->R;
	
	
	if(!initialized){
		x->P = 1.0 ;
		x->sample_basefreq = 261.0;
		x->bottomfreq = 0.0;
		x->mute = 0;
		x->Wanal = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->Wsyn = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->Hwin = (float *) getbytes( (MAX_Nw) * sizeof(float));	
		x->input = (float *) getbytes( MAX_Nw * sizeof(float) );	
		x->buffer = (float *) getbytes( MAX_N * sizeof(float) );
		x->channel = (float *) getbytes( (MAX_N+2) * sizeof(float) );
		x->output = (float *) getbytes( MAX_Nw * sizeof(float) );
		x->bitshuffle = (int *) getbytes( MAX_N * 2 * sizeof( int ) );
		x->trigland = (float *) getbytes( MAX_N * 2 * sizeof( float ) );
		x->c_lastphase_in = (float *) getbytes((MAX_N2+1) * sizeof(float));
		x->binsort = (short *) getbytes((MAX_N2+1) * sizeof(short) );
		x->listdata = (float *) getbytes(40 * sizeof(float));
		
	} 
	memset((char *)x->input,0,x->Nw * sizeof(float));
	memset((char *)x->output,0,x->Nw * sizeof(float));	
	memset((char *)x->c_lastphase_in,0,(x->N2+1) * sizeof(float));
	init_rdft( x->N, x->bitshuffle, x->trigland);
	makewindows( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D);
	curfreq = 0;
	x->hi_bin = 0;
	x->lo_bin = 0;
	while( curfreq < x->topfreq ) {
		++(x->hi_bin);
		curfreq += x->c_fundamental ;
	}
	x->lo_bin = 0;  
	curfreq = 0;
	while( curfreq < x->bottomfreq ) {
		++(x->lo_bin);
		curfreq += x->c_fundamental ;
	}
	
}

void pvgrain_overlap(t_pvgrain *x, t_floatarg o)
{
  if(!power_of_two((int)o)){
    error("%f is not a power of two",o);
    return;
  }
  x->overlap = (int)o;
  pvgrain_init(x,1);
}

void pvgrain_winfac(t_pvgrain *x, t_floatarg f)
{
  if(!power_of_two((int)f)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = (int)f;
  pvgrain_init(x,2);
}

void pvgrain_fftinfo( t_pvgrain *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}


void pvgrain_free(t_pvgrain *x)
{
#if MSP
  dsp_free((t_pxobject *) x);
#endif
  freebytes(x->trigland,0);
  freebytes(x->bitshuffle,0);
  freebytes(x->Wanal,0);
  freebytes(x->Wsyn,0);
  freebytes(x->input,0);
  freebytes(x->Hwin,0);
  freebytes(x->buffer,0);
  freebytes(x->channel,0);
  freebytes(x->output,0);
  freebytes(x->c_lastphase_in,0);
  freebytes(x->binsort,0);
  freebytes(x->listdata,0);
}

t_int *pvgrain_perform(t_int *w)
{
  int 	i,j, in;
  float tmp, dice;
  short print_grain;

  t_pvgrain *x = (t_pvgrain *) (w[1]);
  t_float *inbuf = (t_float *)(w[2]);
  t_int n = w[3];
  
  short *binsort = x->binsort;
  int grains_per_frame = x->grains_per_frame ;
  int hi_bin = x->hi_bin;
  int lo_bin = x->lo_bin;
  int *bitshuffle = x->bitshuffle;
  float *trigland = x->trigland;
  float factor_in =  x->c_factor_in;
  int D = x->D;
  int I = D;
  int R = x->R;
  int Nw = x->Nw;
  int N = x->N ;
  int N2 = x-> N2;
  int Nw2 = x->Nw2;
  float fundamental = x->c_fundamental;
  float *lastphase_in = x->c_lastphase_in;
  float *Wanal = x->Wanal;
  float *Wsyn = x->Wsyn;
  float *input = x->input;
  float *Hwin = x->Hwin;
  float *buffer = x->buffer;
  float *channel = x->channel;
  float selection_probability = x->grain_probability;

  if (x->mute) { 	
    return (w+4);	
  }


  in = x->inCount ;
  x->list_count = 0;

  in += D;

  for ( j = 0 ; j < (Nw - D) ; j++ ){
    input[j] = input[j+D];
  }
  for ( j = (Nw-D), i = 0 ; j < Nw; j++, i++ ) {
    input[j] = *inbuf++;
  }

  fold( input, Wanal, Nw, buffer, N, in );   
  rdft( N, 1, buffer, bitshuffle, trigland );
  convert( buffer, channel, N2, lastphase_in, fundamental, factor_in );
  if( grains_per_frame > hi_bin - lo_bin )
    grains_per_frame = hi_bin - lo_bin;
//  binsort[0] = 0;
  for( i = 0; i < hi_bin; i++ ){// could be hi_bin - lo_bin
    binsort[i] = i + lo_bin;
  }
  for( i = lo_bin; i < hi_bin - 1; i++ ){
    for( j = i+1; j < hi_bin; j++ ){
      if(channel[binsort[j] * 2] > channel[binsort[i] * 2]) {
		tmp = binsort[j];
		binsort[j] = binsort[i];
		binsort[i] = tmp;
      }
    }
  }
  for( i = 0; i < grains_per_frame; i++ ){
    print_grain = 1;
    dice = pvgrain_randf(0.,1.);
    if( dice < 0.0 || dice > 1.0 ){
      error("dice %f out of range", dice);
    }
    if( selection_probability < 1.0 ){
      if( dice > selection_probability) {
		print_grain = 0;
      } 
    }
    if( print_grain ){
      x->listdata[ x->list_count * 2 ] = channel[ binsort[i]*2 ];
      x->listdata[ (x->list_count * 2) + 1 ] = channel[(binsort[i]*2) + 1] ;
      ++(x->list_count);
      clock_delay(x->m_clock,0); 
    }
  }
  x->inCount = in % Nw;
	
  return (w+4);
}	

float pvgrain_randf(float min, float max) {
  float guess;
  guess = (float) (rand() % RAND_MAX) / (float) RAND_MAX ;
  return ( min + guess * (max - min) );
}	

void pvgrain_mute(t_pvgrain *x, t_floatarg state)
{
  x->mute = (short)state;	
}

#if MSP
void pvgrain_float(t_pvgrain *x, double f) // Look at floats at inlets
{
int inlet = x->x_obj.z_in;
	
  if (inlet == 1)
    {
      x->P = f;
      //post("P set to %f",f);
    }
  else if (inlet == 2)
    {
      x->synt = f;
      //post("synt set to %f",f);
    }
}
#endif

void pvgrain_dsp(t_pvgrain *x, t_signal **sp, short *count)
{
  /*	long i;
	if( count[1] ){
	x->pitch_connected = 1;
	} else {
	x->pitch_connected = 0;
	}
	if( count[2] ){
	x->synt_connected = 1;
	} else {
	x->synt_connected = 0;
	}
  */
  if(x->vs != sp[0]->s_n || x->R != sp[0]->s_sr){
    x->vs = sp[0]->s_n;
    x->R = sp[0]->s_sr;
    pvgrain_init(x,1);
  }
  dsp_add(pvgrain_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

