#include <stdlib.h>
#include "m_pd.h"
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define WINDOWSIZE 1024

/* ------------------------ rj_zcr~ for pd----------------------------- */

/* tilde object to take absolute value. */

static t_class *rj_zcr_class;

typedef struct _rj_zcr
{
    t_object x_obj; 	   /* obligatory header */    
	t_float x_f;	
	t_outlet* rj_zcr;		   /* m: place for outlet */
	
	float * buffer;
	int buffercnt;
    
} t_rj_zcr;

    /* this is the actual performance routine which acts on the samples.
    It's called with a single pointer "w" which is our location in the
    DSP call list.  We return a new "w" which will point to the next item
    after us.  Meanwhile, w[0] is just a pointer to dsp-perform itself
    (no use to us), w[1] and w[2] are the input and output vector locations,
    and w[3] is the number of points to calculate. */

static t_int *rj_zcr_perform(t_int *w)
{
	
	t_rj_zcr *x = (t_rj_zcr *)(w[1]);
    
	t_float *in = (t_float *)(w[2]);
	
	t_float rate=1.f;

	int size=(int)(w[3]); 
	
	int ibuf; 
	for (ibuf=0; ibuf<size; ibuf++) {
		x->buffer[x->buffercnt++]=in[ibuf];
		if(x->buffercnt==WINDOWSIZE-1) {
			
			int ncross=0;
			int i;
			for (i=0; i<WINDOWSIZE-1; i++) {
				if(in[i]*in[i+1]<0) ncross++;
			}
			rate=(float) ncross/(float) WINDOWSIZE;
			
			// copy rate
			outlet_float(x->rj_zcr, rate);
			
			x->buffercnt=0;
		}
	}

	return (w+4);
}

    /* called to start DSP.  Here we call Pd back to add our perform
    routine to a linear callback list which Pd in turn calls to grind
    out the samples. */
static void rj_zcr_dsp(t_rj_zcr *x, t_signal **sp)
{
    dsp_add(rj_zcr_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void *rj_zcr_new(void)
{
    t_rj_zcr *x = (t_rj_zcr *)pd_new(rj_zcr_class);
	x->rj_zcr=outlet_new(&x->x_obj, &s_float);
	x->x_f = 0;
	
	x->buffer=malloc(WINDOWSIZE*sizeof(float));
	x->buffercnt=0;
	
	return (x);
}

static void rj_zcr_free(t_rj_zcr *x) {
	free(x->buffer);	
}


    /* this routine, which must have exactly this name (with the "~" replaced
    by "_tilde) is called when the code is first loaded, and tells Pd how
    to build the "class". */
void rj_zcr_tilde_setup(void)
{
    rj_zcr_class = class_new(gensym("rj_zcr~"), (t_newmethod)rj_zcr_new, (t_method)rj_zcr_free,
    	sizeof(t_rj_zcr), 0, A_DEFFLOAT, 0);

	    /* this is magic to declare that the leftmost, "main" inlet
	    takes signals; other signal inlets are done differently... */

    CLASS_MAINSIGNALIN(rj_zcr_class, t_rj_zcr, x_f);
    	/* here we tell Pd about the "dsp" method, which is called back
	when DSP is turned on. */
    
	class_addmethod(rj_zcr_class, (t_method)rj_zcr_dsp, gensym("dsp"), 0);
	post("rj_zcr version 0.1");
}
