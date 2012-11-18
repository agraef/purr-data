/* 
ritmo1
try to extract the current pattern
*/
#include "m_pd.h"

#define BUFFER_LENGHT 16

static t_class *ritmo1_class;

typedef struct _ritmo1
{
    t_object x_obj; // myself
    t_symbol *x_arrayname_src; // where i read the current pattern
    t_symbol *x_arrayname_dest; // where i put the computed pattern
	t_float *buf1;
//	t_float *buf2;
//	t_float *buf3;
} t_ritmo1;

void ritmo1_allocate_buffers(t_ritmo1 *x)
{
	x->buf1 = (t_float *)getbytes(BUFFER_LENGHT * sizeof(t_float));
//	x->buf2 = (t_float *)getbytes(BUFFER_LENGHT * sizeof(t_float));
//	x->buf3 =  (t_float *)getbytes(BUFFER_LENGHT * sizeof(t_float));
}

void ritmo1_free(t_ritmo1 *x)
{
	freebytes(x->buf1, sizeof(x->buf1));
//	freebytes(x->buf2, sizeof(x->buf2));	
//	freebytes(x->buf3, sizeof(x->buf3));
}

static void ritmo1_bang(t_ritmo1 *x) {

	int i, vecsize;
	t_garray *arysrc;
	t_garray *arydest;
	t_float *vecsrc;
	t_float *vecdest;

	if (!(arysrc = (t_garray *)pd_findbyclass(x->x_arrayname_src, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src->s_name);
	}
    else if (!garray_getfloatarray(arysrc, &vecsize, &vecsrc))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src->s_name);
	}
	  else 	if (!(arydest = (t_garray *)pd_findbyclass(x->x_arrayname_dest, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_dest->s_name);
	}
    else if (!garray_getfloatarray(arydest, &vecsize, &vecdest))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_dest->s_name);
	}
	else // I got arrays and data
	{
		// step 1: compute the pattern
		// and write it in vecdest
		for (i=0; i<16; i++)
		{
	//		vecdest[i] = (x->buf1[i] + x->buf2[i] + x->buf3[i] + vecsrc[i])/4;
			vecdest[i] = (x->buf1[i] + vecsrc[i])/2;
		}
		// redraw the arrays
		garray_redraw(arysrc);
		garray_redraw(arydest);

		// step 2: cycle buffers
	//	x->buf3 = x->buf2;
	//	x->buf2 = x->buf1;
	//	x->buf1 = x->buf3;
		// fill the first buffer with src data
		for (i=0; i<16; i++)
		{
			x->buf1[i] = vecsrc[i];
			vecsrc[i]=0;
		}
	}
}

static void ritmo1_src(t_ritmo1 *x, t_symbol *s) {
    x->x_arrayname_src = s;
}

static void ritmo1_dest(t_ritmo1 *x, t_symbol *s) {
    x->x_arrayname_dest = s;
}

static void *ritmo1_new(t_symbol *s, int argc, t_atom *argv)
{
    t_ritmo1 *x = (t_ritmo1 *)pd_new(ritmo1_class);
	ritmo1_allocate_buffers(x);
	if (argc>0) 
	{
		x->x_arrayname_src = atom_getsymbolarg(0, argc, argv);
	} 
	if (argc>1) 
	{
		x->x_arrayname_dest = atom_getsymbolarg(1, argc, argv);
	}
    return (x);
}

void ritmo1_setup(void)
{
    ritmo1_class = class_new(gensym("ritmo1"), (t_newmethod)ritmo1_new,
        (t_method)ritmo1_free, sizeof(t_ritmo1), CLASS_DEFAULT, A_GIMME, 0);
    class_addbang(ritmo1_class, (t_method)ritmo1_bang);
    class_addmethod(ritmo1_class, (t_method)ritmo1_src, gensym("src"),A_SYMBOL, 0);
    class_addmethod(ritmo1_class, (t_method)ritmo1_dest, gensym("dest"),A_SYMBOL, 0);
}


