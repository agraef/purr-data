/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2011 */


#include "m_pd.h"
#include "iemlib.h"
#include <math.h>

/* ------------------------ biquad_freq_resp ------------------- */
/* -- calculates the frequency responce of a biquad structure -- */

typedef struct _biquad_freq_resp
{
  t_object  x_obj;
  t_float   a0;
  t_float   a1;
  t_float   a2;
  t_float   b1;
  t_float   b2;
  t_outlet  *x_out_re;
  t_outlet  *x_out_im;
  t_outlet  *x_out_abs;
  t_outlet  *x_out_arg;
} t_biquad_freq_resp;

static t_class *biquad_freq_resp_class;

static void biquad_freq_resp_float(t_biquad_freq_resp *x, t_floatarg freq_phase)
{
  t_float re1, im1, re2, im2, re, im;
  t_float c, s, ra;
  t_float pi = 4.0 * atan(1.0);
  
  if(freq_phase < 0.0)
    freq_phase = 0.0;
  else if(freq_phase > 180.0)
    freq_phase = 180.0;
  freq_phase *= pi;
  freq_phase /= 180.0;
  
  c = cos(freq_phase);
  s = sin(freq_phase);
  
  re1 = x->a0 + x->a1*c + x->a2*(c*c - s*s);
  im1 = x->a1*s + x->a2*2.0*(s*c);
  re2 = 1.0 - x->b1*c - x->b2*(c*c - s*s);
  im2 = -x->b1*s - x->b2*2.0*(s*c);
  ra = 1.0 / (re2*re2 + im2*im2);
  im = (re1*im2 - re2*im1) * ra; /* because z^-1 = e^-jwt, negative sign */
  re = (re1*re2 + im1*im2) * ra;
  outlet_float(x->x_out_arg, 180.0*atan2(im, re)/pi);
  outlet_float(x->x_out_abs, sqrt(re*re + im*im));
  outlet_float(x->x_out_im, im);
  outlet_float(x->x_out_re, re);
  
}
/* y/x = (a0 + a1*z-1 + a2*z-2)/(1 - b1*z-1 - b2*z-2);*/

static void biquad_freq_resp_list(t_biquad_freq_resp *x, t_symbol *s, int argc, t_atom *argv)
{
  if((argc == 5)&&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,3)&&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,1)&&IS_A_FLOAT(argv,0))
  {
    x->b1 = (float)atom_getfloatarg(0, argc, argv);
    x->b2 = (float)atom_getfloatarg(1, argc, argv);
    x->a0 = (float)atom_getfloatarg(2, argc, argv);
    x->a1 = (float)atom_getfloatarg(3, argc, argv);
    x->a2 = (float)atom_getfloatarg(4, argc, argv);
  }
}

static void *biquad_freq_resp_new(void)
{
  t_biquad_freq_resp *x = (t_biquad_freq_resp *)pd_new(biquad_freq_resp_class);
  x->x_out_re = outlet_new(&x->x_obj, &s_float);
  x->x_out_im = outlet_new(&x->x_obj, &s_float);
  x->x_out_abs = outlet_new(&x->x_obj, &s_float);
  x->x_out_arg = outlet_new(&x->x_obj, &s_float);
  x->b1 = 0.0;
  x->b2 = 0.0;
  x->a0 = 0.0;
  x->a1 = 0.0;
  x->a2 = 0.0;
  return (x);
}

void biquad_freq_resp_setup(void)
{
  biquad_freq_resp_class = class_new(gensym("biquad_freq_resp"), (t_newmethod)biquad_freq_resp_new, 0,
    sizeof(t_biquad_freq_resp), 0, 0);
  class_addfloat(biquad_freq_resp_class, biquad_freq_resp_float);
  class_addlist(biquad_freq_resp_class, (t_method)biquad_freq_resp_list);
}
