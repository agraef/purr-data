/* Required Header Files */

#include "MSPd.h"

/* The class pointer */

static t_class *windowvec_class;

/* The object structure */

typedef struct _windowvec {
	t_object obj;
	t_float x_f;
	float *envelope;
	long vecsize;
	long oldbytes;
} t_windowvec;

#define OBJECT_NAME "windowvec~"

/* Function prototypes */

void *windowvec_new(void);
void windowvec_dsp(t_windowvec *x, t_signal **sp, short *count);
t_int *windowvec_perform(t_int *w);

/* The object setup function */

void windowvec_tilde_setup(void)
{
	windowvec_class = class_new(gensym("windowvec~"), (t_newmethod)windowvec_new, 0, sizeof(t_windowvec), 0,0);
	CLASS_MAINSIGNALIN(windowvec_class, t_windowvec, x_f);
	class_addmethod(windowvec_class, (t_method)windowvec_dsp, gensym("dsp"), A_CANT, 0);
	potpourri_announce(OBJECT_NAME);
}

/* The new instance routine */

void *windowvec_new(void)
{
	t_windowvec *x = (t_windowvec *)pd_new(windowvec_class);
	outlet_new(&x->obj, gensym("signal"));
	x->vecsize = 0;
	x->envelope = NULL;
	return x;
}

/* The free memory function*/

void windowvec_free(t_windowvec *x, t_signal **sp, short *count)
{
	freebytes(x->envelope, x->oldbytes);
}

/* The perform routine */

t_int *windowvec_perform(t_int *w)
{
	t_windowvec *x = (t_windowvec *) (w[1]);
	t_float *input = (t_float *) (w[2]);
	t_float *output = (t_float *) (w[3]);
	int n = (int) w[4];
	int i;
	float *envelope = x->envelope;
	
	/* Apply a Hann window to the input vector */
	
	for(i=0; i < n; i++){
		output[i] = input[i] * envelope[i];
	}
	return w + 5;
}

void windowvec_dsp(t_windowvec *x, t_signal **sp, short *count)
{
	int i;
	float twopi = 8. * atan(1);
	if(x->vecsize != sp[0]->s_n){
		x->vecsize = sp[0]->s_n;
		
		/* Allocate memory */
		
		if(x->envelope == NULL){
			x->envelope = (float *) getbytes(x->vecsize * sizeof(float));
		} else {
			x->envelope = (float *) resizebytes(x->envelope, x->oldbytes, x->vecsize * sizeof(float));
		}
		x->oldbytes = x->vecsize * sizeof(float);
		
		/* Generate a Hann window */
		
		for(i = 0 ; i < x->vecsize; i++){
			x->envelope[i] = - 0.5 * cos(twopi * (i / (float)x->vecsize)) + 0.5;
		}
	}
	dsp_add(windowvec_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}