#include "MSPd.h"

static t_class *epluribus_class;

#define MAXBEATS (256)
#define OBJECT_NAME "epluribus~"
#define COMPILE_DATE "5.3.08"
#define OBJECT_VERSION "2.0"

typedef struct _epluribus
{
	t_object x_obj;
    float x_f;
	int incount; // how many inlets (must be at least 2)
	short inverse; // flag to look for minimum instead
} t_epluribus;

void *epluribus_new(t_symbol *msg, int argc, t_atom *argv);
t_int *epluribus_perform(t_int *w);
void epluribus_dsp(t_epluribus *x, t_signal **sp);
void epluribus_inverse(t_epluribus *x, t_floatarg tog);

void epluribus_tilde_setup(void)
{
    t_class *c;
	c = class_new(gensym("epluribus~"), (t_newmethod)epluribus_new,
                                0,sizeof(t_epluribus), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_epluribus, x_f);
	class_addmethod(c, (t_method)epluribus_dsp, gensym("dsp"), 0);
    class_addmethod(c, (t_method)epluribus_inverse, gensym("inverse"),A_FLOAT, 0);
    epluribus_class = c;
	potpourri_announce(OBJECT_NAME);
}

void epluribus_inverse(t_epluribus *x, t_floatarg tog)
{
	x->inverse = (short) tog;
}


void *epluribus_new(t_symbol *msg, int argc, t_atom *argv)
{
    t_epluribus *x;
    int i;
    x = (t_epluribus *)pd_new(epluribus_class);
    x->incount = (int) atom_getfloatarg(0,argc,argv);
    if(x->incount < 2 || x->incount > 256 ){
        int defcount = x->incount < 2 ? 2 : 256;
        post("%s: warning: there must be between 2 and 256 input vectors",
            OBJECT_NAME);
        post("defaulting to %d", defcount);
        x->incount = defcount;
    }

    for(i = 0; i < x->incount - 1; i++){
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"),gensym("signal"));
    }
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
	x->inverse = 0; // by default don't do inverse behaviour
	return (x);
}


t_int *epluribus_perform(t_int *w)
{
	int i,j,k;
	t_epluribus *x = (t_epluribus *) (w[1]);
	t_float *inlet;
	t_float *outlet;
	t_float *selection;
	t_int n;
	t_float maxamp = 0.0;
	t_float maxout = 0.0;
	int maxloc;
	int incount = x->incount;
	int next_pointer = incount + 5;
	
	outlet = (t_float *) w[incount + 2];
	selection = (t_float *) w[incount + 3];
	n = (int) w[incount + 4];
	
	if( x->inverse ){
		for(k = 0; k < n; k ++ ){
			maxamp = 99999999.0;
			maxloc = 0;
			for(i = 0, j=2; i < incount ; i++, j++){
				inlet = (t_float *) (w[j]);
				if( maxamp > fabs( inlet[k] ) ){
					maxamp = fabs( inlet[k] ); 
					maxout = inlet[k]; // don't actually change signal
					maxloc = i + 1; // record location of max amp
				}
			}
			outlet[k] = maxout;
			selection[k] = maxloc;
		}
	} 
	else {
		for(k = 0; k < n; k ++ ){
			maxamp = 0.0;
			maxloc = 0;
			for(i = 0, j=2; i < incount ; i++, j++){
				inlet = (t_float *) (w[j]);
				if( maxamp < fabs( inlet[k] ) ){
					maxamp = fabs( inlet[k] );
					maxout = inlet[k]; // don't actually change signal
					maxloc = i + 1; // record location of max amp
				}
			}
			outlet[k] = maxout;
			selection[k] = maxloc;
		}
	}
		
	return w + next_pointer;
}


void epluribus_dsp(t_epluribus *x, t_signal **sp)
{
	long i;
	t_int **sigvec;
	int pointer_count;

	
	if( x->incount < 2 || x->incount > 256 ){
		post("bad vector count");
		return;
	}
	pointer_count = x->incount + 4; // all metros, plus 2 outlets, plus the object pointer, plus N

	sigvec  = (t_int **) calloc(pointer_count, sizeof(t_int *));	
	for(i = 0; i < pointer_count; i++){
		sigvec[i] = (t_int *) calloc(sizeof(t_int),1);
	}
	sigvec[0] = (t_int *)x; // first pointer is to the object
	
	sigvec[pointer_count - 1] = (t_int *)sp[0]->s_n; // last pointer is to vector size (N)
	
	for(i = 1; i < pointer_count - 1; i++){ // now attach the inlet and all outlets
		sigvec[i] = (t_int *)sp[i-1]->s_vec;
	}

	dsp_addv(epluribus_perform, pointer_count, (t_int *) sigvec); 
	free(sigvec);
	
}

