/*
 * lister:  this is for lists, what "float" is for floats  (use [list]  instead)
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

static t_class *lister_class = NULL;


static void atoms_copy(int argc, t_atom *from, t_atom *to)
{
  int i;
  for (i = 0; i < argc; i++) {
    to[i] = from[i];
  }
}


static void mypdlist_storelist(t_mypdlist *x, int argc, t_atom *argv)
{
  if(x->x_list) {
    freebytes(x->x_list, x->x_n*sizeof(t_atom));
  }
  x->x_n=argc;
  x->x_list=(t_atom*)getbytes(x->x_n*sizeof(t_atom));

  atoms_copy(argc, argv, x->x_list);
}
static void mypdlist_secondlist(t_mypdlist *x, t_symbol *UNUSED(s),
                                int argc,
                                t_atom *argv)
{
  mypdlist_storelist(x, argc, argv);
}

static void mypdlist_bang(t_mypdlist *x)
{
  int outc=x->x_n;
  t_atom*outv = (t_atom*)getbytes(outc * sizeof(t_atom));
  atoms_copy(x->x_n, x->x_list, outv);
  outlet_list(x->x_obj.ob_outlet, gensym("list"), outc, outv);
  freebytes(outv, outc * sizeof(t_atom));
}


static void mypdlist_list(t_mypdlist *x, t_symbol *s, int argc,
                          t_atom *argv)
{
  mypdlist_secondlist(x, s, argc, argv);
  mypdlist_bang(x);
}


static void mypdlist_free(t_mypdlist *x)
{
  freebytes(x->x_list, x->x_n * sizeof(t_atom));
}

static void *mypdlist_new(t_symbol *UNUSED(s), int argc, t_atom *argv)
{
  t_mypdlist *x = (t_mypdlist *)pd_new(lister_class);

  outlet_new(&x->x_obj, 0);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("list"), gensym("lst2"));

  x->x_n = 0;
  x->x_list = 0;

  if(argc) {
    mypdlist_secondlist(x, gensym("list"), argc, argv);
  }

  return (x);
}


static void mypdlist_help(t_mypdlist*UNUSED(x))
{
  post("\n"HEARTSYMBOL
       " lister\t\t:: basic list storage (use pd>=0.39 for real [list] objects)");
}

static t_class* zclass_setup(const char*name)
{
  t_class *c = zexy_new(name,
                        mypdlist_new, mypdlist_free, t_mypdlist, 0, "*");
  class_addbang    (c, mypdlist_bang);
  class_addlist    (c, mypdlist_list);
  zexy_addmethod(c, (t_method)mypdlist_secondlist, "lst2", "*");
  zexy_addmethod(c, (t_method)mypdlist_help, "help", "");
  return c;
}
static void dosetup()
{
  zexy_register("lister");
  lister_class = zclass_setup("lister");
  zclass_setup("l");
}
ZEXY_SETUP void lister_setup(void)
{
  dosetup();
}
void l_setup(void)
{
  dosetup();
}
