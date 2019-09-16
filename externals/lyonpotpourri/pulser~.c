#include "MSPd.h"

#define FUNC_LEN (16384)
#define FUNC_LEN_OVER2 (8192)
#define MAX_COMPONENTS (256)

#define OBJECT_NAME "pulser~"

static t_class *pulser_class;


typedef struct _pulser
{

  t_object x_obj;
  float x_f;
  int components;
  float global_gain;
  float *wavetab;

  float *phases;
  float frequency;
  float pulsewidth;
  float si_fac;
  short mute;
  short connected[4];
  float sr;
} t_pulser;

void *pulser_new(t_symbol *s, int argc, t_atom *argv);

t_int *pulser_perform(t_int *w);
void pulser_dsp(t_pulser *x, t_signal **sp);
void pulser_mute(t_pulser *x, t_floatarg toggle);
void pulser_harmonics(t_pulser *x, t_floatarg c);
void pulser_float(t_pulser *x, double f);
void pulser_free(t_pulser *x);

void pulser_tilde_setup(void){
  pulser_class = class_new(gensym("pulser~"), (t_newmethod)pulser_new, 
			    (t_method)pulser_free,sizeof(t_pulser), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(pulser_class, t_pulser, x_f);
  class_addmethod(pulser_class,(t_method)pulser_dsp,gensym("dsp"),0);
  class_addmethod(pulser_class,(t_method)pulser_mute,gensym("mute"),A_FLOAT,0);
  class_addmethod(pulser_class,(t_method)pulser_harmonics,gensym("harmonics"),A_FLOAT,0);
  potpourri_announce(OBJECT_NAME);
}

void pulser_mute(t_pulser *x, t_floatarg toggle)
{
	x->mute = toggle;
}

void pulser_harmonics(t_pulser *x, t_floatarg c)
{
	if(c < 2 || c > MAX_COMPONENTS){
		error("harmonic count out of bounds");
		return;
	}
	x->components = c;
	x->global_gain = 1.0 / (float) x->components ;
	// reset phases too?
}

void pulser_free(t_pulser *x)
{
	free(x->phases);
	free(x->wavetab);
}

void *pulser_new(t_symbol *s, int argc, t_atom *argv)
{
  int i;

  t_pulser *x = (t_pulser *)pd_new(pulser_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
  x->sr = sys_getsr();
  if(!x->sr){
    error("zero sampling rate, setting to 44100");
    x->sr = 44100;
  }

  x->mute = 0;
  x->components = 8;
  x->frequency = 440.0;
  x->pulsewidth = 0.5;
  
  if( argc > 0 )
    x->frequency = atom_getfloatarg(0,argc,argv);
  if( argc > 1 )
    x->components = atom_getfloatarg(1,argc,argv);
			
  x->si_fac = ((float)FUNC_LEN/x->sr) ;
  
  if(x->components <= 0 || x->components > MAX_COMPONENTS){
  	error("%d is an illegal number of components, setting to 8",x->components );
  	x->components = 8;
  }
  x->global_gain = 1.0 / (float) x->components ;
  x->phases = (float *) calloc(MAX_COMPONENTS, sizeof(float) );
  x->wavetab = (float *) calloc(FUNC_LEN, sizeof(float) );

  for(i = 0 ; i < FUNC_LEN; i++) {
    x->wavetab[i] = sin(TWOPI * ((float)i/(float) FUNC_LEN)) ;
  }
  return (x);
}


t_int *pulser_perform(t_int *w)
{

  int i,j;
  float gain;
  float incr;

  float outsamp;
  int lookdex;
  t_pulser *x = (t_pulser *) (w[1]);
  t_float *frequency_vec = (t_float *)(w[2]);
  t_float *pulsewidth_vec = (t_float *)(w[3]);
  t_float *out = (t_float *)(w[4]);
  int n = (int) w[5];
	

  float *wavetab = x->wavetab;
  float si_fac = x->si_fac;

  float *phases = x->phases;
  int components = x->components;
  float global_gain = x->global_gain;
  float pulsewidth = x->pulsewidth;
  float frequency = x->frequency;
  short *connected = x->connected;
  
  if( x->mute )
  {
  	while( n-- ){
  		*out++ = 0.0;
  	}
  	return (w+6);
  }
  
  incr = frequency * si_fac;
  
  while (n--) { 

    if( connected[1] ){
    	pulsewidth = *pulsewidth_vec++;
    	// post("pw %f",pulsewidth);
    }
    if( pulsewidth < 0 )
      pulsewidth = 0;
    if( pulsewidth > 1 )
      pulsewidth = 1;
    
    if( connected[0] ){
      incr = *frequency_vec++ * si_fac ;
    }
 
    outsamp = 0;
    
    for( i = 0, j = 1; i < components; i++, j++ ){

      lookdex = (float)FUNC_LEN_OVER2 * pulsewidth * (float)j;
 
      while( lookdex >= FUNC_LEN ){
				lookdex -= FUNC_LEN;
		  }
 
      gain = wavetab[ lookdex ] ;
 
      phases[i] += incr * (float) j;
      while( phases[i] < 0.0 ) {
				phases[i] += FUNC_LEN;
      }
      while( phases[i] >= FUNC_LEN ){
				phases[i] -= FUNC_LEN;
      }
      outsamp += gain * wavetab[ (int) phases[i] ];
		
    }
    *out++ =  outsamp * global_gain; 
  }

  //	x->bendphs = bendphs;
  return (w+6);
}		

void pulser_dsp(t_pulser *x, t_signal **sp)
{
  long i;

	if(!sp[0]->s_sr){
		error("zero sampling rate");
		return;
	}
	
  if(x->sr != sp[0]->s_sr){
  	x->sr = sp[0]->s_sr;
  	x->si_fac = ((float)FUNC_LEN/x->sr);
  	for(i=0;i<MAX_COMPONENTS;i++){
  		x->phases[i] = 0.0;
  	}
  }
  for( i = 0; i < 2; i++){

  	x->connected[i] = 1;
  }
  dsp_add(pulser_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, (t_int)sp[0]->s_n);
}


