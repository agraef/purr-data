/******************************************************
 *
 * zexy - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   1999:forum::für::umläute:2004
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/

/*
  sleepgrain :  get (and set?) the sleepgrain of Pd
*/

#include "zexy.h"

EXTERN int* get_sys_sleepgrain(void ) ;

/* ------------------------- sleepgrain ------------------------------- */


static t_class *sleepgrain_class;

typedef struct _sleepgrain
{
  t_object x_obj;

} t_sleepgrain;


static void sleepgrain_bang(t_sleepgrain *x)
{
  int*current=get_sys_sleepgrain();
  t_float f=*current;
  outlet_float(x->x_obj.ob_outlet, f);
}

static void sleepgrain_float(t_sleepgrain *x, t_float f)
{
  int value=(int)f;
  int*current=get_sys_sleepgrain();

  if(value<=0) {
    pd_error(x, "[sleepgrain]: sleepgrain cannot be <= 0");
    return;
  }

  *current=value;

  //  outlet_float(x->x_obj.ob_outlet, f);
}

static void *sleepgrain_new(void)
{
  t_sleepgrain *x = (t_sleepgrain *)pd_new(sleepgrain_class);
  outlet_new(&x->x_obj, 0);
  return (x);
}

void sleepgrain_setup(void)
{
  sleepgrain_class = class_new(gensym("sleepgrain"), (t_newmethod)sleepgrain_new, 
                                     0, sizeof(t_sleepgrain), 0, A_NULL);
  
  class_addbang  (sleepgrain_class, sleepgrain_bang);
  class_addfloat (sleepgrain_class, sleepgrain_float);
  zexy_register("sleepgrain");
}
