#include "MSPd.h"


static t_class *granola_class;

#define OBJECT_NAME "granola~"

typedef struct _granola
{
    
    t_object x_obj;
    float x_f;
    float *gbuf;
    long grainsamps;
    long buflen ; // length of buffer
    int maxgrainsamps; // set maximum delay in ms.
    float grain_duration; // user grain duration in seconds
    float sr;
    float *grainenv;
    long gpt1; // grain pointer 1
    long gpt2; // grain pointer 2
    long gpt3; // grain pointer 3
    float phs1; // phase 1
    float phs2; // phase 2
    float phs3; // phase 3
    float incr;
    long curdel;
    short mute_me;
    short iconnect;
    
} t_granola;



void *granola_new(t_floatarg val);
t_int *offset_perform(t_int *w);
t_int *granola_perform(t_int *w);
void granola_dsp(t_granola *x, t_signal **sp);
void granola_mute(t_granola *x, t_floatarg toggle);
void granola_float(t_granola *x, double f ) ;
void granola_dsp_free(t_granola *x);
void granola_size(t_granola *x, t_floatarg newsize);
void granola_clear(t_granola *x);
void granola_init(t_granola *x);

void granola_tilde_setup(void){
    granola_class = class_new(gensym("granola~"), (t_newmethod)granola_new,
                              (t_method)granola_dsp_free ,sizeof(t_granola), 0,A_FLOAT,0);
    CLASS_MAINSIGNALIN(granola_class, t_granola, x_f);
    class_addmethod(granola_class,(t_method)granola_dsp,gensym("dsp"),0);
    class_addmethod(granola_class,(t_method)granola_clear,gensym("clear"),0);
    class_addmethod(granola_class,(t_method)granola_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(granola_class,(t_method)granola_size,gensym("size"),A_FLOAT,0);
    potpourri_announce(OBJECT_NAME);
}


void granola_float(t_granola *x, double f) {
    x->incr = f;
}

void granola_dsp_free(t_granola *x)
{
    
    free(x->gbuf);
    free(x->grainenv);
}


void granola_mute(t_granola *x, t_floatarg toggle)
{
    x->mute_me = (short)toggle;
}

void granola_clear(t_granola *x) {
	memset((char *)x->gbuf, 0, x->buflen);
}

void granola_size(t_granola *x, t_floatarg newsize) {
	int newsamps, i;
	newsamps = newsize * 0.001 * sys_getsr();
	if( newsamps >= x->maxgrainsamps ){
		error("granola~: specified size over preset maximum, no action taken");
		return;
	}
	if( newsamps < 8 ){
		error("granola~: grainsize too small");
		return;
	}
	x->grainsamps = newsamps; // will use for shrinkage
	x->buflen = x->grainsamps * 4;
	for(i = 0; i < x->grainsamps; i++ ){
		x->grainenv[i] = .5 + (-.5 * cos( TWOPI * ((float)i/(float)x->grainsamps) ) );
	}
	x->gpt1 = 0;
	x->gpt2 = x->grainsamps / 3.;
	x->gpt3 = 2. * x->grainsamps / 3.;
	x->phs1 = 0;
	x->phs2 = x->grainsamps / 3. ;
	x->phs3 = 2. * x->grainsamps / 3. ;
	x->curdel = 0;
}

void *granola_new(t_floatarg val)
{
    
    t_granola *x = (t_granola *)pd_new(granola_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    
    // INITIALIZATIONS
	x->sr = sys_getsr();
	x->grain_duration = val * 0.001; // convert to seconds
	x->gbuf = NULL;
	x->grainenv = NULL;
	
	granola_init(x);
    return x;
    /*
     
     
    if( val > 0 ) {
        x->grainsamps = val;
        //    post( "grainsize set to %.0f", val );
    } else {
        x->grainsamps = 2048;
        // post( "grainsize defaults to %d, val was %.0f", x->grainsamps, val );
        
    }
    x->buflen = x->grainsamps * 4;
    x->gbuf = (float *) calloc( x->buflen, sizeof(float) ) ;
    x->grainenv = (float *) calloc( x->grainsamps, sizeof(float) );
    for(i = 0; i < x->grainsamps; i++ ){
        x->grainenv[i] = .5 + (-.5 * cos( TWOPI * ((float)i/(float)x->grainsamps) ) );
    }
    x->gpt1 = 0;
    x->gpt2 = x->grainsamps / 3.;
    x->gpt3 = 2. * x->grainsamps / 3.;
    x->phs1 = 0;
    x->phs2 = x->grainsamps / 3. ;
    x->phs3 = 2. * x->grainsamps / 3. ;
    x->incr = .5 ;
    x->curdel = 0;
    x->mute_me = 0;
    
    return (x);
    */
}

void granola_init(t_granola *x)
{
	int i;
	if(x->sr == 0){
		post("granola~: dodging zero sampling rate!");
		return;
	}
	x->grainsamps = x->grain_duration * x->sr;
	if(x->grainsamps <= 5 || x->grainsamps > 4410000) {
		x->grainsamps = 2048;
		post( "granola~: grainsize autoset to %d samples, rather than user-specified length %.0f", x->grainsamps, x->grain_duration * x->sr);
	}
	x->maxgrainsamps = x->grainsamps; // will use for shrinkage
	x->buflen = x->grainsamps * 4;
	// first time only
	if(x->gbuf == NULL){
		x->gbuf = (float *) calloc(x->buflen, sizeof(float));
		x->grainenv = (float *) calloc(x->grainsamps, sizeof(float));
		x->incr = .5;
		x->mute_me = 0;
	}
	// or realloc if necessary
	else {
		x->gbuf = (float *) realloc(x->gbuf, x->buflen * sizeof(float));
		x->grainenv = (float *) realloc(x->grainenv, x->grainsamps * sizeof(float));
	}
	for(i = 0; i < x->grainsamps; i++ ){
		x->grainenv[i] = .5 + (-.5 * cos(TWOPI * ((float)i/(float)x->grainsamps)));
	}
	x->gpt1 = 0;
	x->gpt2 = x->grainsamps / 3.;
	x->gpt3 = 2. * x->grainsamps / 3.;
	x->phs1 = 0;
	x->phs2 = x->grainsamps / 3. ;
	x->phs3 = 2. * x->grainsamps / 3. ;
	x->curdel = 0;
}


t_int *granola_perform(t_int *w)
{
	float  outsamp ;
	int iphs_a, iphs_b;
	float frac;
    
	
	/****/
	t_granola *x = (t_granola *) (w[1]);
	t_float *in = (t_float *)(w[2]);
	t_float *increment = (t_float *)(w[3]);
	t_float *out = (t_float *)(w[4]);
	int n = (int)(w[5]);
	
	long gpt1 = x->gpt1;
	long gpt2 = x->gpt2;
	long gpt3 = x->gpt3;
	float phs1 = x->phs1;
	float phs2 = x->phs2;
	float phs3 = x->phs3;
	long curdel = x->curdel;
	long buflen = x->buflen;
	long grainsamps = x->grainsamps;
	float *grainenv = x->grainenv;
	float *gbuf = x->gbuf;
	float incr = x->incr;
	
	if( x->mute_me ) {
		while( n-- ){
			*out++ = 0.0;
		}
		return (w+6);
	}
	
	while (n--) {
        
		x->incr = *increment++;
        
		if( x->incr <= 0. ) {
			x->incr = .5 ;
		}
		
		if( curdel >= buflen ){
			curdel = 0 ;
		}
		gbuf[ curdel ] = *in++;
    	
		// grain 1
		iphs_a = floor( phs1 );
		iphs_b = iphs_a + 1;
		
		frac = phs1 - iphs_a;
		while( iphs_a >= buflen ) {
			iphs_a -= buflen;
		}
		while( iphs_b >= buflen ) {
			iphs_b -= buflen;
		}
		outsamp = (gbuf[ iphs_a ] + frac * ( gbuf[ iphs_b ] - gbuf[ iphs_a ])) * grainenv[ gpt1++ ];
        
		if( gpt1 >= grainsamps ) {
			
			gpt1 = 0;
			phs1 = curdel;
		}
		phs1 += incr;
		while( phs1 >= buflen ) {
			phs1 -= buflen;
		}
		
		// now add second grain
		
		
		iphs_a = floor( phs2 );
		
		iphs_b = iphs_a + 1;
		
		frac = phs2 - iphs_a;
        
		
		while( iphs_a >= buflen ) {
			iphs_a -= buflen;
		}
		while( iphs_b >= buflen ) {
			iphs_b -= buflen;
		}
		outsamp += (gbuf[ iphs_a ] + frac * ( gbuf[ iphs_b ] - gbuf[ iphs_a ])) * grainenv[ gpt2++ ];
		if( gpt2 >= grainsamps ) {
			gpt2 = 0;
			phs2 = curdel ;
		}
		phs2 += incr ;
		while( phs2 >= buflen ) {
			phs2 -= buflen ;
		}
		
		// now add third grain
		
		iphs_a = floor( phs3 );
		iphs_b = iphs_a + 1;
		
		frac = phs3 - iphs_a;
        
		while( iphs_a >= buflen ) {
			iphs_a -= buflen;
		}
		while( iphs_b >= buflen ) {
			iphs_b -= buflen;
		}
		outsamp += (gbuf[ iphs_a ] + frac * ( gbuf[ iphs_b ] - gbuf[ iphs_a ])) * grainenv[ gpt3++ ];
		if( gpt3 >= grainsamps ) {
			gpt3 = 0;
			phs3 = curdel ;
		}
		phs3 += incr ;
		while( phs3 >= buflen ) {
			phs3 -= buflen ;
		}
		
		
		++curdel;
		
		*out++ = outsamp;
		/* output may well need to attenuated */
	}
	x->phs1 = phs1;
	x->phs2 = phs2;
	x->phs3 = phs3;
	x->gpt1 = gpt1;
	x->gpt2 = gpt2;
	x->gpt3 = gpt3;
	x->curdel = curdel;
	return (w+6);
	
}		

void granola_dsp(t_granola *x, t_signal **sp)
{
    
    dsp_add(granola_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, (t_int)sp[0]->s_n);
}

