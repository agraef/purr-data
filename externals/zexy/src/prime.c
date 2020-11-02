/*
 * prime:  get the n-th prime number
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


static t_class *prime_class=NULL;

typedef struct _prime {
  t_object  x_obj;
} t_prime;


static void prime_float(t_prime *x, t_float f)
{

  unsigned int i=f;
  unsigned int max_divisor;
  unsigned int divisor=1;

  if (f<2) {
    outlet_float(x->x_obj.ob_outlet, 0.0);
    return;
  }

  if (!(i%2)) {
    outlet_float(x->x_obj.ob_outlet, (t_float)(i==2));
    return;
  }

  max_divisor = sqrt(f)+1;

  while ((divisor+=2)<max_divisor)
    if (!(i%divisor)) {
      outlet_float(x->x_obj.ob_outlet, 0.0);
      return;
    }

  outlet_float(x->x_obj.ob_outlet, 1.0);
}

static void *prime_new(void)
{
  t_prime *x = (t_prime *)pd_new(prime_class);

  outlet_new(&x->x_obj, gensym("float"));

  return (x);
}

static void prime_help(t_prime*UNUSED(x))
{
  post("\n"HEARTSYMBOL " prime\t\t:: test whether a given number is prime");
}


ZEXY_SETUP void prime_setup(void)
{
  prime_class = zexy_new("prime",
                         prime_new, 0, t_prime, CLASS_DEFAULT, "");

  class_addfloat(prime_class, prime_float);
  zexy_addmethod(prime_class, (t_method)prime_help, "help", "");
  zexy_register("prime");
}
