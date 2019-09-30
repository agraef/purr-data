#include "MSPd.h"

static t_class *rotapan_class;

#define OBJECT_NAME "rotapan~"

/* Pd version of rotapan~ */

typedef struct _rotapan
	{
		t_object x_obj;
        t_float x_f;
		t_double sr;
        long rchans;
        double pio2;
        t_float *inarr;
        t_float **ins; // array of input signal vectors
        t_float **outs; // array of output signal vectors
	} t_rotapan;


void *rotapan_new(t_symbol *s, int argc, t_atom *argv);


void rotapan_free(t_rotapan *x);
void rotapan_assist(t_rotapan *x, void *b, long msg, long arg, char *dst);
void rotapan_version(t_rotapan *x);
void rotapan_dsp(t_rotapan *x, t_signal **sp);
t_int *rotapan_perform(t_int *w);

void rotapan_tilde_setup(void){
    rotapan_class = class_new(gensym("rotapan~"), (t_newmethod)rotapan_new,
                               (t_method)rotapan_free, sizeof(t_rotapan),0,A_GIMME,0);
	CLASS_MAINSIGNALIN(rotapan_class, t_rotapan, x_f);
    class_addmethod(rotapan_class, (t_method)rotapan_dsp, gensym("dsp"),0);
    potpourri_announce(OBJECT_NAME);
}

void *rotapan_new(t_symbol *s, int argc, t_atom *argv)
{
	int i;
    
    t_rotapan *x = (t_rotapan *)pd_new(rotapan_class);
    x->rchans = (long) atom_getfloatarg(0,argc,argv);

    /* The rotapan_perform routine does a modulo computation with
       x->rchans value as the right operand. Thus, the value cannot
       be zero.

       Here, we set rchans to 1 to keep from hitting the issue when
       no arguments are given.

       However, we want to encourage the user to actually set the argument
       in order to remain compatible with lyonpotpourri in Pd Vanilla.
       So we send an error here, too.
    */
    if (x->rchans < 1)
    {
        x->rchans = 1;
        pd_error(x, "need an argument to specify the number of channels. "
                    "Defaulting to 1.");
    }

    /* allocate in chans plus 1 for controlling the pan */
    for(i = 0; i < x->rchans; i++){
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"),gensym("signal"));
    }
    for(i=0; i < x->rchans; i++){
        outlet_new(&x->x_obj, gensym("signal"));
    }
    x->pio2 = PI / 2.0;
    x->inarr = (t_float *) malloc((x->rchans + 1) * sizeof(t_float));
    // for better compatibility with Max 6
    x->ins = (t_float **) malloc((x->rchans + 1) * sizeof(t_float *));
    x->outs = (t_float **) malloc(x->rchans * sizeof(t_float *));
    for(i = 0; i < x->rchans + 1; i++){
        x->ins[i] = (t_float *) malloc(8192 * sizeof(t_float));
    }
    return x;
}



void rotapan_free(t_rotapan *x)
{
    int i;
    for(i = 0; i < x->rchans + 1; i++){
        free(x->ins[i]);
    }
    free(x->ins);
    free(x->outs);
    free(x->inarr);
}

// try copying all vectors first!!!

t_int *rotapan_perform(t_int *w)
{
    t_rotapan *x = (t_rotapan*) w[1];
    t_float *invec;
	long rchans = x->rchans;
    t_double pio2 = x->pio2;
    t_float *inarr = x->inarr;
    t_float **ins = x->ins;
    t_float **outs = x->outs;
    double amp1, amp2;
    double panloc;
    double scaledIndex;
	int chan,i,j;
    int offset;
    
    int n = (int) w[(rchans * 2) + 3];
    
    // copy input vectors
    for(i = 0; i < rchans + 1; i++){
        invec = (t_float *) w[2 + i];
        for(j = 0; j < n; j++){
            ins[i][j] = invec[j];
        }
    }
    
    // assign output vector pointers
    for(i = 0; i < rchans; i++){
        outs[i] = (t_float *) w[3 + rchans + i];
    }
    
	for( j = 0; j < n; j++){
        for(chan = 0; chan < rchans; chan++){
            inarr[chan] = ins[chan][j];
            outs[chan][j] = 0;
        }
        scaledIndex = ins[rchans][j] * (double) rchans;
        if(scaledIndex < 0.0 || scaledIndex > rchans)
            scaledIndex = 0.0;
        
        offset = (int) floor(scaledIndex) % rchans;
        panloc = (scaledIndex - offset) * pio2;
        
        amp1 = cos( panloc );
        amp2 = sin( panloc );
        
        for(chan = 0; chan < rchans; chan++){
            outs[(chan+offset)%rchans][j] += amp1 * inarr[chan];
            outs[(chan+offset+1)%rchans][j] += amp2 * inarr[chan];
        }
	}
    return (w + (rchans * 2) + 4);
}

void rotapan_dsp(t_rotapan *x, t_signal **sp)
{
	long i;
    t_int **sigvec;
    int pointer_count = (x->rchans * 2) + 3; // input/output chans + object + panner + vectorsize
    sigvec  = (t_int **) calloc(pointer_count, sizeof(t_int *));
	for(i = 0; i < pointer_count; i++){
		sigvec[i] = (t_int *) calloc(sizeof(t_int),1);
	}
	sigvec[0] = (t_int *)x; // first pointer is to the object
	sigvec[pointer_count - 1] = (t_int *)sp[0]->s_n; // last pointer is to vector size (N)
	for(i = 1; i < pointer_count - 1; i++){ // now attach the inlet and all outlets
		sigvec[i] = (t_int *)sp[i-1]->s_vec;
	}
    dsp_addv(rotapan_perform, pointer_count, (t_int *)sigvec);
    free(sigvec);
}
