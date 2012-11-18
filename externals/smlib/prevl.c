#include "defines.h"

/*--------------- prevl ---------------*/

static t_class *prevl_class;

typedef struct _prevl
{
    t_object x_obj;
	float m_lo;
	float m_hi;
	int m_buffer_size;
	int m_buffer_index;
	float *m_buffer;  // circular buffer
} t_prevl;


static void prevl_perform_float(t_prevl *x, t_float f)
{
	int index;
	index=x->m_buffer_index+1;
	index=(index==x->m_buffer_size)?0:index;
	x->m_buffer_index=index;
	x->m_buffer[index]=f;
}

static void prevl_bang(t_prevl *x)
{
	int lo,hi,n,index,size;
	t_atom *ap,*app;
	float *buffer, *bp;

	lo=(int)x->m_lo;
	hi=(int)x->m_hi;

	n=hi-lo;
	size=x->m_buffer_size;
	index=x->m_buffer_index;
    ap = (t_atom *)getbytes(sizeof(t_atom)*n);
	app=ap;
	buffer=x->m_buffer;
	bp=buffer+index-lo;
	bp=(bp>=buffer)?bp:bp+size; // wrap
	
	if (bp-buffer>=n)
	{ // no wrap-around needed
		index=n;
		while(index--){
		  SETFLOAT(app, *bp--);
		  app++;
		}
//		post("not wrapped, app-ap=%i",app-ap);
	}
	else // need to wrap
	{
		int ps, nn;
		ps = bp-buffer;
		nn=n;
//		post("        nn=%i",nn);
		for(;ps>=0;ps--)  // don't we miss one sample in signal??? 
		{
//			post("ps=%i",ps);
 			SETFLOAT(app, buffer[ps]);
			app++;
			nn--;
		}
		ps=size-1;
//		post("       nn=%i",nn);
		for(;nn>0;nn--)
		{
//			post("ps=%i",ps);
 			SETFLOAT(app, buffer[ps--]);
			app++;
		}

	}

	outlet_list(x->x_obj.ob_outlet,gensym("list"),n,ap);
	freebytes(ap, sizeof(t_atom)*n);
}

static void prevl_clear(t_prevl *x)
{
	int i,s;
	float *f;
	f=x->m_buffer;
	s=x->m_buffer_size;
	for (i=0;i<s;i++)
		*f++=0.0f;
}

static void prevl_set(t_prevl *x, t_float lo, t_float hi, t_float size)
{
	if (size<1)
	{
		size=1;
		post("prevl: size is minimum 1...");
	}
	if (hi>size)
	{
		post("prevl: higher bound cannot be higher than the buffer size...");	
		hi=size;
	}
	if (lo<0)
	{
		post("prevl: lower bound cannot be negative...");	
		lo=0;
	}
	if (hi<1)
	{
		post("prevl: higher bound cannot be smaller than one...");	
		hi=1;
	}
	if (hi<=lo)
	{
		post("prevl: higher bound must be higher than lower bound...");	
		lo=hi-1.0f;
	}

	freebytes(x->m_buffer, x->m_buffer_size);

	x->m_hi=(float)((int)hi);
	x->m_lo=(float)((int)lo);
	x->m_buffer_size=(int)size;
    x->m_buffer = (float*)getbytes(sizeof(float)*x->m_buffer_size);
	prevl_clear(x);
	x->m_buffer_index=0;
}

static void *prevl_new(t_float lo, t_float hi, t_float size)
{
	t_prevl *x=(t_prevl *)pd_new(prevl_class);
	outlet_new(&x->x_obj, gensym("list"));
	x->m_buffer_size=0;
	x->m_buffer=0;
	prevl_set(x, lo, hi, size);

    floatinlet_new(&x->x_obj, &x->m_lo);
    floatinlet_new(&x->x_obj, &x->m_hi);

	return (void *)x;
}

static void prevl_free(t_prevl *x)
{
	freebytes(x->m_buffer, x->m_buffer_size);
}

void prevl_setup(void)
{
    prevl_class = class_new(gensym("prevl"),
    	(t_newmethod)prevl_new, (t_method)prevl_free,
		sizeof(t_prevl), 
		CLASS_DEFAULT,
	    A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT,0);

	class_addmethod(prevl_class, (t_method)prevl_clear, gensym("clear"),0);
    class_addfloat(prevl_class, (t_method)prevl_perform_float);
    class_addbang(prevl_class, (t_method)prevl_bang);
}

