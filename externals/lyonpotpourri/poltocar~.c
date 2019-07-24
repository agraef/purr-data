#include "MSPd.h"

static t_class *poltocar_class;

/* Pd version of poltocar~ */

#define OBJECT_NAME "poltocar~"
typedef struct _poltocar
{
	t_object x_obj;
    t_float x_f;
} t_poltocar;

void *poltocar_new(t_symbol *msg, short argc, t_atom *argv);
void poltocar_free(t_poltocar *x);
void poltocar_dsp(t_poltocar *x, t_signal **sp);


void poltocar_tilde_setup(void){
    poltocar_class = class_new(gensym("poltocar~"), (t_newmethod)poltocar_new,
                              0, sizeof(t_poltocar),0,A_GIMME,0);
	CLASS_MAINSIGNALIN(poltocar_class, t_poltocar, x_f);
    class_addmethod(poltocar_class, (t_method)poltocar_dsp, gensym("dsp"),0);
    potpourri_announce(OBJECT_NAME);
}

void *poltocar_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_poltocar *x = (t_poltocar *)pd_new(poltocar_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
	return x;
}

t_int *poltocar_perform(t_int *w)
{
    int i;
    t_poltocar *x = (t_poltocar *) w[1];
    t_float *mag_in = (t_float *) w[2];
    t_float *phase_in = (t_float *) w[3];
    t_float *real_out = (t_float *) w[4];
    t_float *imag_out = (t_float *) w[5];
    t_float real, imag;

    int n = (int) w[6]; // obj, func, 1 inlet
    int N2 = n/2;
    
    for(i = 0; i < N2 + 1; i++){
        real = mag_in[i] * cos( phase_in[i] );
        if(i == N2){
            imag = 0;
        } else{
            imag = -mag_in[i] * sin( phase_in[i] );
        }
        real_out[i] = real;
        imag_out[i] = imag;
    }

    return (w + 7);
}

void poltocar_dsp(t_poltocar *x, t_signal **sp)
{
    dsp_add(poltocar_perform,6, x,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}
