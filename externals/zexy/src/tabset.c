/*
 * tabset: set a table with a list of floats
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

/* you could also send a message [0 <data...>( to the table to achieve the same result */

#include "zexy.h"


/* =================== tabset ====================== */

static t_class *tabset_class=NULL;

typedef struct _tabset {
  t_object x_obj;
  t_symbol *x_arrayname;
} t_tabset;

static void tabset_float(t_tabset *x, t_floatarg f)
{
  t_garray *A;
  int npoints;
  t_word *vec;

  if (!(A = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class))) {
    error("%s: no such array", x->x_arrayname->s_name);
  } else if (!garray_getfloatwords(A, &npoints, &vec)) {
    error("%s: bad template for tabset", x->x_arrayname->s_name);
  } else {
    while(npoints--) {
      vec->w_float = f;
      vec++;
    }
    garray_redraw(A);
  }
}

static void tabset_list(t_tabset *x, t_symbol* UNUSED(s), int argc,
                        t_atom* argv)
{
  t_garray *A;
  int npoints;
  t_word *vec;

  if (!(A = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class))) {
    error("%s: no such array", x->x_arrayname->s_name);
  } else if (!garray_getfloatwords(A, &npoints, &vec)) {
    error("%s: bad template for tabset", x->x_arrayname->s_name);
  } else {
    if (argc>=npoints)
      while(npoints--) {
        t_float f= atom_getfloat(argv++);
        vec->w_float = f;
        vec++;
      } else {
      npoints-=argc;
      while (argc--) {
        t_float f= atom_getfloat(argv++);
        vec->w_float = f;
        vec++;
      }
      while (npoints--) {
        vec->w_float = 0.;
        vec++;
      }
    }
    garray_redraw(A);
  }
}
static void tabset_set(t_tabset *x, t_symbol *s)
{
  x->x_arrayname = s;
}

static void *tabset_new(t_symbol *s)
{
  t_tabset *x = (t_tabset *)pd_new(tabset_class);
  x->x_arrayname = s;

  return (x);
}

static void tabset_helper(void)
{
  post("\n"HEARTSYMBOL
       " tabset - object : set a table with a package of floats");
  post("'set <table>'\t: set another table\n"
       "<list>\t\t: set the table"
       "<float>\t\t: set the table to constant float\n");
  post("creation\t: \"tabset <table>\"");
}

ZEXY_SETUP void tabset_setup(void)
{
  tabset_class = zexy_new("tabset",
                          tabset_new, 0, t_tabset, 0, "S");
  class_addfloat(tabset_class, (t_method)tabset_float);
  class_addlist (tabset_class, (t_method)tabset_list);
  zexy_addmethod(tabset_class, (t_method)tabset_set, "set", "s");

  zexy_addmethod(tabset_class, (t_method)tabset_helper, "help", "");
  zexy_register("tabset");
}
