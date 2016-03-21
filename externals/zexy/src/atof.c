/*
 * atof: ascii to A_FLOAT converter
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
#include <stdlib.h>
#include <string.h>

static t_class *atof_class;

typedef struct _atof {
  t_object x_obj;
  t_float x_f;
  t_outlet*x_reject;
} t_atof;
static void atof_bang(t_atof *x)
{
  outlet_float(x->x_obj.ob_outlet, (t_float)x->x_f);
}
static void atof_float(t_atof *x, t_floatarg f)
{
  x->x_f = f;
  atof_bang(x);
}
static void atof_symbol(t_atof *x, t_symbol *sym)
{
  const char* s = sym->s_name;
  char*endptr=NULL;
  double d=strtod(s, &endptr);
  size_t len=strlen(s);
  if(endptr && ((s+len)==endptr)) {
    atof_float(x, d);
  } else {
    outlet_symbol(x->x_reject, sym);
  }
}
static void atof_list(t_atof *x, t_symbol* UNUSED(s), int argc,
                      t_atom *argv)
{
  if(!argc) {
    atof_bang(x);
    return;
  }

  if (argv->a_type==A_FLOAT) {
    atof_float(x, atom_getfloat(argv));
    return;
  }
  atof_symbol(x, atom_getsymbol(argv));
}
static void atof_free(t_atof*x)
{
  outlet_free(x->x_reject);
  x->x_reject=NULL;
}
static void *atof_new(void)
{
  t_atof *x = (t_atof *)pd_new(atof_class);
  outlet_new(&x->x_obj, gensym("float"));
  x->x_reject=outlet_new(&x->x_obj, gensym("symbol"));
  x->x_f = 0.;
  return (x);
}

void atof_setup(void)
{
  atof_class = class_new(gensym("atof"), (t_newmethod)atof_new,
                         (t_method)atof_free,
                         sizeof(t_atof), 0, A_DEFFLOAT, 0);

  class_addbang(atof_class, (t_method)atof_bang);
  class_addfloat(atof_class, (t_method)atof_float);
  class_addlist(atof_class, (t_method)atof_list);
  class_addsymbol(atof_class, (t_method)atof_symbol);
  class_addanything(atof_class, (t_method)atof_symbol);

  zexy_register("atof");
}
