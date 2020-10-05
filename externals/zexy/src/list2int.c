/*
 * list2int:  cast each float of a list (or anything) to integer
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

static t_class *list2int_class=NULL;

static void list2int_any(t_mypdlist *x, t_symbol *s, int argc,
                         t_atom *argv)
{
  t_atom *ap;
  if (x->x_n != argc) {
    freebytes(x->x_list, x->x_n * sizeof(t_atom));
    x->x_n = argc;
    x->x_list = copybytes(argv, argc * sizeof(t_atom));
  } else {
    memcpy(x->x_list, argv, argc * sizeof(t_atom));
  }
  ap = x->x_list;
  while(argc--) {
    if(ap->a_type == A_FLOAT) {
      ap->a_w.w_float=(int)ap->a_w.w_float;
    }
    ap++;
  }
  outlet_anything(x->x_obj.ob_outlet, s, x->x_n, x->x_list);
}
static void list2int_bang(t_mypdlist *x)
{
  outlet_bang(x->x_obj.ob_outlet);
}
static void list2int_float(t_mypdlist *x, t_float f)
{
  outlet_float(x->x_obj.ob_outlet, (int)f);
}
static void list2int_symbol(t_mypdlist *x, t_symbol *s)
{
  outlet_symbol(x->x_obj.ob_outlet, s);
}
static void list2int_pointer(t_mypdlist *x, t_gpointer *p)
{
  outlet_pointer(x->x_obj.ob_outlet, p);
}

static void *list2int_new(t_symbol *s, int argc, t_atom *argv)
{
  t_mypdlist *x = (t_mypdlist *)pd_new(list2int_class);
  outlet_new(&x->x_obj, 0);
  x->x_n = 0;
  x->x_list = 0;

  list2int_any(x, s, argc, argv);
  return (x);
}


static void mypdlist_free(t_mypdlist *x)
{
  freebytes(x->x_list, x->x_n * sizeof(t_atom));
}
static t_class* zclass_setup(const char*name)
{
  t_class *c = zexy_new(name,
                        list2int_new, mypdlist_free, t_mypdlist, 0, "*");
  class_addanything(c, list2int_any);
  class_addlist(c, list2int_any);
  class_addbang(c, list2int_bang);
  class_addfloat(c, list2int_float);
  class_addsymbol(c, list2int_symbol);
  class_addpointer(c, list2int_pointer);
  return c;
}
static void dosetup()
{
  zexy_register("list2int");
  list2int_class=zclass_setup("list2int");
  zclass_setup("l2i");
}
ZEXY_SETUP void list2int_setup(void)
{
  dosetup();
}
void l2i_setup(void)
{
  dosetup();
}
