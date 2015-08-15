#include "m_pd.h"
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

// size of the magnitude spectrum to analyze
#define WINSIZE 513

/* ------------------------ rj_centroid~ for pd----------------------------- */

/* tilde object to take absolute value. */

static t_class *rj_centroid_class;

typedef struct _rj_centroid
{
    t_object x_obj; 	   /* obligatory header */    
	t_float x_f;	
	t_outlet* rj_centroid;		   /* m: place for outlet */
	
} t_rj_centroid;

    /* this is the actual performance routine which acts on the samples.
    It's called with a single pointer "w" which is our location in the
    DSP call list.  We return a new "w" which will point to the next item
    after us.  Meanwhile, w[0] is just a pointer to dsp-perform itself
    (no use to us), w[1] and w[2] are the input and output vector locations,
    and w[3] is the number of points to calculate. */

static t_int *rj_centroid_perform(t_int *w)
{
	
	t_rj_centroid *x = (t_rj_centroid *)(w[1]);
    t_float *in = (t_float *)(w[2]);
	
	//int size=(int)(w[3]); 
	int size=WINSIZE;
	
    int j;
    float sum = 0., sc = 0.;
    for ( j = 0; j < size; j++ ) {
        sum += in[j];
    }
    if (sum == 0.) {
        outlet_float(x->rj_centroid,0.f);
    }
    else {
        for ( j = 0; j < size; j++ ) {
            sc += (float)j * in[j];
        }
         
	outlet_float(x->rj_centroid, sc / sum * sys_getsr() / (float)(size));
    }
	
	return (w+4);
}

    /* called to start DSP.  Here we call Pd back to add our perform
    routine to a linear callback list which Pd in turn calls to grind
    out the samples. */
static void rj_centroid_dsp(t_rj_centroid *x, t_signal **sp)
{
    dsp_add(rj_centroid_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void *rj_centroid_new(void)
{
    t_rj_centroid *x = (t_rj_centroid *)pd_new(rj_centroid_class);
	x->rj_centroid=outlet_new(&x->x_obj, &s_float);
	x->x_f = 0;
	
	return (x);
}

static void rj_centroid_free(t_rj_centroid *x) {
	
}


    /* this routine, which must have exactly this name (with the "~" replaced
    by "_tilde) is called when the code is first loaded, and tells Pd how
    to build the "class". */
void rj_centroid_tilde_setup(void)
{
    rj_centroid_class = class_new(gensym("rj_centroid~"), (t_newmethod)rj_centroid_new, (t_method)rj_centroid_free,
    	sizeof(t_rj_centroid), 0, A_DEFFLOAT, 0);

	    /* this is magic to declare that the leftmost, "main" inlet
	    takes signals; other signal inlets are done differently... */

    CLASS_MAINSIGNALIN(rj_centroid_class, t_rj_centroid, x_f);
    	/* here we tell Pd about the "dsp" method, which is called back
	when DSP is turned on. */
    
	class_addmethod(rj_centroid_class, (t_method)rj_centroid_dsp, gensym("dsp"), 0);
	post("rj_centroid version 0.1");
}
