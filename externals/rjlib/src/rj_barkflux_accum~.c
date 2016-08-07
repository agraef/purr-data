#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define NFILTERS 12
#define LF 40.f
#define HF 8000.f
#define WINSIZE 513

/* ----------------------- frequency domain conversion utilities -----------------------*/

// Bark <-> Hz conversion functions
// // See: Traunmüller (1990) "Analytical expressions for the tonotopic sensory scale" J. Acoust. Soc. Am. 88: 97-100. 
float freq2bark(float freq){
    return (26.81f / (1.f + 1960.f / freq ) - 0.53f);
}

float bark2freq(float z){
    return (1960.f / (26.81f / (z + 0.53f) - 1.f));
}

// Mel <-> Hz conversion functions
// // See: Stevens, Volkman and Newman in 1937 (J. Acoust. Soc. Am 8(3) 185--190)
float freq2mel(float freq){
  return (1127.01048f * log (1.f+(freq/700.f)));
}

float mel2freq(float m){
    return (700.f*(exp(m/1127.01048f)-1));
}

// FFTBin <-> Hz conversion functions

int freq2bin(int windowSize,int sampleRate, float freq){
    return (int) ceil((windowSize*2*freq)/(float)sampleRate);
}

float bin2freq(int windowSize,int sampleRate, int bin){
    return ((float) (bin*sampleRate))/(2.f*(float) windowSize);
}


/* ------------------------ Filter objects ------------------------ */

typedef struct {
    float lf;
    float hf;
    float gain;
} RectSpecFilter;

typedef RectSpecFilter * RectSpecFilter_ptr;

float getCf(RectSpecFilter m) {
    return (m.hf+m.lf)*.5f;
}

float getEnergy(RectSpecFilter m ,float * input, int winSize, int sampleRate) { 
    int lf_index=freq2bin(winSize, sampleRate, m.lf); 
    int hf_index=freq2bin(winSize, sampleRate, m.hf); 
    float res=0.; 
    int i;
	for (i=lf_index; i<=hf_index; i++) {
        res+=input[i]*input[i]; 
    } 
    return res; 
}

/* ------------------------ Filterbank ---------------------------- */
typedef struct {
	int winSize;
	int nFilters; 
	int sampleRate;
	float lowF;
	float highF;
	RectSpecFilter_ptr bankFilters;
	// bank energies
	float * bankEn;
} Filterbank;

typedef Filterbank * Filterbank_ptr;


Filterbank_ptr allocFilterbank(int nFilters) {
	
	Filterbank_ptr fb=malloc(sizeof(Filterbank));
	fb->bankFilters=malloc(nFilters*sizeof(RectSpecFilter));
	fb->bankEn=malloc(nFilters*sizeof(float));
	
	return fb;
}


void deleteFilterbank(Filterbank_ptr fb) {

	free(fb->bankFilters);
	free(fb->bankEn);
	free(fb);
}

void initFilterbank(Filterbank_ptr m, int nFilters, int winSize, int sampleRate, float lowF, float highF) {
	
	m->lowF=lowF;
	m->highF=highF;
	m->sampleRate=sampleRate;
	m->nFilters=nFilters;
	m->winSize=winSize;
	
	float zBeg, zEnd, zBw;
	zBeg=freq2bark(lowF);
	zEnd=freq2bark(highF);
	zBw=(zEnd-zBeg)/(float) nFilters;
	
	// linear separation on the bark scale
	int i;
	for (i=0; i<nFilters; i++) {
		
		float lf=bark2freq(zBeg+zBw*((float) i));
		float hf=bark2freq(zBeg+zBw*((float) i + 1.f));
		float gain=1.f;
		
		// set filters lf, hf, gain
	    m->bankFilters[i].lf=lf;
		m->bankFilters[i].hf=hf;
		m->bankFilters[i].gain=lf;
	}
};

void computeFilterEnergies(Filterbank_ptr m, float * input){
	int i;
	for (i=0; i< m->nFilters; i++){
		m->bankEn[i]=getEnergy(m->bankFilters[i],input, m->winSize, m->sampleRate) /((float) m->winSize);
	}
};

/* ------------------------ rj_barkflux_accum~ for pd----------------------------- */

/* tilde object to take absolute value. */

static t_class *rj_barkflux_accum_class;

