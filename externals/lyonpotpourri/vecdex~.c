/* Required Header Files */

#include "MSPd.h"

/* The class pointer */

static t_class *vecdex_class;

/* The object structure */

typedef struct _vecdex {
	t_object obj;
	t_float x_f;
//    float top;
} t_vecdex;

#define OBJECT_NAME "vecdex~"

/* Function prototypes */

void *vecdex_new(t_symbol *msg, short argc, t_atom *argv);
void vecdex_dsp(t_vecdex *x, t_signal **sp);
t_int *vecdex_perform(t_int *w);

/* The object setup function */

void vecdex_tilde_setup(void)
{
	vecdex_class = class_new(gensym("vecdex~"), (t_newmethod)vecdex_new, 0,sizeof(t_vecdex),0,A_GIMME,0);
	CLASS_MAINSIGNALIN(vecdex_class, t_vecdex, x_f);
	class_addmethod(vecdex_class, (t_method)vecdex_dsp, gensym("dsp"), A_CANT, 0);

	potpourri_announce(OBJECT_NAME);
}

/* The new instance routine */

void *vecdex_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_vecdex *x = (t_vecdex *)pd_new(vecdex_class);
	outlet_new(&x->obj, gensym("signal"));
	return x;
}

/* The free memory function*/


/* The perform routine */

t_int *vecdex_perform(t_int *w)
{
	t_vecdex *x = (t_vecdex *) (w[1]);
//	t_float *input = (t_float *) (w[2]);
	t_float *output = (t_float *) (w[3]);
	int n = (int) w[4];
	int i;
	
	for(i=0; i < n; i++){
        output[i] = i;
	}
	return w + 5;
}

void vecdex_dsp(t_vecdex *x, t_signal **sp)
{
	dsp_add(vecdex_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}
