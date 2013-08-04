/*
 * HOWTO write an External for Pure data
 * (c) 2001-2006 IOhannes m zmölnig zmoelnig[AT]iem.at
 *
 * this is the source-code for the third example in the HOWTO
 * it creates an object that increments and outputs a counter
 * whenever it gets banged.
 * the counter value can be "set" to a special value, or "reset" to a default
 * an upper and lower boundary can be specified: whenever the counter crosses
 * such boundary a "bang" is emitted at the 2nd outlet and the counter value wraps
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
 * the first element is the mandatory "t_object"
 * then we have all sort of variables for the
 * actual counter value, the step-size and the counting boundaries
 * finally we have 2 "t_outlet" elements so we can send data
 * to a "named" outlet.
 */
typedef struct _counter {
  t_object  x_obj;         /* mandatory t_object */
  t_int i_count;           /* the current counter value */
  t_float step;            /* step size; 
                            * this is "float" because of the passive inlet we are using */
  t_int i_down, i_up;      /* lower and upper boundary */
  t_outlet *f_out, *b_out; /* outlets */
} t_counter;


/**
 * this method is called whenever a "bang" is sent to the object
 */
void counter_bang(t_counter *x)
{
  t_float f=x->i_count;
  t_int step = x->step;
  x->i_count+=step;

  if (x->i_down-x->i_up) {
    if ((step>0) && (x->i_count > x->i_up)) {
      x->i_count = x->i_down;
      /* we crossed the upper boundary, so we send a bang out of 
       * the 2nd outlet (which is x->b_out)
       */
      outlet_bang(x->b_out);
    } else if (x->i_count < x->i_down) {
      x->i_count = x->i_up;
      outlet_bang(x->b_out);
    }
  }
  /* output the current counter value at the 1st outlet (which is x->f_out) */
  outlet_float(x->f_out, f);
}


/**
 * this is called whenever a "reset" message is sent to the inlet of the object
 * since the "reset" message has no arguments (as declared in counter_setup())
 * we only get a reference to the class-dataspace
 */
void counter_reset(t_counter *x)
{
  x->i_count = x->i_down;
}


/**
 * this is called whenever a "set" message is sent to the inlet of the object
 * since the "set" message has one floating-point argument (as declared in counter_setup())
 * we get a reference to the class-dataspace and the value 
 */
void counter_set(t_counter *x, t_floatarg f)
{
  x->i_count = f;
}


/**
 * this is called whenever a "bound" message is sent to the inlet of the object
 * note that in counter_new(), we rewrite a list to the 2nd inlet 
 * to a "bound" message to the 1st inlet
 */
void counter_bound(t_counter *x, t_floatarg f1, t_floatarg f2)
{
  x->i_down = (f1<f2)?f1:f2;
  x->i_up   = (f1>f2)?f1:f2;
}


/**
 * this is the "constructor" of the class
 * we expect a variable number of arguments to this object
 * symbol "s" is the name of the object itself
 * the arguments are given as a t_atom array of argc elements.
 */
void *counter_new(t_symbol *s, int argc, t_atom *argv)
{
  t_counter *x = (t_counter *)pd_new(counter_class);
  t_float f1=0, f2=0;

  /* depending on the number of arguments we interprete them differently */
  x->step=1;
  switch(argc){
  default:
  case 3:
    x->step=atom_getfloat(argv+2);
  case 2:
    f2=atom_getfloat(argv+1);
  case 1:
    f1=atom_getfloat(argv);
    break;
  case 0:
    break;
  }
  if (argc<2)f2=f1;

  x->i_down = (f1<f2)?f1:f2;
  x->i_up   = (f1>f2)?f1:f2;

  x->i_count=x->i_down;

  /* create a new active inlet for this object
   * a message with the selector "list" that is sent
   * to this inlet (it is the 2nd inlet from left),
   * will be appear to be the same message but with the selector "bound"
   * at the 1st inlet.
   * the method for "bound" messages is given in counter_setup()
   */
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,
        gensym("list"), gensym("bound"));

  /* create a passive inlet inlet (it will be the 2rd inlet from left)
   * whenever a floating point number is sent to this inlet,
   * its value will be immediately stored in "x->step"
   * no function will be called
   */
  floatinlet_new(&x->x_obj, &x->step);

  /* create a new outlet which will output floats
   * we store a reference to this outlet in x->f_out
   * so we are able to send data to this very outlet
   */
  x->f_out = outlet_new(&x->x_obj, &s_float);
  /* create a new outlet which will output bangs */
  x->b_out = outlet_new(&x->x_obj, &s_bang);

  return (void *)x;
}


/**
 * define the function-space of the class
 */
void counter_setup(void) {
  counter_class = class_new(gensym("counter"),
                            (t_newmethod)counter_new,
                            0, sizeof(t_counter),
                            CLASS_DEFAULT, 
                            A_GIMME, /* an arbitrary number of arguments 
                                      * which are of arbitrary type */
                            0);

  /* call a function when a "bang" message appears on the first inlet */
  class_addbang  (counter_class, counter_bang);

  /* call a function when a "reset" message (without arguments) appears on the first inlet */
  class_addmethod(counter_class,
        (t_method)counter_reset, gensym("reset"), 0);

  /* call a function when a "set" message with one float-argument (defaults to 0)
   * appears on the first inlet */
  class_addmethod(counter_class, 
        (t_method)counter_set, gensym("set"),
        A_DEFFLOAT, 0);

  /* call a function when a "bound" message with 2 float-argument (both default to 0)
   * appears on the first inlet
   * this is used for "list" messages which appear on the 2nd inlet
   * the magic is done in counter_new()
   */
  class_addmethod(counter_class,
        (t_method)counter_bound, gensym("bound"),
        A_DEFFLOAT, A_DEFFLOAT, 0);

  /* set the name of the help-patch to "help-counter"(.pd) */
  class_sethelpsymbol(counter_class, gensym("help-counter"));
}
