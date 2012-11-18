/*
* Trying logistic function for audio processing
*/

#include "m_pd.h"

static t_class *logistic_tilde_class;

typedef struct _logistic_tilde {
t_object x_obj;
t_sample f_logistic;
t_sample f;
t_float a;
} t_logistic_tilde;

t_sample logistic(t_sample x,t_float a) {
	return a*x*(1-x);
}

t_int *logistic_tilde_perform(t_int *w)
{
t_logistic_tilde *x = (t_logistic_tilde *)(w[1]);
//t_sample *in1 = (t_sample *)(w[2]);
//t_sample *in2 = (t_sample *)(w[3]);
t_sample *out = (t_sample *)(w[2]);
int n = (int)(w[3]);
//t_sample f_logistic = (x->f_logistic<0)?0.0:(x->f_logistic>1)?1.0:x->f_logistic;
while (n--){
	  x->f_logistic = logistic(x->f_logistic,x->a);
	 *out++ = x->f_logistic - 0.5;
	 }

return (w+4);
}



void logistic_tilde_dsp(t_logistic_tilde *x, t_signal **sp)
{
dsp_add(logistic_tilde_perform, 3, x,
sp[0]->s_vec, 
//sp[1]->s_vec, 
//sp[2]->s_vec,
sp[0]->s_n);
}

void *logistic_tilde_new(t_floatarg f)
{
t_logistic_tilde *x = (t_logistic_tilde *)pd_new(logistic_tilde_class);
x->f_logistic = 0.5;
x->a = 3.7;
if (x->a < 0) x->a = 1;
if (x->a > 4) x->a = 4;
//inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
floatinlet_new (&x->x_obj, &x->a);
outlet_new(&x->x_obj, &s_signal);
return (void *)x;
}

