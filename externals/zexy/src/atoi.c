/* 
 * atoi: ascii to integer converter
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

static t_class *atoi_class;

typedef struct _atoi
{
  t_object x_obj;
  int x_i;
  t_outlet*x_reject;
} t_atoi;

static void atoi_bang(t_atoi *x)
{
  outlet_float(x->x_obj.ob_outlet, (t_float)x->x_i);
}
static void atoi_float(t_atoi *x, t_float f)
{
  x->x_i = f;
  atoi_bang(x);
}
static void atoi_atoi(t_atoi *x, t_symbol*s, int base) {
  char*endptr=NULL;
  const char*str=s->s_name;
  long l=strtol(str, &endptr, base);
  size_t len=strlen(str);
  if(str+len == endptr)
    atoi_float(x, l);
  else
    outlet_symbol(x->x_reject, s);
}

static void atoi_symbol(t_atoi *x, t_symbol *sym)
{
  int base=10;
  const char* s = sym->s_name;
  if(s[0]=='0'){
    base=8;
    if (s[1]=='x')base=16;
  }
  atoi_atoi(x, sym, base);
}
static void atoi_list(t_atoi *x, t_symbol* s, int argc, t_atom *argv)
{
  int base=10;
  if (argv->a_type==A_FLOAT){
    atoi_float(x, atom_getfloat(argv));
    return;
  }
  
  if (argc>1){
    base=atom_getfloat(argv+1);
    if (base<2) {
      error("atoi: setting base to 10");
      base=10;
    }
  }
  atoi_atoi(x, s, base);
}

static void atoi_free(t_atoi *x){
  outlet_free(x->x_reject);
  x->x_reject=NULL;
}
static void *atoi_new(void)
{
  t_atoi *x = (t_atoi *)pd_new(atoi_class);
  outlet_new(&x->x_obj, gensym("float"));
  x->x_reject=outlet_new(&x->x_obj, gensym("symbol"));
  x->x_i=0;
  return (x);
}

void atoi_setup(void)
{
  atoi_class = class_new(gensym("atoi"), (t_newmethod)atoi_new, (t_method)atoi_free,
			 sizeof(t_atoi), 0, A_DEFFLOAT, 0);

  class_addbang(atoi_class, (t_method)atoi_bang);
  class_addfloat(atoi_class, (t_method)atoi_float);
  class_addlist(atoi_class, (t_method)atoi_list);
  class_addsymbol(atoi_class, (t_method)atoi_symbol);
  class_addanything(atoi_class, (t_method)atoi_symbol);

  zexy_register("atoi");
}
