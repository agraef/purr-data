#include "m_pd.h"


#define fran()         (t_float)rand()/(t_float)RAND_MAX

static char *version = "gauss v0.1, generates a Gaussian distributed random variable\n"
                       "            with mean 'mu' and standard deviation 'sigma',\n"
                       "            written by Olaf Matthes <olaf.matthes@gmx.de>";

/* -------------------------- rand_gauss ------------------------------ */

static t_class *gauss_tilde_class;

typedef struct _gauss_tilde
{
    t_object x_obj;
	t_float  x_sigma;
	t_float  x_mu;
} t_gauss_tilde;



t_int *gauss_tilde_perform(t_int *w)
{
	t_gauss_tilde *x = (t_gauss_tilde *)(w[1]);
	t_sample *out = (t_sample *)(w[2]);
	int n = (int)(w[3]);
	t_float u, halfN = 6.0, sum = 0, scale;
	t_int k, N = 12;
	scale = 1/sqrt(N/12);
	while (n--){
		sum = 0;
		for(k = 1; k <= N; k++)
		sum += fran();
	 	*out++ = x->x_sigma*scale*(sum-halfN)+x->x_mu;
	 }
	 return (w+4);
}

void gauss_tilde_dsp(t_gauss_tilde *x, t_signal **sp)
{
dsp_add(gauss_tilde_perform, 3, x,
sp[0]->s_vec, 
sp[0]->s_n);
}


void *gauss_tilde_new(t_floatarg fs, t_floatarg fm)
{
    t_gauss_tilde *x = (t_gauss_tilde *)pd_new(gauss_tilde_class);

    floatinlet_new(&x->x_obj, &x->x_sigma);
    floatinlet_new(&x->x_obj, &x->x_mu);
    outlet_new(&x->x_obj, &s_signal);
	x->x_sigma = fs;
	x->x_mu = fm;
    return (x);
}







