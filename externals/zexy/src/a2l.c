/*
 * a2l: convert anythings to lists (use [list] instead)
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
#include <string.h>

static t_class *a2l_class=NULL;

typedef struct _a2l {
  t_object x_obj;
} t_a2l;

static void a2l_anything(t_a2l *x, t_symbol *s, int argc, t_atom *argv)
{
  int n = argc+1;
  t_atom *cur, *alist = (t_atom *)getbytes(n * sizeof(t_atom));

  cur = alist;
  SETSYMBOL(cur, s);
  cur++;

  memcpy(cur, argv, argc * sizeof(t_atom));

  outlet_list(x->x_obj.ob_outlet, gensym("list"), n, alist);

  freebytes(alist, n * sizeof(t_atom));

}

static void a2l_list(t_a2l *x, t_symbol *s, int argc, t_atom *argv)
{
  outlet_list(x->x_obj.ob_outlet, s, argc, argv);
}

static void a2l_float(t_a2l *x, t_floatarg f)
{
  outlet_float(x->x_obj.ob_outlet, f);
}

static void a2l_symbol(t_a2l *x, t_symbol *s)
{
  outlet_symbol(x->x_obj.ob_outlet, s);
}

static void a2l_pointer(t_a2l *x, t_gpointer *gp)
{
  outlet_pointer(x->x_obj.ob_outlet, gp);
}

static void a2l_bang(t_a2l *x)
{
  outlet_bang(x->x_obj.ob_outlet);
}

static void *a2l_new(void)
{
  t_a2l *x = (t_a2l *)pd_new(a2l_class);
  outlet_new(&x->x_obj, 0);
  return (x);
}
static t_class* zclass_setup(const char*name)
{
  t_class *c = zexy_new(name,
                        a2l_new, 0, t_a2l, 0, "");
  class_addbang    (c, a2l_bang);
  class_addfloat   (c, a2l_float);
  class_addsymbol  (c, a2l_symbol);
  class_addpointer (c, a2l_pointer);
  class_addlist    (c, a2l_list);
  class_addanything(c, a2l_anything);
  return c;
}
static void dosetup()
{
  zexy_register("any2list");
  a2l_class=zclass_setup("any2list");
  zclass_setup("a2l");
}
void any2list_setup(void)
{
  dosetup();
}
ZEXY_SETUP void a2l_setup(void)
{
  dosetup();
}