typedef struct _rj_barkflux_accum
{
    t_object x_obj; 	   /* obligatory header */
    t_float x_f;    	   /* place to hold inlet's value if it's set by message */
    Filterbank_ptr fb;     /* fb: place to hold the filterbank structure */
    
    // configuration values
    float blocksize;
    float samplerate;
    
    // short term signal variables   
    // frame counter
    int st_buffercnt;
    // desired duration of short trem description
    float st_duration;
    // number of frames needed to describe st signal 
    int st_buffersize;
    // short term bark spectrum
    float * st_spectrum;
    float * st_spectrum_norm;

    // long term signal variables
    // frame counter
    int lt_buffercnt;
    // desired duration of short trem description
    float lt_duration;
    // number of frames needed to describe st signal 
    int lt_buffersize;
    // short term bark spectrum
    float * lt_spectrum;
    float * lt_spectrum_norm;
    
    // error mode
    // 0=rms
    // 1-kl divergence
    int error_mode;
	
	t_outlet* rj_barkflux_accum;		   /* m: place for outlet */
	
} t_rj_barkflux_accum;

    /* this is the actual performance routine which acts on the samples.
    It's called with a single pointer "w" which is our location in the
    DSP call list.  We return a new "w" which will point to the next item
    after us.  Meanwhile, w[0] is just a pointer to dsp-perform itself
    (no use to us), w[1] and w[2] are the input and output vector locations,
    and w[3] is the number of points to calculate. */

static t_int *rj_barkflux_accum_perform(t_int *w)
{
	
	t_rj_barkflux_accum *x = (t_rj_barkflux_accum *)(w[1]);
    t_float *in = (t_float *)(w[2]);
	
	int size=(int)(w[3]);
	int ifilter;
	float tmp_err, err=0.f;
	float st_sum=0.f, lt_sum=0.f; 
	 
	if (size>=WINSIZE){
       
        // compute filterbank energies	
	    computeFilterEnergies(x->fb, in);
	    // compute accumulation parameters - short term
	    if (x->st_buffercnt< x->st_buffersize-1) x->st_buffercnt++;
        float st_before_weight= (float) x->st_buffercnt / ((float) x->st_buffercnt +1.f);
        float st_after_weight= 1.f / ((float) x->st_buffercnt +1.f);
	    // compute accumulation parameters - long term
	    if (x->lt_buffercnt< x->lt_buffersize-1) x->lt_buffercnt++;
        float lt_before_weight= (float) x->lt_buffercnt / ((float) x->lt_buffercnt +1.f);
        float lt_after_weight= 1.f / ((float) x->lt_buffercnt +1.f);
	    // actualize average st and lt spectral distribution 
	    for (ifilter=0; ifilter<x->fb->nFilters; ifilter++)
	    {	          	       
           x->st_spectrum[ifilter]=x->st_spectrum[ifilter]*st_before_weight + x->fb->bankEn[ifilter]*st_after_weight;
           x->lt_spectrum[ifilter]=x->lt_spectrum[ifilter]*lt_before_weight + x->fb->bankEn[ifilter]*lt_after_weight;
           st_sum+=x->st_spectrum[ifilter];
           lt_sum+=x->lt_spectrum[ifilter];
           
           if(x->error_mode==0)
	       {
            tmp_err=x->st_spectrum[ifilter]-x->lt_spectrum[ifilter];
            err+=tmp_err*tmp_err;
	       }
	    }
	    
	    if(x->error_mode==1)
	    {
      	    // getting normalized st and lt
      	    for (ifilter=0; ifilter<x->fb->nFilters; ifilter++)
      	    {
      	       x->st_spectrum_norm[ifilter]=x->st_spectrum[ifilter];
      	       x->lt_spectrum_norm[ifilter]=x->lt_spectrum[ifilter];
      	    }
      	    
      	    //computing kl
      	    for (ifilter=0; ifilter<x->fb->nFilters; ifilter++)
      	    {
      	       err+=x->st_spectrum_norm[ifilter]*log(x->st_spectrum_norm[ifilter]/x->lt_spectrum_norm[ifilter]);
      	    }
	    }

	    outlet_float(x->rj_barkflux_accum, err);	    
	}
	
	else post("rj_barkflux_accum: input buffer too small. expected spectrum size is %d, so block size should be",WINSIZE,(WINSIZE-1)*2);
	
	return (w+4);
}

    /* called to start DSP.  Here we call Pd back to add our perform
    routine to a linear callback list which Pd in turn calls to grind
    out the samples. */
