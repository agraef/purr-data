/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iem_atan2 written by Thomas Musil (c) IEM KUG Graz Austria 2000 - 2006 */

#include "m_pd.h"
#include <math.h>

#if defined(MSW) || defined(MACOSX)
#define atan2f atan2
#endif

/* -------------------------- iem_atan2 ------------------------------ */
static t_class *iem_atan2_class;

typedef struct _iem_atan2
{
  t_object x_ob;
  t_float x_1st_arg;
  t_float x_2nd_arg;
} t_iem_atan2;

static void iem_atan2_bang(t_iem_atan2 *x)
{
  t_float r = (x->x_1st_arg == 0.0f && x->x_2nd_arg == 0.0f ? 0.0f : atan2f(x->x_1st_arg, x->x_2nd_arg));
  
  outlet_float(x->x_ob.ob_outlet, r);
}

static void iem_atan2_float(t_iem_atan2 *x, t_floatarg first_arg)
{
  t_float r = (first_arg == 0.0f && x->x_2nd_arg == 0.0f ? 0.0f : atan2f(first_arg, x->x_2nd_arg));
  
  x->x_1st_arg = first_arg;
  outlet_float(x->x_ob.ob_outlet, r);
}

static void *iem_atan2_new(void)
{
  t_iem_atan2 *x = (t_iem_atan2 *)pd_new(iem_atan2_class);
  
  floatinlet_new(&x->x_ob, &x->x_2nd_arg);
  x->x_1st_arg = 0.0f;
  x->x_2nd_arg = 0.0f;
  outlet_new(&x->x_ob, &s_float);
  return (x);
}

void iem_atan2_setup(void)
{
  iem_atan2_class = class_new(gensym("iem_atan2"), (t_newmethod)iem_atan2_new,
    0, sizeof(t_iem_atan2), 0, 0);
  class_addbang(iem_atan2_class, (t_method)iem_atan2_bang);
  class_addfloat(iem_atan2_class, (t_method)iem_atan2_float);
//  class_sethelpsymbol(iem_atan2_class, gensym("iemhelp/help-iem_atan2"));
  post("iem_atan2 (R-1.17) library loaded!   (c) Thomas Musil 11.2006");
	post("   musil%ciem.at iem KUG Graz Austria", '@');
}

