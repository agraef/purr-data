/*
 * HOWTO write an External for Pure data
 * (c) 2001-2006 IOhannes m zmölnig zmoelnig[AT]iem.at
 *
 * this is the source-code for the second example in the HOWTO
 * it creates an object that increments and outputs a counter
 * whenever it gets banged.
 *
 * for legal issues please see the file LICENSE.txt
 */



/**
 * include the interface to Pd 
 */
#include "m_pd.h"

/**
 * define a new "class" 
 */
static t_class *counter_class;



/**
 * this is the dataspace of our new object
 * the first (mandatory) "t_object"
 * and a variable that holds the current counter value
 */
typedef struct _counter {
  t_object  x_obj;
  t_int i_count;
} t_counter;


/**
 * this method is called whenever a "bang" is sent to the object
 * a reference to the class-dataspace is given as argument
 * this enables us to do something with the data (e.g. increment the counter)
 */
void counter_bang(t_counter *x)
{
  /*
   * convert the current counter value to floating-point to output it later
   */
  t_float f=x->i_count;
  /* increment the counter */
  x->i_count++;
  /* send the old counter-value to the 1st outlet of the object */
  outlet_float(x->x_obj.ob_outlet, f);
}


/**
 * this is the "constructor" of the class
 * we have one argument of type floating-point (as specified below in the counter_setup() routine)
 */
void *counter_new(t_floatarg f)
{
  t_counter *x = (t_counter *)pd_new(counter_class);

  /* set the counter value to the given argument */
  x->i_count=f;

  /* create a new outlet for floating-point values */
  outlet_new(&x->x_obj, &s_float);

  return (void *)x;
}


/**
 * define the function-space of the class
 */
void counter_setup(void) {
  counter_class = class_new(gensym("counter"),
			    (t_newmethod)counter_new,
			    0, 
			    sizeof(t_counter),
			    CLASS_DEFAULT,
			    A_DEFFLOAT, 0); /* the object takes one argument which is a floating-point and defaults to 0 */

  /* call a function when object gets banged */
  class_addbang(counter_class, counter_bang);
}
