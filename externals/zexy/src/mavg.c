/*
 * mavg :: moving average filter
 *
 * (c) 1999-2011 IOhannes m zmölnig, forum::für::umläute, institute of electronic music and acoustics (iem)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "zexy.h"

static t_class *mavg_class=NULL;

typedef struct _mavg {
  t_object x_obj;

  t_float n_inv;
  t_float avg;
  int size;
  t_float *buf, *wp;
} t_mavg;

static void mavg_resize(t_mavg *x, t_float f)
{
  int i = (int)f;
  t_float *dumbuf;

  if ((i<1) || (i == x->size)) {
    return;
  }

  dumbuf = getbytes(sizeof(t_float)*i);
  if(!dumbuf) {
    pd_error(x, "unable to allocate memory for %d elements", i);
    return;
  }
  if(x->buf) {
    freebytes(x->buf, sizeof(t_float)*x->size);
  }
  x->buf = x->wp = dumbuf;
  x->size = i;
  x->n_inv = 1.0/(t_float)i;

  while(i--) {
    *dumbuf++ = x->avg;
  }
}

static void mavg_set(t_mavg *x, t_symbol* UNUSED(s), int argc,
                     t_atom *argv)
{
  int i = x->size;
  t_float *dummy = x->buf;
  t_float f=(argc)?atom_getfloat(argv):x->avg;
  if(!x->buf) {
    return;
  }

  while (i--) {
    *dummy++=f;
  }

  x->wp = x->buf;
}

static void mavg_float(t_mavg *x, t_float f)
{
  int i = x->size;
  t_float dummy = 0;
  t_float *dumb = x->buf;
  if(!x->buf) {
    return;
  }

  *x->wp++ = f;
  if (x->wp == x->buf + x->size) {
    x->wp = x->buf;
  }

  while (i--) {
    dummy += *dumb++;
  }

  x->avg = dummy*x->n_inv;

  outlet_float(x->x_obj.ob_outlet,x->avg);
}

static void *mavg_new(t_floatarg f)
{
  t_mavg *x = (t_mavg *)pd_new(mavg_class);
  int i = (f<1)?2:f;

  outlet_new(&x->x_obj, gensym("float"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym(""));
  x->size = 0;
  x->buf = x->wp = NULL;
  x->n_inv = 0.;
  x->avg = 0.;

  mavg_resize(x, i);

  return (x);
}

static void mavg_help(void)
{
  post("mavg\t:: moving average filter");
}

ZEXY_SETUP void mavg_setup(void)
{
  mavg_class = zexy_new("mavg",
                        mavg_new, 0, t_mavg, 0, "F");

  class_addfloat(mavg_class, (t_method)mavg_float);

  zexy_addmethod(mavg_class, (t_method)mavg_help, "help", "");
  zexy_addmethod(mavg_class, (t_method)mavg_set, "set", "*");
  zexy_addmethod(mavg_class, (t_method)mavg_resize, "", "F");

  zexy_register("mavg");
}