static void rj_barkflux_accum_dsp(t_rj_barkflux_accum *x, t_signal **sp)
{
    dsp_add(rj_barkflux_accum_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

void accum_set_st(t_rj_barkflux_accum *x, t_floatarg g)
{
    post("short term duration fixed to %f", g);
    x->st_duration=g;
    // determining st buffer size
    x->st_buffersize= (int) ((x->st_duration*x->samplerate)/x->blocksize); 
    x->st_buffercnt=0;
    post("short term number of frames %d", x->st_buffersize);
}

void accum_set_lt(t_rj_barkflux_accum *x, t_floatarg g)
{
    post("long term duration fixed to %f", g);
    x->lt_duration=g;
    // determining st buffer size
    x->lt_buffersize= (int) ((x->lt_duration*x->samplerate)/x->blocksize);   
    x->lt_buffercnt=0;
    post("long term number of frames %d", x->lt_buffersize);

}

static void *rj_barkflux_accum_new(void)
{
    int ifilter;
    
    t_rj_barkflux_accum *x = (t_rj_barkflux_accum *)pd_new(rj_barkflux_accum_class);
	x->rj_barkflux_accum=outlet_new(&x->x_obj, &s_float);
	x->x_f = 0;
	
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("st"), gensym("float"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("lt"), gensym("float"));
	
	x->st_duration=5;
    x->lt_duration=30;
    x->blocksize=1024;  
    x->samplerate=sys_getsr();
    
    // determining st buffer size
    x->st_buffersize= (int) ((x->st_duration*x->samplerate)/x->blocksize);  
    x->st_buffercnt=0;
    
    // determining lt buffer size
    x->lt_buffersize= (int) ((x->lt_duration*x->samplerate)/x->blocksize);  
	x->lt_buffercnt=0;
    
	// filterbank alloc and init
	x->fb=allocFilterbank(NFILTERS);
	initFilterbank(x->fb, NFILTERS, WINSIZE, sys_getsr(), LF, HF);
	
	// buffers for st and lt spectrum
	x->st_spectrum=malloc(NFILTERS*sizeof(float));
	x->lt_spectrum=malloc(NFILTERS*sizeof(float));
	for (ifilter=0; ifilter<NFILTERS; ifilter++)
	{
	   x->st_spectrum[ifilter]=0; 
	   x->lt_spectrum[ifilter]=0;   
	}	
	x->st_spectrum_norm=malloc(NFILTERS*sizeof(float));
	x->lt_spectrum_norm=malloc(NFILTERS*sizeof(float));
	
	// error mode
	// 0: rms
	// 1: kl divergence
	x->error_mode=0;
	
	post("rj_barkflux_accum: init ok");
	
	return (x);
}


static void rj_barkflux_accum_free(t_rj_barkflux_accum *x) {

	deleteFilterbank(x->fb);
	free(x->st_spectrum);
	free(x->lt_spectrum);
	free(x->st_spectrum_norm);
	free(x->lt_spectrum_norm);
}


    /* this routine, which must have exactly this name (with the "~" replaced
    by "_tilde) is called when the code is first loaded, and tells Pd how
    to build the "class". */
void rj_barkflux_accum_tilde_setup(void)
{
    rj_barkflux_accum_class = class_new(gensym("rj_barkflux_accum~"), (t_newmethod)rj_barkflux_accum_new, (t_method)rj_barkflux_accum_free,
    	sizeof(t_rj_barkflux_accum), 0, A_DEFFLOAT, 0);
    
    // gimme init format ... for later
    //rj_barkflux_accum_class = class_new(gensym("rj_barkflux_accum~"), (t_newmethod)rj_barkflux_accum_new, (t_method)rj_barkflux_accum_free,
    //	sizeof(t_rj_barkflux_accum), 0, A_GIMME, 0);

	/* this is magic to declare that the leftmost, "main" inlet
	takes signals; other signal inlets are done differently... */

    CLASS_MAINSIGNALIN(rj_barkflux_accum_class, t_rj_barkflux_accum, x_f);
    	/* here we tell Pd about the "dsp" method, which is called back
	when DSP is turned on. */
    
	class_addmethod(rj_barkflux_accum_class, (t_method)rj_barkflux_accum_dsp, gensym("dsp"), 0);
	
	class_addmethod(rj_barkflux_accum_class, (t_method)accum_set_st, gensym("st"), A_FLOAT, 0);
	class_addmethod(rj_barkflux_accum_class, (t_method)accum_set_lt, gensym("lt"), A_FLOAT, 0);
	
	
	post("rj_barkflux_accum version 0.1");
}

