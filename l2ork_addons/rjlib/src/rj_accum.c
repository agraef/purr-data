/* code for the "rj_accum" pd class. 
ver 0.2
Amaury Hazan 
Damian Stewart
*/

#include "m_pd.h"
#include <stdlib.h>
#include <string.h>

#define DEF_BLOCKSIZE 512
 
typedef struct rj_accum
{
  t_object x_obj;
  
  // configuration values
  float blocksize;
  float samplerate;
  
  //short-time buffer, size,counter and desired duration (in s)
  float * st_buffer;
  int st_buffersize;
  int st_buffercnt;
  float st_duration;
  float st_mean;
  float st_total;
  int st_idx;
	
  //long-time buffer, size, counter and desired duration (in s)
  float * lt_buffer;
  int lt_buffersize;
  int lt_buffercnt;
  float lt_duration;
  float lt_mean;
  float lt_total;
  int lt_idx;
  
  float rel_change;
  
  t_outlet* relative_change;		   /* m: place for outlet */
  
} t_rj_accum;

void rj_accum_float(t_rj_accum *x, t_floatarg f)
{
    if (x->st_buffercnt< x->st_buffersize-1) x->st_buffercnt++;
    // calculate new total: subtract old, add new
    x->st_total -= x->st_buffer[x->st_idx];
    x->st_total += f;
    x->st_buffer[x->st_idx] = f;
    // increment index
    x->st_idx = ( x->st_idx + 1 ) % x->st_buffersize;
    
    //x->st_mean=x->st_mean*st_before_weight + f*st_after_weight;
    x->st_mean = x->st_total / x->st_buffercnt;
    
    if (x->lt_buffercnt< x->lt_buffersize-1) x->lt_buffercnt++;
    // calculate new total: subtract old, add new
    x->lt_total -= x->lt_buffer[x->lt_idx];
    x->lt_total += f;
    x->lt_buffer[x->lt_idx] = f;
    // increment index
    x->lt_idx = ( x->lt_idx + 1 ) % x->lt_buffersize;
    //x->lt_mean=x->lt_mean*lt_before_weight + f*lt_after_weight;
    x->lt_mean = x->lt_total / x->lt_buffercnt;
    
    if (x->lt_mean==0) x->rel_change=0;
    else x->rel_change=(x->st_mean-x->lt_mean)/x->lt_mean;    
    
	outlet_float(x->relative_change, x->rel_change);
}

t_class *rj_accum_class;

void rj_accum_set_st(t_rj_accum *x, t_floatarg g)
{
    post("short term duration fixed to %f", g);
    x->st_duration=g;
    // determining st buffer size
    x->st_buffersize= (int) ((x->st_duration*x->samplerate)/x->blocksize);  
    x->st_buffercnt=0;
    free( x->st_buffer );
    x->st_buffer = malloc( x->st_buffersize*sizeof(float) );
    memset( x->st_buffer, 0, x->st_buffersize*sizeof(float) );
    x->st_total = 0;
    x->st_idx = 0;
    post("short term number of frames %d", x->st_buffersize);
    
}

void rj_accum_set_lt(t_rj_accum *x, t_floatarg g)
{
    post("long term duration fixed to %f", g);
    x->lt_duration=g;
    // determining st buffer size
    x->lt_buffersize= (int) ((x->lt_duration*x->samplerate)/x->blocksize);  
    x->lt_buffercnt=0;
    free( x->lt_buffer );
    x->lt_buffer = malloc( x->lt_buffersize*sizeof(float) );
    memset( x->lt_buffer, 0, x->lt_buffersize*sizeof(float) );
    x->lt_total = 0;
    x->lt_idx = 0;
    post("long term number of frames %d", x->lt_buffersize);
}

void *rj_accum_new(t_symbol *selector, int argcount, t_atom *argvec)
{
    t_rj_accum *x = (t_rj_accum *)pd_new(rj_accum_class);
    x->relative_change=outlet_new(&x->x_obj, &s_float);
    
    post("new %s", selector->s_name);
    
    // param passing
    if (argcount==1){
      x->st_duration=5.f;
      x->lt_duration=30.f;
      x->blocksize=argvec[0].a_w.w_float;
      x->samplerate=sys_getsr();
    }
    else if (argcount==0){
      x->st_duration=5.f;
      x->lt_duration=30.f;
      x->blocksize=DEF_BLOCKSIZE;  
      x->samplerate=sys_getsr();
    }
    else{
      post("usage: rj_accum [hopsize]");
      post("       where hopsize is processing hop size (default 512)");
      
      post("you provided %d arguments",argcount);
      
      x->st_duration=5.f;
      x->lt_duration=30.f;
      x->blocksize=DEF_BLOCKSIZE;  
      x->samplerate=sys_getsr();  
    }
    
    post("std %f", x->st_duration);
    post("ltd %f", x->lt_duration);
    post("hopsize %f", x->blocksize);
    post("samplerate %f", x->samplerate);
    
    // buffers allocation
    int cnt;
    
    // detrmining st buffer size
    x->st_buffersize= (int) ((x->st_duration*x->samplerate)/x->blocksize);  
    x->st_buffercnt=0;
    
    // detrmining lt buffer size
    x->lt_buffersize= (int) ((x->lt_duration*x->samplerate)/x->blocksize);  
	x->lt_buffercnt=0;

    // initializing buffers	
    x->st_buffer = (float*)malloc( x->st_buffersize*sizeof(float) );
    x->lt_buffer = (float*)malloc( x->lt_buffersize*sizeof(float) );
    memset( x->st_buffer, 0, x->st_buffersize*sizeof(float) );
    memset( x->lt_buffer, 0, x->lt_buffersize*sizeof(float) );

	// initilaizing means
	x->st_mean=0.f;
	x->lt_mean=0.f;
	
	// initializing totals and indexes
	x->st_total = 0.f;
	x->st_idx = 0;
	x->lt_total = 0.f;
	x->lt_idx = 0;
	
	x->rel_change=0.f;
    
    return (void *)x;
}

static void rj_accumulator_free(t_rj_accum *x) 
{
    free( x->st_buffer );	
    free( x->lt_buffer );	
}

void rj_accum_setup(void)
{
    
    /* We specify "A_GIMME" as creation argument for both the creation
	routine and the method (callback) for the "conf" message.  */
    rj_accum_class = class_new(gensym("rj_accum"), (t_newmethod)rj_accum_new,
    	0, sizeof(t_rj_accum), 0, A_GIMME, 0);
    
    class_addfloat(rj_accum_class, rj_accum_float);
    class_addmethod(rj_accum_class, (t_method)rj_accum_set_st, gensym("st"), A_FLOAT, 0);
	class_addmethod(rj_accum_class, (t_method)rj_accum_set_lt, gensym("lt"), A_FLOAT, 0);
    post("rj_accum version 0.2");
}

