/* code for "mycobject" pd class.  This takes two messages: floating-point
numbers, and "rats", and just prints something out for each message. */

#include "m_pd.h"

    /* the data structure for each copy of "mycobject".  In this case we
    on;y need pd's obligatory header (of type t_object). */
typedef struct mycobject
{
  t_object x_ob;
} t_mycobject;

    /* this is called back when mycobject gets a "float" message (i.e., a
    number.) */
void mycobject_float(t_mycobject *x, t_floatarg f)
{
    post("mycobject: %f", f);
    x=NULL; /* don't warn about unused variables */
}

    /* this is called when mycobject gets the message, "rats". */
void mycobject_rats(t_mycobject *x)
{
    post("mycobject: rats");
    x=NULL; /* don't warn about unused variables */
}

    /* this is a pointer to the class for "mycobject", which is created in the
    "setup" routine below and used to create new ones in the "new" routine. */
t_class *mycobject_class;

    /* this is called when a new "mycobject" object is created. */
void *mycobject_new(void)
{
    t_mycobject *x = (t_mycobject *)pd_new(mycobject_class);
    post("mycobject_new");
    return (void *)x;
}

    /* this is called once at setup time, when this code is loaded into Pd. */
void mycobject_setup(void)
{
    post("mycobject_setup");
    mycobject_class = class_new(gensym("mycobject"), (t_newmethod)mycobject_new, 0,
    	sizeof(t_mycobject), 0, 0);
    class_addmethod(mycobject_class, (t_method)mycobject_rats, gensym("rats"), 0);
    class_addfloat(mycobject_class, mycobject_float);
}

