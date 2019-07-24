#include "MSPd.h"

static t_class *cartopol_class;

/* Pd version of cartopol~ */

#define OBJECT_NAME "cartopol~"
typedef struct _cartopol
{
	t_object x_obj;
    t_float x_f;
} t_cartopol;

void *cartopol_new(t_symbol *msg, short argc, t_atom *argv);
void cartopol_free(t_cartopol *x);
void cartopol_dsp(t_cartopol *x, t_signal **sp);


void cartopol_tilde_setup(void){
    cartopol_class = class_new(gensym("cartopol~"), (t_newmethod)cartopol_new,
                              0, sizeof(t_cartopol),0,A_GIMME,0);
	CLASS_MAINSIGNALIN(cartopol_class, t_cartopol, x_f);
    class_addmethod(cartopol_class, (t_method)cartopol_dsp, gensym("dsp"),0);
    potpourri_announce(OBJECT_NAME);
}

void *cartopol_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_cartopol *x = (t_cartopol *)pd_new(cartopol_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
	return x;
}

t_int *cartopol_perform(t_int *w)
{
    int i;
    t_cartopol *x = (t_cartopol *) w[1];
    t_float *real_in = (t_float *) w[2];
    t_float *imag_in = (t_float *) w[3];
    t_float *mag_out = (t_float *) w[4];
    t_float *phase_out = (t_float *) w[5];
    t_float imag, real;

    int n = (int) w[6]; // obj, func, 1 inlet
    int N2 = n/2;
    
    for(i = 0; i < N2 + 1; i++){
        real = (i == N2 ? real_in[1] : real_in[i]);
        imag = (i == 0 || i == N2 ? 0.0 : imag_in[i]);
        mag_out[i] = hypot(real,imag);
        phase_out[i] = -atan2(imag,real);
    }

    return (w + 7);
}

void cartopol_dsp(t_cartopol *x, t_signal **sp)
{
    dsp_add(cartopol_perform,6, x,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}
