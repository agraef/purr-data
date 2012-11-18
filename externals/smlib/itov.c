#include "defines.h"

/*--------------- itov ---------------*/


static t_class *itov_class;

typedef struct _itov
{
    t_object x_obj;
	t_outlet *f_out1;
	t_outlet *f_out2;
	t_outlet *f_out3;
	float m_lo;
	float m_hi;
	float m_scale;
	int m_nbins;
} t_itov;


static void itov_perform_float(t_itov *x, t_float j)
{
	float i;
	j-=1.0f;
	j=(j>0)?(j<x->m_nbins?j:x->m_nbins-1):0; // limit without IF

//	j=(int)((f-x->m_lo)*x->m_scale);
	i=(j/x->m_scale)-x->m_lo;

	outlet_float(x->f_out2, i);

}

static void itov_perform_list(t_itov *x, t_symbol *s, int argc, t_atom *argv)
{


}

static void itov_set(t_itov *x, t_float lo, t_float hi, t_float nbins)
{
	if (nbins<1)
	{
		nbins=1;
		post("itov: number of bins is minimum 1...");
	}
	if (hi<=lo)
	{
		post("itov: higher bound must be higher than lower bound...");	
		hi=lo+1.0f;
	}

	x->m_hi=hi;
	x->m_lo=lo;
	x->m_nbins=(int)nbins;
	x->m_scale=(float)x->m_nbins/(hi-lo);
}

static void *itov_new(t_float lo, t_float hi, t_float nbins)
{
	t_itov *x=(t_itov *)pd_new(itov_class);
	x->f_out1=outlet_new(&x->x_obj, gensym("float"));
	x->f_out2=outlet_new(&x->x_obj, gensym("float"));
	x->f_out3=outlet_new(&x->x_obj, gensym("float"));

	x->m_nbins=0;
	itov_set(x, lo, hi, nbins);
	return (void *)x;
}

static void itov_free(t_itov *x)
{
}

void itov_setup(void)
{
    itov_class = class_new(gensym("itov"),
    	(t_newmethod)itov_new, (t_method)itov_free,
		sizeof(t_itov), 
		CLASS_DEFAULT,
	    A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT,0);

//    class_addlist(itov_class, (t_method)itov_perform_list);
    class_addfloat(itov_class, (t_method)itov_perform_float);
}

