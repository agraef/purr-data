/* 
 * author: David Merrill <dmerrill@media.mit.edu>
 * based on code from Tom Schouten, found online at:
 * http://lists.puredata.info/pipermail/pd-list/2002-02/004871.html
 */

#include "m_pd.h"
#include <stdio.h>

#define MAXHEAD 1024

typedef struct{
    char buffer[MAXHEAD];
} t_parseeventnodenum_data;
 
typedef struct parseeventnodenum
{
  t_object t_ob;
  t_outlet *x_out;  
  t_outlet *x_out_error;
} t_parseeventnodenum;

void penn_any_method(t_parseeventnodenum *x, t_symbol *s, int argc, t_atom *argv)
{
    char* p = s->s_name;
    int eventnodenum = -1, rv = 0;

    if (sscanf(p,"/dev/input/event%i",&eventnodenum)) {

	// success, send the float to the left outlet
	outlet_float(x->x_out, eventnodenum);
    } else {

	// failure, send the input symbol to the right outlet
	outlet_symbol(x->x_out_error, gensym(p));
    }
}

void parseeventnodenum_free(void)
{
}

t_class *parseeventnodenum_class;

void *parseeventnodenum_new(void)
{
    t_parseeventnodenum *x = (t_parseeventnodenum *)pd_new(parseeventnodenum_class);

    // left outlet is where the parsed number will come out
    x->x_out = outlet_new(&x->t_ob, gensym("float"));

    // right outlet is where the original string will come out if an error occured
    x->x_out_error = outlet_new(&x->t_ob, gensym("symbol"));


    return (void *)x;
}

void parseeventnodenum_setup(void)
{
    parseeventnodenum_class = class_new(gensym("parseeventnodenum"), (t_newmethod)parseeventnodenum_new,
    	(t_method)parseeventnodenum_free, sizeof(t_parseeventnodenum), 0, 0);
    class_addanything(parseeventnodenum_class, penn_any_method);
}

